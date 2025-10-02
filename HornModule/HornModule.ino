#include <DFPlayerMini_Fast.h>
#include <SoftwareSerial.h>

SoftwareSerial swSerial(10, 11);  // RX, TX

DFPlayerMini_Fast mp3;

void setup()
{
    Serial.begin(115200);

    swSerial.begin(9600);
    mp3.begin(swSerial, true); // Debug output is bool parameter

    mp3.volume(10);  // Set volume to max
    mp3.randomAll();
}

void loop()
{
    //mp3.play(random(3));
    //mp3.randomAll();
    //delay(4000);
}
