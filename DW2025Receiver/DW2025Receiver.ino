#include <ESP8266WiFi.h>
#include <espnow.h>

// ESP8266
// Just relays data from instrument to computer
// MAC: 84:0D:8E:B7:E8:2C
// Instrument MAC: cc:db:a7:9a:b1:f8

// From Tobo CarComms
#define ESP_NOW_CHANNEL 4
#define CHECK_BYTE 0xFC

uint8_t instrumentAddress[6] = { 0xCC, 0xDB, 0xA7, 0x9A, 0xB1, 0xF8 };
uint8_t dataWithCheckByte[250] = { 0 };

void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);

    wifi_set_channel(ESP_NOW_CHANNEL);

    // Init ESP-NOW
    if (esp_now_init() != 0)
    {
        Serial.println("Error initializing ESP-NOW for DW receiver!");
        return;
    }

    // "Connect" to instrument
    esp_now_add_peer(instrumentAddress, ESP_NOW_ROLE_COMBO, ESP_NOW_CHANNEL, NULL, 0);

    // Register receive callback
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(recv);
}

void loop()
{
    if (Serial.available())
    {
        uint8_t data[3];
        data[0] = CHECK_BYTE;
        Serial.readBytes(&data[1], 2); // Read player health and boss health into packet
        esp_now_send(instrumentAddress, data, 3);
    }

    // esp_now_send(uint8* mac_address, uint8 data, int len)
}

void recv(uint8_t* mac, uint8_t* incomingData, uint8_t len)
{
    if (incomingData[0] != CHECK_BYTE)
        return;

    // Send data straight to serial
    Serial.write(&incomingData[1], len - 1);
}
