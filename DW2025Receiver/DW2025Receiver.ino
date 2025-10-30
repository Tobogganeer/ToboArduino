#include <ESP8266WiFi.h>
#include <espnow.h>

// ESP8266
// Just relays data from instrument to computer

// From Tobo CarComms
#define ESP_NOW_CHANNEL 4
#define CHECK_BYTE 0xFC

uint8_t instrumentAddress[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t dataWithCheckByte[250] = { 0 };

const int inputSize = ##;

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    // TODO: Read serial and send display data back
    // put your main code here, to run repeatedly:
}

void recv(uint8_t* mac, uint8_t* incomingData, uint8_t len)
{
    // Input + check byte
    if (len < inputSize + 1)
        return;

    if (incomingData[0] != CHECK_BYTE)
        return;

    // Send data straight to serial
    Serial.write(&incomingData[1], len - 1);
}
