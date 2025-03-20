#ifndef CARDATARECEIVER_H
#define CARDATARECEIVER_H

/*

Any module that wants to receive data from the car should pull this in
Made for ESP8266 modules

*/

#include <Arduino.h>
#include "CarData.h"

#include <ESP8266WiFi.h>
#include <espnow.h>

CarData _CDR_internal_data;
void (*_CDR_internal_callback) (CarData* data);


void OnCarDataReceived(uint8_t* mac, uint8_t* incomingData, uint8_t len) {
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
