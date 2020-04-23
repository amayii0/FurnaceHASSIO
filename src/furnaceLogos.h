// Original code structure from https://www.instructables.com/id/Fun-With-OLED-Display-and-Arduino/

// Thermometer logo : 16x16
static const unsigned int thermometer_h = 16;
static const unsigned int thermometer_w = 16;
static const unsigned char PROGMEM thermometer_logo[] = {
    B00011100, B00000000,
    B00100010, B01100000,
    B00100110, B10010000,
    B00100010, B10010000,
    B00100110, B01100000,
    B00100010, B00000000,
    B00100110, B00011110,
    B00100010, B00111111,
    B00100110, B01110011,
    B00100010, B01100000,
    B00100110, B01100000,
    B01100011, B01100000,
    B01000001, B01110011,
    B01100011, B00111111,
    B01111111, B00011110,
    B00111110, B00000000
};

// Power button logo : 16x16
static const unsigned int power_h = 16;
static const unsigned int power_w = 16;
static const unsigned char PROGMEM power_logo[] = {
    B00000000, B00000000,
    B00000000, B00000000,
    B00000001, B10000000,
    B00000001, B10000000,
    B00000001, B10000000,
    B00001001, B10010000,
    B00011001, B10011000,
    B00110001, B10001100,
    B00100000, B00000100,
    B01100000, B00000110,
    B01100000, B00000110,
    B00110000, B00001100,
    B00011000, B00011000,
    B00011100, B00111000,
    B00000111, B11100000,
    B00000001, B10000000
};

// Pump logo : 16x16
static const unsigned int circulator_h = 16;
static const unsigned int circulator_w = 16;
static const unsigned char PROGMEM circulator_logo[] = {
    B00000000, B00000000,
    B00011111, B00000000,
    B00010001, B00000000,
    B00011111, B00000000,
    B00001010, B00000000,
    B00001010, B01111100,
    B00001010, B11000110,
    B11101011, B10000011,
    B10111010, B11111001,
    B10101010, B10000001,
    B10111010, B11111001,
    B11101000, B10000001,
    B00001111, B10000011,
    B00000000, B11000110,
    B00000000, B01111100,
    B00000000, B00000000
};

// Small arrow to the right : 9x16
static const unsigned int arrow_r_h = 9;
static const unsigned int arrow_r_w = 16;
static const unsigned char PROGMEM arrow_r_logo[] = {
      B00000000	, B00001000
    , B00000000	, B00001100
    , B00000000	, B00000110
    , B00000000	, B00000011
    , B00000000	, B11111111
    , B00000000	, B00000011
    , B00000000	, B00000110
    , B00000000	, B00001100
    , B00000000	, B00001000
};