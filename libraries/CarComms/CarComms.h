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
        static CarComms* instance; // Icky and kinda stupid
        // I want to keep it as a class though as that is Arduino-library stype

        void (*_internalRecvCallback) (CarDataType type, const uint8_t* data, int len);

        #ifndef ARDUINO_ARCH_ESP8266
        static void OnDataReceivedStatic(const esp_now_recv_info* info, const uint8_t* incomingData, int len);
        #else
        static void OnDataReceivedStatic(uint8_t* mac, uint8_t* incomingData, uint8_t len);
        #endif
        void OnDataReceived(const uint8_t* incomingData, uint8_t len);

    public:
        uint8_t receiveTypeMask = 0xFF; // Restricts what types of message we receive (CarDataType)

        CarComms(void (*recvCallback) (CarDataType type, const uint8_t* data, int len));
        void begin();
        bool send(CarDataType type, const uint8_t* data, int len);
        uint32_t getLastReceiveTimeMS();
        uint32_t getTimeSinceLastReceiveMS(); // Returns -1 if no message has been received
        uint32_t getLastReceiveTimeMS(CarDataType messageType);
        uint32_t getTimeSinceLastReceiveMS(CarDataType messageType); // Returns -1 if no message has been received
        void setReceiveTypeMask(uint8_t mask) { receiveTypeMask = mask; }
};

#endif // ifndef CARCOMMS_H
