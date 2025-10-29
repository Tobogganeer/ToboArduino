/*

- IO
16x2 display (I2C)
4x switches (with leds)
2x buttons (with leds)
Keypad (3x4)
Keyboard (5 keys/buttons)
Joystick (4 switches)
Pullstart (rotary encoder)
LEDs (various)

- PINS
Display   2
Switches  4 OR 8
Buttons   2 OR 4
Keypad    7
Keyboard  5
Joystick  4
Pullstart 2
LEDs      #
________________
Total     24 min, 30 max + LEDs

- ESP PINS
D13 Blue Button
D12 Red Button
D14 Red Switch
D27 Blue Switch
D26 Green Switch
D25 Yelorange Switch
D33
D32
D35 (input only, no pullup)
D34 (input only, no pullup)
D15
D2
D4
D16
D17
D5
D18
D19
D21 (default SDA) Display
D22 (default SCL) Display
D23
___________
Total 20 pins

NEED A DEMULTIPLEXER

Tools:
- Print better keys
- Pullstart spacer
- Switch panel
- Demultiplexer
- Longer wires (M-M)

*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const int redButton = 25;


void setup()
{
    pinMode(redButton, INPUT_PULLUP);

    lcd.init();  // initialize the lcd
    lcd.init();
    // Print a message to the LCD.
    lcd.backlight();
    lcd.setCursor(3, 0);
    lcd.print("Hello, world!");
    lcd.setCursor(2, 1);
    lcd.print("Group 40!");
}

void loop()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(digitalRead(redButton) ? "Not Pressed" : "Pressed");
    delay(200);
}
