
#include <Arduino.h>

#define IR_RECEIVE_PIN 2
#include <IRremote.hpp>


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


void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;  // Wait for Serial to become available. Is optimized away for some cores.

  // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
  IrReceiver.begin(IR_RECEIVE_PIN, false);
}

void loop() {
  if (IrReceiver.decode()) {

    /*
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
      Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
      // We have an unknown protocol here, print extended info
      IrReceiver.printIRResultRawFormatted(&Serial, true);
      IrReceiver.resume();  // Do it here, to preserve raw data for printing with printIRResultRawFormatted()
    } else {
      IrReceiver.resume();  // Early enable receiving of the next IR frame
      IrReceiver.printIRResultShort(&Serial);
      IrReceiver.printIRSendUsage(&Serial);
    }
    Serial.println();
    */

    IrReceiver.resume();

    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)
    {
      Serial.println("Repeat");
    }

    if (IrReceiver.decodedIRData.command == POWER) {
      Serial.println("Power");
    } else if (IrReceiver.decodedIRData.command == HOME) {
      // do something else
    }
  }
}
