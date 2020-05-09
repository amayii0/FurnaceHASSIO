// Software specifications
  #define FW_NAME                 "D1Mini-Furnace"
  #define FW_VERSION              "0.20.5.9a"

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

// I2C OLED Screen : 128*64
  #define OLED_PIN_SCL            D2   // SCL pin : Mandatory
  #define OLED_PIN_SDA            D1   // SDA pin : Mandatory

  #define SCREEN_WIDTH            128  // OLED display width, in pixels
  #define SCREEN_HEIGHT           64   // OLED display height, in pixels
  #define OLED_RESET              -1   // Our screen has 4 pins : GND, VCC, SCL, SDA
  #define OLED_I2C_ADDRESS        0x3C // I2C adress for the screen, might differ from silkscreen printout

// General settings
  #define LOOP_INTERVAL           10 // How often to execute loop (seconds)
