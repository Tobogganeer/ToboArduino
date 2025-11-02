/*

Display speed and gear on windshield
Potential for shift indicator (have accel pedal, but engine load would be useful)
ESP8266

Segments

   A
   _
 F| |B
  G-
 E|_|C .
   D   DP


*/

#include "LedControl.h"
#include "CarComms.h"

CarComms comms(handleCarData);

// https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
const int mosi = 13;  // D7, GPIO 13, goes to DataIn
const int sclk = 14;  // D5, GPIO 14, goes to CLK
const int cs = 15;    // D8, GPIO 15, goes to LOAD

// ------------- DPABCDEFG
// Remember digits are upside down
const int n_0 = 0b01111110;
const int n_1 = 0b00110000;
const int n_2 = 0b01011011;
const int n_3 = 0b01111001;
const int n_4 = 0b00110101;
const int n_5 = 0b01101101;
const int n_6 = 0b01101111;
const int n_7 = 0b00111000;
const int n_8 = 0b01111111;
const int n_9 = 0b01111101;

const int d_reverse = 0b00000011;
const int d_neutral = 0b00100011;

const int d_dash = 0b00000001;

// guhhhhh whatever
const int white_left = 1;
const int white_middle = 0;
const int white_right = 3;
const int blue_left = 2;
const int blue_middle = 5;
const int blue_right = 4;

int digits[10] = { n_0, n_1, n_2, n_3, n_4, n_5, n_6, n_7, n_8, n_9 };
int digitIndices[6] = { white_left, white_middle, white_right, blue_left, blue_middle, blue_right };

int gears[7] = { d_neutral, n_1, n_2, n_3, n_4, n_5, d_reverse };

const int hundredsDigit = 0;
const int tensDigit = 1;
const int onesDigit = 2;

const int gearDigit = 4;

// Data pin, clock pin, cs pin, num devices (MAX7219 drivers)
LedControl hud = LedControl(mosi, sclk, cs, 1);

void setup()
{
    hud.shutdown(0, false);  // WAKE UP
    hud.setIntensity(0, 8);  // 8 is a medium value
    hud.clearDisplay(0);

    // Blank out digits before data is received
    setDigit(hundredsDigit, d_dash);
    setDigit(tensDigit, d_dash);
    setDigit(onesDigit, d_dash);
    setDigit(gearDigit, d_dash);

    comms.begin();
    // Speed and gear
    comms.receiveTypeMask = CarDataType::ID_CARINFO | CarDataType::ID_GEAR;
}

void handleCarData(CarDataType type, const uint8_t* data, int len)
{
    if (type == CarDataType::ID_CARINFO)
    {
        CarInfoMsg* info = (CarInfoMsg*)data;
        setSpeed(info->speed);
        hud.setIntensity(0, info->lightsOn ? 1 : 8);
    }
    else if (type == CarDataType::ID_GEAR)
    {
        GearMsg* gear = (GearMsg*)data;
        setGear(gearDigit, gear->gear);
    }
}

void loop() {}

void setSpeed(uint8_t speed)
{
    int hundreds = (speed / 100) % 10;
    int tens = (speed / 10) % 10;
    int ones = speed % 10;

    setDigit(hundredsDigit, hundreds > 0 ? digits[hundreds] : 0);        // '0' character would be blank (all bits zero)
    setDigit(tensDigit, (tens > 0 || hundreds > 0) ? digits[tens] : 0);  // Make sure numbers like 101 will display
    setDigit(onesDigit, digits[ones]);                                   // Display a 0 if the total speed is zero
}

void setGear(int digit, uint8_t gear)
{
    int character = gears[gear];  // Gear is -1-5, array is 0-6
    setDigit(digit, character);
}

void setDigit(int digit, int character)
{
    hud.setRow(0, digitIndices[digit], character);
}

// Reference for now

// /*
//          * Gets the number of devices attached to this LedControl.
//          * Returns :
//          * int	the number of devices on this LedControl
//          */
//         int getDeviceCount();

//         /*
//          * Set the shutdown (power saving) mode for the device
//          * Params :
//          * addr	The address of the display to control
//          * status	If true the device goes into power-down mode. Set to false
//          *		for normal operation.
//          */
//         void shutdown(int addr, bool status);

//         /*
//          * Set the number of digits (or rows) to be displayed.
//          * See datasheet for sideeffects of the scanlimit on the brightness
//          * of the display.
//          * Params :
//          * addr	address of the display to control
//          * limit	number of digits to be displayed (1..8)
//          */
//         void setScanLimit(int addr, int limit);

//         /*
//          * Set the brightness of the display.
//          * Params:
//          * addr		the address of the display to control
//          * intensity	the brightness of the display. (0..15)
//          */
//         void setIntensity(int addr, int intensity);

//         /*
//          * Switch all Leds on the display off.
//          * Params:
//          * addr	address of the display to control
//          */
//         void clearDisplay(int addr);

//         /*
//          * Set the status of a single Led.
//          * Params :
//          * addr	address of the display
//          * row	the row of the Led (0..7)
//          * col	the column of the Led (0..7)
//          * state	If true the led is switched on,
//          *		if false it is switched off
//          */
//         void setLed(int addr, int row, int col, boolean state);

//         /*
//          * Set all 8 Led's in a row to a new state
//          * Params:
//          * addr	address of the display
//          * row	row which is to be set (0..7)
//          * value	each bit set to 1 will light up the
//          *		corresponding Led.
//          */
//         void setRow(int addr, int row, byte value);

//         /*
//          * Set all 8 Led's in a column to a new state
//          * Params:
//          * addr	address of the display
//          * col	column which is to be set (0..7)
//          * value	each bit set to 1 will light up the
//          *		corresponding Led.
//          */
//         void setColumn(int addr, int col, byte value);

//         /*
//          * Display a hexadecimal digit on a 7-Segment Display
//          * Params:
//          * addr	address of the display
//          * digit	the position of the digit on the display (0..7)
//          * value	the value to be displayed. (0x00..0x0F)
//          * dp	sets the decimal point.
//          */
//         void setDigit(int addr, int digit, byte value, boolean dp);

//         /*
//          * Display a character on a 7-Segment display.
//          * There are only a few characters that make sense here :
//          *	'0','1','2','3','4','5','6','7','8','9','0',
//          *  'A','b','c','d','E','F','H','L','P',
//          *  '.','-','_',' '
//          * Params:
//          * addr	address of the display
//          * digit	the position of the character on the display (0..7)
//          * value	the character to be displayed.
//          * dp	sets the decimal point.
//          */
//         void setChar(int addr, int digit, char value, boolean dp);