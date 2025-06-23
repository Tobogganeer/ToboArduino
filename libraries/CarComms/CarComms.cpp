#include "CarComms.h"

#ifndef ARDUINO_ARCH_ESP8266
void CarComms::OnCarDataReceived(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
#else
void CarComms::OnCarDataReceived(uint8_t* mac, uint8_t* incomingData, uint8_t len) {
#endif
    // Needs to have at least the check byte and the type
    if (len < 2)
        return;
    // Make sure this isn't a stray broadcast or anything - only from car electronics
    if (incomingData[0] != CHECK_BYTE)
        return;
    
    // Callback with type (first byte), data (data after first 2 bytes), and length
    _internalRecvCallback((CarDataType)incomingData[1], incomingData + 2, len - 2);
    //_CDR_internal_callback(&_CDR_internal_data);
}


CarComms::CarComms(void (*recvCallback) (CarDataType type, const uint8_t* data, int len))
{
    _internalRecvCallback = recvCallback;
}

void CarComms::begin()
{
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    #ifdef ARDUINO_ARCH_ESP8266
    wifi_set_channel(ESP_NOW_CHANNEL);
    #else
    WiFi.setChannel(ESP_NOW_CHANNEL);
    #endif

    // Init ESP-NOW
    if (esp_now_init() != 0) {
        Serial.println("Error initializing ESP-NOW for CarDataReceiver");
        return;
    }

    // Register receive callback
    #ifdef ARDUINO_ARCH_ESP8266
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    #endif
    esp_now_register_recv_cb(OnCarDataReceived);
}

uint8_t CarComms::broadcastAddress[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t CarComms:dataWithTypeAndCheckByte[250] = {};

bool CarComms::send(CarDataType type, const uint8_t* data, int len)
{
    // Max length is 250 and we are adding two bytes
    if (len > 248)
        return false;

    // Prepend data with type and check byte
    memcpy(dataWithTypeAndCheckByte, data, len);
    dataWithTypeAndCheckByte[0] = CHECK_BYTE;
    dataWithTypeAndCheckByte[1] = type;
    // Send it
    esp_now_send(broadcastAddress, dataWithTypeAndCheckByte, len + 2);
    return true;
}