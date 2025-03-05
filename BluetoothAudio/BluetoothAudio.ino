#include <btAudio.h>

// Sets the name of the audio device
btAudio audio = btAudio("Quandale Dingle's Whip");

void setup() {

  // Streams audio data to the ESP32
  audio.begin();

  // Re-connects to last connected device
  //audio.reconnect();

  int ws = 16;
  int dout = 17;
  int bck = 5;
  audio.I2S(bck, dout, ws);
  audio.volume(1.0);
}

void loop() {
}
