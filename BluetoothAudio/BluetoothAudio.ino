#include <btAudio.h>
#include "CarDataReceiver.h"

// Sets the name of the audio device
btAudio audio = btAudio("Quandale Dingle's Whip");

// Temp code just to test if all libs will fit into memory
// https://www.youtube.com/watch?v=QixtxaAda18

void setup()
{
    // Streams audio data to the ESP32
    audio.begin();

    // Re-connects to last connected device
    //audio.reconnect();

    int ws = 16;
    int dout = 17;
    int bck = 5;
    audio.I2S(bck, dout, ws);
    audio.volume(1.0);

    initCarDataReceiver(handleCarData);
}


void handleCarData(CarData* data)
{
    Serial.print("Got data. RPM: ");
    Serial.print(data->rpm);
    Serial.print(", Speed:");
    Serial.println(data->speed);
}


void loop()
{
}
