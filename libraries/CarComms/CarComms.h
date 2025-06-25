#ifndef CARCOMMS_H
#define CARCOMMS_H

/*

Wrapper for ESP-NOW functions
Any module that wants to send/receive data in the car should pull this in
ESP8266/ESP32 modules

*/

#include <Arduino.h>
#include "CarData.h"

// Check if we are running on ESP32
// Most boards will be ESP8266 but the BT module is ESP32
#ifndef ARDUINO_ARCH_ESP8266
#include <esp_now.h>
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#include <espnow.h>
#endif

#define ESP_NOW_CHANNEL 4
#define CHECK_BYTE 0xFB


class CarComms
{
    private:
        static uint8_t broadcastAddress[6];
        static uint8_t dataWithTypeAndCheckByte[250];
        void (*_internalRecvCallback) (CarDataType type, const uint8_t* data, int len);

        #ifndef ARDUINO_ARCH_ESP8266
        void OnCarDataReceived(const esp_now_recv_info* info, const uint8_t* incomingData, int len);
        #else
        void OnCarDataReceived(uint8_t* mac, uint8_t* incomingData, uint8_t len);
        #endif

    public:
        CarComms(void (*recvCallback) (CarDataType type, const uint8_t* data, int len))
        void begin();
        void send(CarDataType type, const uint8_t* data, int len);
}

#endif // ifndef CARCOMMS_H
