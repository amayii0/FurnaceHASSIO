// Software specifications
  #define FW_NAME                 "D1Mini-Furnace"
  #define FW_VERSION              "0.20.5.7a"

// DS18B20 setup
  #define ONE_WIRE_BUS            D7 // Pin used for OneWire protocol
  #define DS18B20_INDEX_WATERIN   0
  #define DS18B20_INDEX_WATEROUT  1

// Relay board setup
  // Pinouts
    #define RELAY_PIN_POWER         D5
    #define RELAY_PIN_CIRCULATOR    D6
    #define RELAY_STATE_ON          0
    #define RELAY_STATE_OFF         1

  // Default states
    //#define RELAY_MODE_NO           1
    //#define RELAY_MODE_NC           0
    //#define RELAY_MODE_POWER        RELAY_MODE_NC // Default state = Power ON, connected through NC 
    //#define RELAY_MODE_CIRCULATOR   RELAY_MODE_NO // Default state = Circulator OFF, kept open circuit through NO

// I2C OLED Screen : 128*64
  #define OLED_PIN_SCL            D2   // SCL pin : Mandatory
  #define OLED_PIN_SDA            D1   // SDA pin : Mandatory
  // TODO, 2020-04-23, RDU: Reassign properly, currently this uses a hack by modifying the pins_arduino.h

  #define SCREEN_WIDTH            128  // OLED display width, in pixels
  #define SCREEN_HEIGHT           64   // OLED display height, in pixels
  #define OLED_RESET              -1   // Our screen has 4 pins : GND, VCC, SCL, SDA
  #define OLED_I2C_ADDRESS        0x3C // I2C adress for the screen, might differ from silkscreen printout

// General settings
  #define LOOP_INTERVAL           10 // How often to execute loop (seconds)
