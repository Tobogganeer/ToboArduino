/*

ESP8266 (D1 Mini clone)
Gets data from CAN and LSCAN buses in car and transmits to other modules with ESP-NOW
2010 Mazda 3

Evan Daveikis

*/

#include <CAN.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "CarData.h"
//#include "CarDataReceiver.h"


CarData data;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ESP-NOW test code
// https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
unsigned long lastTime = 0;
unsigned long timerDelay = 2000;  // send readings timer


// From example code
void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  initESPNow();

  return;

  Serial.println("CAN Receiver Test");

  // Normal CAN bus is 500kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1)
      ;
  }
}

void initESPNow() {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  //esp_now_register_send_cb(OnDataSent);

  // Register peer
  //esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}


void loop() {
  if ((millis() - lastTime) > timerDelay) {
    sendCarData();
  }

  return;
  
  // try to parse packet
  int packetSize = CAN.parsePacket();

  if (packetSize) {
    // received a packet
    Serial.print("Received ");

    if (CAN.packetExtended()) {
      Serial.print("extended ");
    }

    if (CAN.packetRtr()) {
      // Remote transmission request, packet contains no data
      Serial.print("RTR ");
    }

    Serial.print("packet with id 0x");
    Serial.print(CAN.packetId(), HEX);

    if (CAN.packetRtr()) {
      Serial.print(" and requested length ");
      Serial.println(CAN.packetDlc());
    } else {
      Serial.print(" and length ");
      Serial.println(packetSize);

      // only print packet data for non-RTR packets
      while (CAN.available()) {
        Serial.print((char)CAN.read());
      }
      Serial.println();
    }

    Serial.println();
  }
}

void sendCarData()
{
  data.rpm = random(300, 700);
  data.speed = 100;

  esp_now_send(broadcastAddress, (uint8_t *) &data, sizeof(data));
  Serial.print("Sent data. RPM: ");
  Serial.print(data.rpm);
  Serial.print(", Speed:");
  Serial.println(data.speed);

  lastTime = millis();
}