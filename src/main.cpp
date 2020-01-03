/*********************************************************************************************************************
 ** 2019-01-27, RDU: Rebuild for furnace monitoring and control
 **                  D2 = Data bus (this actually supports multiple sensors using OneWire)
 ** 2019-01-28, RDU: Adding control for 2 relays (Furnace on/off, Circulator on/off) using a 5V relay board
 ** 2019-01-29, RDU: Fool proofing DS18B20 against unread values (-127.00)
 ** 2019-07-30, RDU: Migrated PlatformIO from Atom to VSCode, updated project structure
 ** 2019-08-07, RDU: Updated lib_deps to rely solely on project's deps and not on any global libs, includes use of ArduinoJson 5.x
 **
 ** TODOs
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

    // Measure loop
      unsigned long lastMeasureSent = 0;


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
  if (|isValidSwitchValue(value))
  {
    Homie.getLogger() << "Power switch received an invalid value : " << value << endl;
    return false;
  }

  digitalWrite(RELAY_PIN_POWER, valueToState(value));
  switchPowerNode.setProperty("on").send(valueToPayload(value));
  Homie.getLogger() << "Power switch is " << value << endl;

  return true;
}

// Switch handlers for Circulator Switch
bool switchCirculatorOnHandler(const HomieRange& range, const String& value) {
  if (|isValidSwitchValue(value))
  {
    Homie.getLogger() << "Circulator switch received an invalid value : " << value << endl;
    return false;
  }

  digitalWrite(RELAY_PIN_CIRCULATOR, valueToState(value));
  switchCirculatorNode.setProperty("on").send(valueToPayload(value));
  Homie.getLogger() << "Circulator switch is " << value << endl;

  return true;
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
      temperatureWaterOutNode.setProperty("degrees").send(String(rawTemp));
    }

    // Water in temp
    rawTemp = getTemFromDS18B20(DS18B20_INDEX_WATERIN);
    if (isValidTemperature(rawTemp))
    {
      temperatureWaterInNode.setProperty("degrees").send(String(rawTemp));
    }

    lastMeasureSent = millis();
  }
}

// Homie loop Handler
void loopHandler() {
  loopHandlerDS18B20();
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
