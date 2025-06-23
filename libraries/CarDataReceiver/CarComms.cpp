#include "CarComms.h"

#ifndef ARDUINO_ARCH_ESP8266
void CarComms::OnCarDataReceived(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
#else
void CarComms::OnCarDataReceived(uint8_t* mac, uint8_t* incomingData, uint8_t len) {
#endif
    memcpy(&_CDR_internal_data, incomingData, sizeof(CarData));
    _CDR_internal_callback(&_CDR_internal_data);
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

void CarComms::send(CarDataType type, const uint8_t* data, int len)
{

}