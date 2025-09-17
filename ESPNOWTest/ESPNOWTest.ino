// Seems like the BTAudio module isn't receiving anything from the controller - I want to test this

#include "CarComms.h"

CarComms comms(handleCarData);
long timer;

void setup() {
  Serial.begin(115200);

  comms.begin();
  // We only want to receive info and gear. This can be set at any time.
  comms.receiveTypeMask = CarDataType::ID_CARINFO | CarDataType::ID_GEAR;
}

void handleCarData(CarDataType type, const uint8_t* data, int len) {
  for (int i = 0; i < len; i++) {
    Serial.print(data[i]);
  }
  Serial.print('\n');
}

void loop() {
  if ((millis() - timer) > 2000) {
    timer = millis();
    #ifdef ARDUINO_ARCH_ESP8266
    char* msg = "Sent from ESP8266";
    #else
    char* msg = "Sent from ESP32";
    #endif
    comms.send(CarDataType::ID_CARINFO, msg, sizeof(msg));
    Serial.println("Sent message");
  }
}
