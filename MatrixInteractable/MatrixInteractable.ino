#include <DFPlayerMini_Fast.h>
#include <SoftwareSerial.h>

SoftwareSerial swSerial(10, 11);  // RX, TX

DFPlayerMini_Fast mp3;

int success = 13;
int fail = 14;

const int redButton = 5;
const int blueButton = 6;

const int redButtonLED = 3;
const int blueButtonLED = 4;

int lastPlayTime;

void setup()
{
    Serial.begin(115200);

    swSerial.begin(9600);
    mp3.begin(swSerial, true); // Debug output is bool parameter

    mp3.volume(25);  // Set volume to max
    //mp3.randomAll();

    pinMode(redButton, INPUT_PULLUP);
    pinMode(blueButton, INPUT_PULLUP);
  
    pinMode(redButtonLED, OUTPUT);
    pinMode(blueButtonLED, OUTPUT);

    digitalWrite(redButtonLED, HIGH);
    digitalWrite(blueButtonLED, HIGH);

    mp3.normalMode();
}

void loop()
{
    if (millis() - lastPlayTime < 1000)
        return;

    if (digitalRead(redButton) == LOW)
    {
        mp3.play(success);
        lastPlayTime = millis();
    }
    if (digitalRead(blueButton) == LOW)
    {
        mp3.play(fail);
        lastPlayTime = millis();
    }
}
