#include "CarComms.h"

uint8_t broadcastAddress[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t dataWithTypeAndCheckByte[250] = {0};
int32_t lastReceiveTimeMS = -1;
int32_t lastReceiveTimes[8] = { -1, -1, -1, -1, -1, -1, -1, -1 }; // One for each message type
CarComms* CarComms::instance = nullptr;


void StoreLastReceiveTime(CarDataType type)
{
    // Types are set up to be flags/masks so aren't an even 0-5
    switch (type)
    {
        case CarDataType::ID_CARINFO:
            lastReceiveTimes[0] = millis();
            break;
        case CarDataType::ID_GEAR:
            lastReceiveTimes[1] = millis();
            break;
        case CarDataType::ID_BT_INFO:
            lastReceiveTimes[2] = millis();
            break;
        case CarDataType::ID_BT_TRACK_UPDATE:
            lastReceiveTimes[3] = millis();
            break;
        case CarDataType::ID_OEM_DISPLAY:
            lastReceiveTimes[4] = millis();
            break;
        case CarDataType::ID_REVERSEPROXIMITY:
            lastReceiveTimes[5] = millis();
            break;
        case CarDataType::ID_BUZZER:
            lastReceiveTimes[6] = millis();
            break;
        case CarDataType::ID_AUDIO_SOURCE:
            lastReceiveTimes[7] = millis();
            break;
    }
}


#ifndef ARDUINO_ARCH_ESP8266
void CarComms::OnDataReceivedStatic(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
#else
void CarComms::OnDataReceivedStatic(uint8_t* mac, uint8_t* incomingData, uint8_t len) {
#endif
    if (instance) {
        instance->OnDataReceived(incomingData, len);
    }
    else
        log_e("Didn't find CarComms instance!");
}

void CarComms::OnDataReceived(const uint8_t* incomingData, uint8_t len) {
    // Needs to have at least the check byte and the type
    if (len < 2)
        return;
    // Make sure this isn't a stray broadcast or anything - only from car electronics
    if (incomingData[0] != CHECK_BYTE)
    {
        log_w("Received ESP-NOW packet with incorrect check byte!");
        return;
    }
    
    // Do we want to receive this message?
    uint8_t type = incomingData[1];
    if ((receiveTypeMask & type) == 0)
        return;

    lastReceiveTimeMS = millis();
    StoreLastReceiveTime((CarDataType)type); // Store that specific receive time

    // Data comes after check and type bytes
    _internalRecvCallback((CarDataType)type, incomingData + 2, len - 2);
}


CarComms::CarComms(void (*recvCallback) (CarDataType type, const uint8_t* data, int len))
{
    _internalRecvCallback = recvCallback;
    CarComms::instance = this;
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

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = ESP_NOW_CHANNEL;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(broadcastAddress)) {
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            log_e("Failed to add broadcast peer");
        }
    }

    // Register receive callback
    #ifdef ARDUINO_ARCH_ESP8266
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    #endif
    esp_now_register_recv_cb(OnDataReceivedStatic);
}

bool CarComms::send(CarDataType type, void* data, int len)
{
    // Max length is 250 and we are adding two bytes
    if (len > 247)
    {
        log_w("Tried sending data over 248 bytes!");
        return false;
    }

    // Prepend data with type and check byte
    memcpy(&dataWithTypeAndCheckByte[2], data, len);
    dataWithTypeAndCheckByte[0] = CHECK_BYTE;
    dataWithTypeAndCheckByte[1] = type;
    // Send it
    esp_err_t status = esp_now_send(broadcastAddress, dataWithTypeAndCheckByte, len + 2);
    if (status != ESP_OK)
        log_e("Error sending ESP_NOW message: %d", status);

    return status == ESP_OK;
}

uint32_t CarComms::getLastReceiveTimeMS()
{
    return lastReceiveTimeMS;
}

uint32_t CarComms::getTimeSinceLastReceiveMS()
{
    return millis() - lastReceiveTimeMS;
}

uint32_t CarComms::getLastReceiveTimeMS(CarDataType type)
{
    switch (type)
    {
        case CarDataType::ID_CARINFO:
            return lastReceiveTimes[0];
        case CarDataType::ID_GEAR:
            return lastReceiveTimes[1];
        case CarDataType::ID_BT_INFO:
            return lastReceiveTimes[2];
        case CarDataType::ID_BT_TRACK_UPDATE:
            return lastReceiveTimes[3];
        case CarDataType::ID_OEM_DISPLAY:
            return lastReceiveTimes[4];
        case CarDataType::ID_REVERSEPROXIMITY:
            return lastReceiveTimes[5];
        case CarDataType::ID_BUZZER:
            return lastReceiveTimes[6];
        case CarDataType::ID_AUDIO_SOURCE:
            return lastReceiveTimes[7];
    }

    return lastReceiveTimeMS;
}

uint32_t CarComms::getTimeSinceLastReceiveMS(CarDataType type)
{
    return millis() - getLastReceiveTimeMS(type);
}