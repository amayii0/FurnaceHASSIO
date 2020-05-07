/*********************************************************************************************************************
 ** 2019-01-27, RDU: Rebuild for furnace monitoring and control
 **                  D2 = Data bus (this actually supports multiple sensors using OneWire)
 ** 2019-01-28, RDU: Adding control for 2 relays (Furnace on/off, Circulator on/off) using a 5V relay board
 ** 2019-01-29, RDU: Fool proofing DS18B20 against unread values (-127.00)
 ** 2019-07-30, RDU: Migrated PlatformIO from Atom to VSCode, updated project structure
 ** 2019-08-07, RDU: Updated lib_deps to rely solely on project's deps and not on any global libs, includes use of ArduinoJson 5.x
 ** 2020-04-23, RDU: /!\ For "production" PCB, the SCL/SDA are reassigned to simplify wiring
 ** 2020-05-07, RDU: Implemented restoration of relays' states from MQTT retained messages
 **
 ** TODOs
 ** - (Don't?) Implement NO/NC states logic for relays
 ** - Support multiple sensors as a HomieRange
 **
 *********************************************************************************************************************/

#include <Homie.h>
#include "main.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals
  // DS18b20
    #include <OneWire.h>
    #include <DallasTemperature.h>
    #define TEMP_INVALID 0.12345 // Invalid temperature
    #define TEMP_NOT_READ -127.00 // Unable to read temperature

    OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
    DallasTemperature dtsensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.

    // Sensor consts/vars
      HomieNode temperatureWaterInNode("temperature-WaterIn", "Temperature", "temperature");
      HomieNode temperatureWaterOutNode("temperature-WaterOut", "Temperature", "temperature");
      HomieNode switchPowerNode("switch-power", "Switch", "switch");
      HomieNode switchCirculatorNode("switch-circulator", "Switch", "switch");
      char* switchPowerNodeTopic;
      char* switchCirculatorNodeTopic;
      int restoreTopicsCount = 0;

    // Measure loop
      unsigned long lastMeasureSent = 0;

  // OLED I2C Screen
      #include <Adafruit_SSD1306.h>
      #include <furnaceLogos.h>
      #include <math.h> // Used to format states

    // Screen consts/vars
      Adafruit_SSD1306 display; // In order to swap pins for PCB, the display is created during screenInit

    // Buffers for screen info
      int switchCirculatorState = -1;
      int switchPowerState = -1;
      float tempInState = TEMP_NOT_READ;
      float tempOutState = TEMP_NOT_READ;

  // MQTT Restore state
      #include <Ethernet.h>
      #include <PubSubClient.h>
    
    // Globals
      bool isRestorationSetup = false;
      WiFiClient espClient;
      PubSubClient client(espClient);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility function to get temperature from a DS18B20 on the bus, based on its index
bool isValidTemperature(float readTemperature)
{
  return readTemperature != TEMP_INVALID && readTemperature > TEMP_NOT_READ;
}

