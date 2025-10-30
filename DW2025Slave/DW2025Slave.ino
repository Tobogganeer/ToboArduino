/*

Glorified demultiplexer
Read inputs and send them to the main ESP32 board

RX (0) is purple
TX (1) is grey

D2
D3
D4
D5
D6
D7

D8
D9
D10
D11
D12
D13

A0
A1
A2
A3
A4
A5

*/

#include <Keypad.h>

const int redSwitch = ;
const int blueSwitch = ;
const int greenSwitch = ;
const int yellowSwitch = ;

const int joystickForwards = ;
const int joystickBackwards = ;
const int joystickLeft = ;
const int joystickRight = ;

// Keypad
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
    { '1', '2', '3' },
    { '4', '5', '6' },
    { '7', '8', '9' },
    { '*', '0', '#' }
};

// Col 2, Row 1, Col 1, Row 4, Col 3, Row 3, Row 2

byte rowPins[ROWS] = { #, #, #, #};
byte colPins[COLS] = { #, #, #};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup()
{
    Serial.begin(9600);

    pinMode(redSwitch, INPUT_PULLUP);
    pinMode(blueSwitch, INPUT_PULLUP);
    pinMode(greenSwitch, INPUT_PULLUP);
    pinMode(yellowSwitch, INPUT_PULLUP);

    pinMode(joystickForwards, INPUT_PULLUP);
    pinMode(joystickBackwards, INPUT_PULLUP);
    pinMode(joystickLeft, INPUT_PULLUP);
    pinMode(joystickRight, INPUT_PULLUP);
}

void loop()
{
    char key = keypad.getKey();

    bool red = !digitalRead(redSwitch);
    bool blue = !digitalRead(blueSwitch);
    bool green = !digitalRead(greenSwitch);
    bool yellow = !digitalRead(yellowSwitch);

    bool forwards = !digitalRead(joystickForwards);
    bool backwards = !digitalRead(joystickBackwards);
    bool left = !digitalRead(joystickLeft);
    bool right = !digitalRead(joystickRight);

    char out[10] = { key, red, blue, green, yellow, forwards, backwards, left, right, '\n' };

    Serial.write(out, 10);

    delay(20); // 50 times per second should be more than enough?
}
