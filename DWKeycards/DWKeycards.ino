
/*

Code for Design Week newspaper
Uses example MFRC522 code

Unused at the moment, may be later VVV
------- my Pyduino library for serial communication
-------https://github.com/Tobogganeer/Pyduino

ESP32-C3 pins:
static const uint8_t SS = 7;
static const uint8_t MOSI = 6;
static const uint8_t MISO = 5;
static const uint8_t SCK = 4;

SS (7) => RFID SDA
MOSI (6) => RFID MOSI
MISO (5) => RFID MISO
SCK (4) => RFID SCK
3 => RFID RST/Reset

*/

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
//#include <Pyduino.h>

#define RST_PIN 3
#define SS_PIN 7

#define RESPONSE_BYTE_1 16
#define RESPONSE_BYTE_2 32

MFRC522 mfrc522(SS_PIN, RST_PIN);  // RFID reader
const int delayBetweenCards = 1000;

void setup() {
  //Pyduino::Init(); // Serial for Unity
  //Pyduino::CustomMessageCallback = UnityMessage;
  Serial.begin(115200);

  while (!Serial); // Make sure serial port is opened


  // int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1
  SPI.begin(4, 5, 6, 7); // SPI bus for rfid reader
  mfrc522.PCD_Init();
}

void loop() {
  //Pyduino::Tick(); // Check for any communications from Unity (will there even be any? Kinda a one way thing...)

  // Wait if there is no card
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  // Send UID of card
  byte response[6] = { RESPONSE_BYTE_1, RESPONSE_BYTE_2, 0, 0, 0, 0 };

  //mfrc522.uid.size;
  // Only read the first 4 bytes
  for (byte i = 0; i < 4; i++) {
    response[i + 2] = mfrc522.uid.uidByte[i];

  // Send bytes
  Serial.flush();
  if (Serial.write(response, 6) != 6)
    Serial.write(response, 6);
  }

  delay(delayBetweenCards);
}


long UnityMessage(byte* message, int len, int type) {
  /*
  if (type == messageIDImLookingFor)
  {
    int read = 0;
    String str = Pyduino::GetString(message, read);
    long num = Pyduino::GetInt(message, read);
    
    // Do something
  }
  */

  // Return value sent to Unity
  return 0;
}
