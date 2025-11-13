const int buzzer = 11;
const int leftPot = A0;
const int rightPot = A1;

#include <Keypad.h>

const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
    { 1, 5, 9, 13 },
    { 2, 6, 10, 14 },
    { 3, 7, 11, 15 },
    { 4, 8, 12, 16 }
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

    // Wired them backwards oops
    int leftVal = 1024 - analogRead(leftPot) + 1;
    int rightVal = 1024 - analogRead(rightPot) + 1;

    const int KeyMult = 10;
    const int PotDivisor = 30;

    const int BaseDuration = 15;
    const int BaseDelay = 20;

    const int DurationDivisor = 100;
    const int DelayDivisor = 80;

    const float Variation = 0.9f;

    float time = millis() / (float)rightPot;
    float loop = time % 1000;
    float offset = (sin(loop) * Variation) + 1f;

    int freq = (key * KeyMult) * (leftVal / PotDivisor);
    int duration = BaseDuration * (rightVal / DurationDivisor);
    int delayMS = BaseDelay * (rightVal / DelayDivisor);

    tone(buzzer, freq, duration);
    delay(delayMS);
    noTone(buzzer);
}
