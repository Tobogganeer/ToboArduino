#ifndef CARDATARECEIVER_H
#define CARDATARECEIVER_H

/*

Any module that wants to receive data from the car should pull this in
ESP8266/ESP32 modules

*/

#include <Arduino.h>
#include "CarData.h"

// Check if we are running on ESP32
// Most boards will be ESP8266 but the BT module is ESP32
#ifndef ARDUINO_ARCH_ESP8266
#include <esp_now.h>
#include <esp_wifi.h>
#else
#include <ESP8266WiFi.h>
#include <espnow.h>
#endif


CarData _CDR_internal_data;
void (*_CDR_internal_callback) (CarData* data);

#ifndef ARDUINO_ARCH_ESP8266
void OnCarDataReceived(const uint8_t * mac, const uint8_t* incomingData, int len) {
#else
void OnCarDataReceived(uint8_t* mac, uint8_t* incomingData, uint8_t len) {
#endif
    memcpy(&_CDR_internal_data, incomingData, sizeof(CarData));
    _CDR_internal_callback(&_CDR_internal_data);
}

// https://randomnerdtutorials.com/esp-now-esp8266-nodemcu-arduino-ide/
void initCarDataReceiver(void (*dataCallback) (CarData* data))
{
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != 0) {
        Serial.println("Error initializing ESP-NOW for CarDataReceiver");
        return;
    }

    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(OnCarDataReceived);
    _CDR_internal_callback = dataCallback;
}

#endif
