#include <btAudio.h>
#include <Wire.h>
#include <U8g2lib.h>

// Sets the name of the audio device
btAudio audio = btAudio("Quandale Dingle's Whip");

// Temp code just to test if all libs will fit into memory
// https://www.youtube.com/watch?v=QixtxaAda18
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

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

    // Temp
    u8g2.begin();
}

void loop()
{
}
