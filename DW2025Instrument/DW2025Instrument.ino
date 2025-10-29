/*

- IO
16x2 display (I2C)
4x switches (with leds)
2x buttons (with leds)
Keypad (3x4)
Keyboard (5 keys/buttons)
Joystick (4 switches)
Pullstart (rotary encoder)
LEDs (various)

- PINS
Display   2
Switches  4 OR 8
Buttons   2 OR 4
Keypad    7
Keyboard  5
Joystick  4
Pullstart 2
LEDs      #
________________
Total     24 min, 30 max + LEDs

- ESP PINS
D13 Blue Button
D12 Red Button
D14 Red Switch
D27 Blue Switch
D26 Green Switch
D25 Yelorange Switch
D33
D32
D35 (input only, no pullup)
D34 (input only, no pullup)
D15
D2
D4
D16
D17
D5
D18
D19
D21 (default SDA) Display
D22 (default SCL) Display
D23
___________
Total 20 pins

NEED A DEMULTIPLEXER

Tools:
- Print better keys
- Pullstart spacer
- Switch panel
- Demultiplexer
- Longer wires (M-M)

*/

/////////////////////////////////////////////////////////////////

#include "Rotary.h";

/////////////////////////////////////////////////////////////////

#define ROTARY_PIN1	18
#define ROTARY_PIN2	19

/////////////////////////////////////////////////////////////////

Rotary r = Rotary(ROTARY_PIN1, ROTARY_PIN2);

/////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  delay(50);
  Serial.println("\n\nSimple Counter");
  
  r.setChangedHandler(rotate);
  r.setLeftRotationHandler(showDirection);
  r.setRightRotationHandler(showDirection);
}

void loop() {
  r.loop();
}

/////////////////////////////////////////////////////////////////

// on change
void rotate(Rotary& r) {
   Serial.println(r.getPosition());
}

// on left or right rotattion
void showDirection(Rotary& r) {
  Serial.println(r.directionToString(r.getDirection()));
}

/////////////////////////////////////////////////////////////////

