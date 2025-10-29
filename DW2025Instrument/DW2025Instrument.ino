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
D13
D12
D14
D27
D26
D25
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
D21 (default SDA)
D22 (default SCL)
D23
___________
Total 20 pins

NEED A DEMULTIPLEXER

*/

void setup()
{
    // put your setup code here, to run once:
}

void loop()
{
    // put your main code here, to run repeatedly:
}
