#include <DFPlayerMini_Fast.h>
#include <SoftwareSerial.h>

SoftwareSerial swSerial(10, 11);  // RX, TX

DFPlayerMini_Fast mp3;

void setup()
{
    Serial.begin(115200);

    swSerial.begin(9600);
    mp3.begin(swSerial, true); // Debug output is bool parameter

    mp3.volume(30);  // Set volume to max
}

void loop()
{
    mp3.play(1);
    delay(2000);
}
