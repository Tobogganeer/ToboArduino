/*

Audio Switcher

Outline:
- ESP8266 with a rotary encoder and 2 audio multiplexers (CD4052B)
- Rotary encoder selects audio source
- AUX multiplexer feeds into main multiplexer
- Display audio source on OLED display
- All outputs go through car's AUX port

ESP8266 Wiring:

D3 (GPIO0): Right side rotary encoder
D4 (GPIO2): Left side rotary encoder
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

#define ROTARY_PIN1 2
#define ROTARY_PIN2 0

#define CLICKS_PER_STEP 4   // this number depends on your rotary encoder
#define MIN_POS         4
#define MAX_POS         20
#define START_POS       10
#define INCREMENT       2   // this number is the counter increment on each step

/////////////////////////////////////////////////////////////////

Rotary r;

/////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  delay(50);

  r.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP, MIN_POS, MAX_POS, START_POS, INCREMENT);
  r.setChangedHandler(rotate);
  r.setLeftRotationHandler(showDirection);
  r.setRightRotationHandler(showDirection);

  Serial.println("\n\nRanged Counter");
  Serial.println("You can only set values between " + String(MIN_POS) + " and " + String(MAX_POS) +".");
  Serial.print("Increment: ");
  Serial.println(r.getIncrement());
  Serial.print("Current position: ");
  Serial.println(r.getPosition());
  Serial.println();
}

void loop() {
  r.loop();
}

/////////////////////////////////////////////////////////////////

// on change
void rotate(Rotary& r) {
   Serial.println(r.getPosition());
}

// on left or right rotation
void showDirection(Rotary& r) {
  Serial.println(r.directionToString(r.getDirection()));
}

/////////////////////////////////////////////////////////////////