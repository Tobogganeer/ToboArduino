/*

Display speed and gear on windshield
Potential for shift indicator (have accel pedal, but engine load would be useful)
ESP8266

*/

#include "LedControl.h"

// https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
const int mosi = 13;  // D7, GPIO 13, goes to DataIn
const int sclk = 14;  // D5, GPIO 14, goes to CLK
const int cs = 15;    // D8, GPIO 15, goes to LOAD

// Data pin, clock pin, cs pin, num devices (MAX7219 drivers)
LedControl hud = LedControl(mosi, sclk, cs, 1);

void setup()
{
    hud.shutdown(0, false);  // WAKE UP
    hud.setIntensity(0, 8);  // 8 is a medium value
    hud.clearDisplay(0);
}

void loop()
{
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