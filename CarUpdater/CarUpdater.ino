/*

Broadcast updates OTA so I don't need to tear the car apart every time
Evan Daveikis

*/

#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include <CarComms.h>

#define AP_SSID "DaveikisCarUpdater"
#define AP_PASS "update123"
#define OTA_FILE_PATH "/spiffs/firmware.bin"

AsyncWebServer server(80);
CarComms comms(handleCarData);

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    comms.begin();
    comms.receiveTypeMask = 0;  // Don't receive

    // Serve the update file
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS mount failed");
        return;
    }
    server.on("/firmware.bin", HTTP_GET, [](AsyncWebServerRequest* request)
              {
                  request->send(SPIFFS, OTA_FILE_PATH, "application/octet-stream");
              });
    server.begin();

    // TODO: Send out update message
}

void handleCarData(CarDataType type, const uint8_t* data, int len) {}

void loop()
{
    // put your main code here, to run repeatedly:
}
