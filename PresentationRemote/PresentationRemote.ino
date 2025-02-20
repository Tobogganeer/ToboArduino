#include <Arduino.h>
#include <IRremote.hpp>

#include "Keyboard.h"



#define IR_RECEIVE_PIN 2


// Codes for the buttons on this remote
#define POWER 0xDC

#define UP 0xCA
#define DOWN 0xD2
#define LEFT 0x99
#define RIGHT 0xC1

#define OK 0xCE

#define RETURN 0xC5
#define HOME 0x88
#define MENU 0x82

#define V_PLUS 0x80
#define V_NEG 0x81



void setup()
{
  //Serial.begin(115200);
  
  // Start IR and keyboard
  IrReceiver.begin(IR_RECEIVE_PIN, false);
  Keyboard.begin();
}

void Press(uint8_t key)
{
  Keyboard.press(key);
  Keyboard.releaseAll();
}

void loop()
{
  if (IrReceiver.decode())
  {
    // Start next batch (not sure if this is necessary? I think so to avoid duplicate data)
    IrReceiver.resume();

    // Ignore if this is a repeat
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)
      return;

    uint16_t cmd = IrReceiver.decodedIRData.command;

    if (cmd == RIGHT)
      Press(KEY_RIGHT_ARROW);

    else if (cmd == LEFT)
      Press(KEY_LEFT_ARROW);

    else if (cmd == HOME)
      Press(KEY_HOME);

    else if (cmd == OK)
      Press(KEY_F5);

    else if (cmd == POWER)
      Press(KEY_ESC);
  }
}
