// Software specifications
  #define FW_NAME    "D1Mini-Furnace"
  #define FW_VERSION "0.19.1.29a"

// DS18B20 setup
  #define ONE_WIRE_BUS           D2 // Data wire is plugged into port 2 on the Arduino
  #define DS18B20_INDEX_WATERIN  0
  #define DS18B20_INDEX_WATEROUT 1

// Relay board setup
  #define RELAY_PIN_POWER       D0
  #define RELAY_PIN_CIRCULATOR  D1
  #define RELAY_STATE_ON        0
  #define RELAY_STATE_OFF       1

// General settings
  #define LOOP_INTERVAL 10 // How often to execute loop (seconds)
