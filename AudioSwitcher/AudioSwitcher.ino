/*

Audio Switcher

Outline:
- ESP8266 with a rotary encoder and 2 audio multiplexers (CD4052B)
- Rotary encoder selects audio source
- AUX multiplexer feeds into main multiplexer
- Display audio source on OLED display
- All outputs go through car's AUX port

ESP8266 Wiring:

D3 (GPIO0): Left side rotary encoder
D4 (GPIO2): Right side rotary encoder
GND: Middle rotary encoder

D1 (GPIO5): SCL (I2C)
D2 (GPIO4): SDA (I2C)

D5 (GPIO14): Main audio select A
D6 (GPIO12): Main audio select B
D7 (GPIO13): AUX audio select A
D8 (GPIO15): AUX audio select B

Audio sources:
 - 1: Bluetooth
 - 2: AUX port
 - 3: Lightning Cable
 - 4: USB-C Cable
 - 5: Carputer
 - 6-8: Reserved

Main audio select:
 - Bluetooth
 - AUX
 - Carputer

AUX audio select:
 - AUX port
 - Lightning
 - USB-C

*/

#include "Rotary.h"
#include <U8g2lib.h>
#include <Wire.h>

#define ROTARY_PIN1 2
#define ROTARY_PIN2 0

#define CLICKS_PER_ROTATION 4  // The encoder outputs 4 times when rotated once

Rotary dial;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

bool displayDirty;

enum : byte {
  Bluetooth,
  AuxPort,
  Lightning,
  USB_C,
  Carputer
} audioSource = Bluetooth;

const byte LAST_AUDIOSOURCE = Carputer;  // Used to loop back around



void setup() {
  dial.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_ROTATION);
  dial.setLeftRotationHandler(rotateLeft);
  dial.setRightRotationHandler(rotateRight);

  u8g2.begin();
}

void loop() {
  dial.loop();

  if (displayDirty) {
    updateScreen();
    displayDirty = false;
  }
}

void updateScreen() {
  u8g2.clearBuffer();                   // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr);   // choose a suitable font
  u8g2.drawStr(0, 10, "Hello World!");  // write something to the internal memory
  u8g2.sendBuffer();                    // transfer internal memory to the display
}

void rotateLeft(Rotary& dial) {
  audioSource--;
  if (audioSource < 0)
    audioSource = LAST_AUDIOSOURCE;
  displayDirty = true;
}

void rotateRight(Rotary& dial) {
  audioSource++;
  if (audioSource > LAST_AUDIOSOURCE)
    audioSource = 0;
  displayDirty = true;
}
