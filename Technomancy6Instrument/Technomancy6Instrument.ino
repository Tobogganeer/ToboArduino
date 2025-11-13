const int buzzer = 11;
const int leftPot = A0;
const int rightPot = A1;

#include <Keypad.h>

const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
    { '1', '4', '7', '*' },
    { '2', '5', '8', '0' },
    { '3', '6', '9', '#' },
    { 'A', 'B', 'C', 'D' }
};
byte rowPins[ROWS] = { 5, 4, 3, 2 };
byte colPins[COLS] = { 9, 8, 7, 6 };

Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup()
{
    pinMode(buzzer, OUTPUT);
    pinMode(leftPot, INPUT);
    pinMode(rightPot, INPUT);
}

void loop()
{
    char key = 0;
    keypad.getKeys();

    if (keypad.key[0].kstate == PRESSED)
        key = keypad.key[0].kchar;

    int leftVal = analogRead(leftPot);
    int rightVal = analogRead(rightPot);

    tone(buzzer, leftVal * 10, 20);
    delay(25);
    noTone(buzzer);
}