float getTemFromDS18B20(int idx)
{
  float rawTemp = dtsensors.getTempCByIndex(idx);
  if (isnan(rawTemp)) {
    Homie.getLogger() << F("Failed to read from sensor ! Index=") << idx;
    return TEMP_INVALID;
  } else {
    Homie.getLogger() << F("Temperature: ") << rawTemp << " °C, index=" << idx << ", IsValidTemp=" << isValidTemperature(rawTemp)<< endl;
    return rawTemp;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility functions to change relay state
bool isValueHigh(const String& value)
{
    if (value=="true") return true;
    if (value=="TRUE") return true;
    if (value=="on") return true;
    if (value=="ON") return true;
    if (value=="1") return true;
    return false;
}

bool isValueLow(const String& value)
{
    if (value=="false") return true;
    if (value=="FALSE") return true;
    if (value=="off") return true;
    if (value=="OFF") return true;
    if (value=="0") return true;
    return false;
}

bool isValidSwitchValue(const String& value)
{
  return isValueHigh(value) || isValueLow(value);
}

int valueToState(const String& value)
{
  return isValueHigh(value) ? RELAY_STATE_ON : RELAY_STATE_OFF;
}

String valueToPayload(const String& value)
{
  return isValueHigh(value) ? "ON" : "OFF";
}

// Switch handlers for Power Switch
bool switchPowerOnHandler(const HomieRange& range, const String& value) {
  if (!isValidSwitchValue(value))
  {
    Homie.getLogger() << "Power switch received an invalid value : " << value << endl;
    return false;
  }

  switchPowerState = valueToState(value);
  digitalWrite(RELAY_PIN_POWER, valueToState(value));
  switchPowerNode.setProperty("on").send(valueToPayload(value));
  Homie.getLogger() << "Power switch is " << value << endl;

  return true;
}

// Switch handlers for Circulator Switch
bool switchCirculatorOnHandler(const HomieRange& range, const String& value) {
  if (!isValidSwitchValue(value))
  {
    Homie.getLogger() << "Circulator switch received an invalid value : " << value << endl;
    return false;
  }

  switchCirculatorState = valueToState(value);
  digitalWrite(RELAY_PIN_CIRCULATOR, valueToState(value));
  switchCirculatorNode.setProperty("on").send(valueToPayload(value));
  Homie.getLogger() << "Circulator switch is " << value << endl;

  return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility functions for OLED Screen
#define CM_WHITE 0
#define CM_BLACK 1
#define CM_INVERT 2
#define ROT_EAST 0
#define ROT_WEST 2
#define ROT_NORTH 3
#define ROT_SOUTH 1

void printText(int16_t x, int16_t y, char* text, int16_t fontSize, int colorMode, uint8_t rotation = 0.0)
{
  // Define position
  display.setCursor(x, y);

  // Rotation
  display.setRotation(rotation);

  // Define font size
  display.setTextSize(fontSize);

  // Define color
  switch (colorMode)
  {
    case CM_WHITE:
      display.setTextColor(WHITE);
      break;
    case CM_BLACK:
      display.setTextColor(BLACK);
      break;
    case CM_INVERT:
      display.setTextColor(BLACK, WHITE);
      break;
  }

  // Output
  display.print(text);
}

void printText(int16_t x, int16_t y, const char* text, int16_t fontSize, int colorMode, uint8_t rotation = 0.0)
{
  printText(x, y, (char*)text, fontSize, colorMode, rotation);
}

void printNumber(int16_t x, int16_t y, int value, int16_t fontSize, int colorMode, uint8_t rotation = 0.0)
{
  // Define position
  display.setCursor(x, y);

  // Rotation
  display.setRotation(rotation);

  // Define font size
  display.setTextSize(fontSize);

  // Define color
  switch (colorMode)
  {
    case CM_WHITE:
      display.setTextColor(WHITE);
      break;
    case CM_BLACK:
      display.setTextColor(BLACK);
      break;
    case CM_INVERT:
      display.setTextColor(BLACK, WHITE);
      break;
  }

  // Output
  display.print(value);
}

void screenPrintStates()
{
  // Temperatures
    if (tempInState != TEMP_NOT_READ) printNumber(5, 40, round(tempInState), 2, CM_INVERT);
    if (tempOutState != TEMP_NOT_READ) printNumber(36, 40, round(tempOutState), 2, CM_INVERT);

  // Relays
    if (switchPowerState != -1) printText(84, 10, (switchPowerState==RELAY_STATE_ON?"ON ":"OFF"), 2, CM_INVERT);
    if (switchCirculatorState != -1) printText(84, 40, (switchCirculatorState==RELAY_STATE_ON?"ON ":"OFF"), 2, CM_INVERT);

  display.display();
}

void screenPrintHeaders()
{
  // Cleanup
    display.clearDisplay();

  // Draw surrounding box, vertical delimiter between temps & relays, underline for temp legend
    display.drawRect(0, 0, 123, 63, WHITE);  // Box
    display.drawLine(64, 0, 64, 63, WHITE);  // Split left-right
    display.drawLine(10, 32, 54, 32, WHITE); // Temperatures legend underline
    display.drawLine(75, 32, 114, 32, WHITE); // Split relays

  // Draw bitmaps
    // Temperature logo with in/out arrows
    display.drawBitmap(4, 13, arrow_r_logo, arrow_r_w, arrow_r_h, WHITE);
    display.drawBitmap(25, 10, thermometer_logo, thermometer_w, thermometer_h, WHITE);
    display.drawBitmap(37, 13, arrow_r_logo, arrow_r_w, arrow_r_h, WHITE);

    // Power logo, circulator logo
    display.drawBitmap(66, 10, power_logo, power_w, power_h, WHITE);
    display.drawBitmap(66, 40, circulator_logo, circulator_w, circulator_h, WHITE);

  display.display();
}

void screenInit()
{
  Homie.getLogger() << F("I2C OLED Screen : address d") << OLED_I2C_ADDRESS << endl;
  Homie.getLogger() << F("  > Init in progress . . .") << endl;

  // Enforce remap of default pins to swap SCL/SDA to ease PCB routing
  Homie.getLogger() << F("  > Remap SCL=") << OLED_PIN_SCL << F(", SDA=") << OLED_PIN_SDA << endl;
  Wire.endTransmission();
  Wire.pins(OLED_PIN_SDA, OLED_PIN_SCL);
  display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Homie.getLogger() << F("  > Allocation failed, check pinout, not found") << endl;
    for(;;); // Don't proceed, loop forever
  }
  Homie.getLogger() << F("  > Init done") << endl;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility functions for loop
void loopHandlerDS18B20()
{
  if (millis() - lastMeasureSent >= LOOP_INTERVAL * 1000UL || lastMeasureSent == 0) {
    dtsensors.requestTemperatures();
    float rawTemp = TEMP_INVALID;

    // Water out temp
    rawTemp = getTemFromDS18B20(DS18B20_INDEX_WATEROUT);
    if (isValidTemperature(rawTemp)) {
      tempOutState = rawTemp;
      temperatureWaterOutNode.setProperty("degrees").send(String(rawTemp));
    }

    // Water in temp
    rawTemp = getTemFromDS18B20(DS18B20_INDEX_WATERIN);
    if (isValidTemperature(rawTemp))
    {
      tempInState = rawTemp;
      temperatureWaterInNode.setProperty("degrees").send(String(rawTemp));
    }

    lastMeasureSent = millis();
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// States restoration
void restoreStateFromTopic(char* topic, char* payload)
{
  Homie.getLogger() << F("State restoration for ") << topic << endl;

  // Update topic to include the final /set command
  char topicSet[255]{};
  memset(&topicSet[0], 0, sizeof(topicSet));
  memcpy(topicSet, topic, sizeof(topicSet));
  strcat_P(topicSet, PSTR("/set"));

  // Push payload to MQTT to force all subscribers to refresh, including our own nodes
  if (client.publish(topicSet, payload))
  {
    Homie.getLogger() << F("  ✔ State restored : ") << payload << endl;
    
    // After restoration, unsubscribe from topic as this is a one shot
    if (client.unsubscribe(topic))
    {
      Homie.getLogger() << F("  ✔ Unsubscribed") << endl;
      restoreTopicsCount--;
    }
    else
    {
      Homie.getLogger() << F("  ✖ Unsubscribe failed") << endl;
    }
  }
  else
  {
    Homie.getLogger() << F("  ✖ State restoration failed : ") << payload << endl;
  }

  // Free connection to MQTT broker if we are done
  if (restoreTopicsCount==0)
  {
    client.disconnect();
  }
}

void restoreStateCallback(char* topic, byte* payload, unsigned int length) {
  // Get clean buffers for topic
  char topicBuffer[255]{};
  memcpy(topicBuffer, topic, sizeof(topicBuffer));

  // Do we have to restore this topic? If so, do it
  if (strcmp(topic, switchPowerNodeTopic) == 0 || strcmp(topic, switchCirculatorNodeTopic) == 0)
  {
    // Copy byte* payload to a char* and lower case it
    char payloadBuffer[length+1];
    memset(&payloadBuffer[0], 0, sizeof(payloadBuffer));
    memcpy(payloadBuffer, payload, length);
    for (unsigned int i=0; i<length; i++)
    {
      payload[i] = tolower(payload[i]);
    }

    restoreStateFromTopic(topicBuffer, payloadBuffer);
  }
}

char* getNodeTopic(const HomieNode& node)
{
  int topicLen =
      strlen(Homie.getConfiguration().mqtt.baseTopic)
    + strlen(Homie.getConfiguration().deviceId) + 1
    + strlen(switchPowerNode.getId()) + 1
    + 2 + 1 // ON + \0
    ;

  char* topic = new char[topicLen];
  memset(&topic[0], 0, topicLen);
  strcpy(topic, Homie.getConfiguration().mqtt.baseTopic); // devices/
  strcat(topic, Homie.getConfiguration().deviceId); // bcddc2256bad
  strcat_P(topic, PSTR("/"));
  strcat(topic, node.getId()); // switch-power
  strcat_P(topic, PSTR("/on"));

  return topic;
}

bool subscribeRestoreTopic(const char* topic)
{
  if (client.subscribe(topic))
  {
    Homie.getLogger() << "  ✔ Subscribed to : " << topic << endl;
    restoreTopicsCount++;
    return true;
  }
  else
  {
    Homie.getLogger() << "  ✖ Subscribe failed to : " << topic << endl;
    return false;
  }
}

void restoreState() {
  // Skip if no topic is to be restored
  if (isRestorationSetup && restoreTopicsCount == 0)
    return;

  // Check for pending retained messages
  if (isRestorationSetup)
  {
    client.loop();
    return;
  }

  // Skip if we don't have a connection
  if (!Homie.isConnected())
    return;

  Homie.getLogger() << "Restore state" << endl;

  // Network configuration of the MQTT server/port, read from Homie configuration
  // We are not using the built-in Homie MQTT connection to enforce reception of retained messages
  IPAddress mqttServer;
  mqttServer.fromString(Homie.getConfiguration().mqtt.server.host);
  client.setServer(mqttServer, Homie.getConfiguration().mqtt.server.port);

  // Subscribe to our nodes' topics
  client.setCallback(restoreStateCallback);
  if (client.connect("restoreState", Homie.getConfiguration().mqtt.username, Homie.getConfiguration().mqtt.password)) {
    // Power switch node
      switchPowerNodeTopic = getNodeTopic(switchPowerNode);
      subscribeRestoreTopic(switchPowerNodeTopic);

    // Circulator switch node
      switchCirculatorNodeTopic = getNodeTopic(switchCirculatorNode);
      subscribeRestoreTopic(switchCirculatorNodeTopic);

    // Setup for restoration is done even if we had errors, they are raised during subscriptions
      isRestorationSetup = true;
  } else {
    Homie.getLogger() << "✖ Client not connected to MQTT, unable to define topics for restoration" << endl;
    isRestorationSetup = false;
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Homie loop Handler
void loopHandler() {
  restoreState(); // Restore states from MQTT if required
  loopHandlerDS18B20(); // Read temperatures
  screenPrintStates(); // Print to OLED screen
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Homie Setup Handler
void setupHandler() {
  // Hardware part
    // DS18B20
      dtsensors.begin();
      Homie.getLogger() << "DS18B20 OneWire bus on pin " << ONE_WIRE_BUS << endl;
      Homie.getLogger() << "Found " << dtsensors.getDeviceCount() << " device(s) on bus" << endl;
    // Relays
      pinMode(RELAY_PIN_POWER, OUTPUT);
      pinMode(RELAY_PIN_CIRCULATOR, OUTPUT);
      digitalWrite(RELAY_PIN_POWER, RELAY_STATE_OFF);
      digitalWrite(RELAY_PIN_CIRCULATOR, RELAY_STATE_OFF);
    // OLED Screen
      screenInit();
      screenPrintHeaders();

  // Homie nodes
    // DS18B20 sensor
      temperatureWaterInNode.setProperty("unit").send("c");
      temperatureWaterOutNode.setProperty("unit").send("c");
      temperatureWaterInNode.advertise("unit");
      temperatureWaterInNode.advertise("degrees");
      temperatureWaterOutNode.advertise("unit");
      temperatureWaterOutNode.advertise("degrees");

      //temperatureRangeNode; // TODOs : Convert named/indexed sensors to array for dynamic mapping at MQTT/Home Assistant

    // Relays
      switchPowerNode.advertise("on").settable(switchPowerOnHandler);
      switchCirculatorNode.advertise("on").settable(switchCirculatorOnHandler);

      //switchRangeNode; // TODOs : Convert named/indexed switches to array for dynamic mapping at MQTT/Home Assistant
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino Setup
void setup() {
  Serial.begin(115200); // Required to enable serial output

  Homie_setFirmware(FW_NAME, FW_VERSION);
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
  Homie.setup();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino Loop
void loop() {
  Homie.loop();
}
