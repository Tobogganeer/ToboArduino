/*

ESP32
Gets data from pins and from connected Arduino Uno
Sends them to ESP8266 via ESP-NOW which relays to computer/Unity

- ESP PINS
D13 Red Button
D12 Blue Button
D14
D27
D26
D25
D33 Uno RX
D32 Uno TX
D35 (input only, no pullup)
D34 (input only, no pullup)
D15
D2
D4 WL (white key, left side)
D16 BL
D17 WM
D5 BR
D18 WR
D19
D21 (default SDA) Display
D22 (default SCL) Display
D23

UNO RX (0) is purple
UNO TX (1) is grey

*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <esp_now.h>
#include <WiFi.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const int blueButton = 12;
const int redButton = 13;

// Uno handles keypad
const int unoRX = 33;
const int unoTX = 32;


HardwareSerial uno(1);

char unoInputs[9];


const int inputsPerSecond = 50;
const int sendMS = 1000 / inputsPerSecond;
long lastSendTime;

// From Tobo CarComms
#define ESP_NOW_CHANNEL 4
#define CHECK_BYTE 0xFC

uint8_t receiverAddress[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t dataWithCheckByte[250] = { 0 };

const int displayDataSize = 2; // Byte for each health bar


void setup()
{
    initPins();

    Serial.begin(115200);
    uno.begin(9600, SERIAL_8N1, unoRX, unoTX);

    lcd.init();
    lcd.backlight();

    debugLCDPrint();

    initESPNOW();
}

void initPins()
{
    // TODO: Handle debouncing
    pinMode(blueButton, INPUT);  // Have external pulldown resistors so we can use the LEDs
    pinMode(redButton, INPUT);
}

void initESPNOW()
{
    WiFi.mode(WIFI_STA);

    WiFi.setChannel(ESP_NOW_CHANNEL);

    // Init ESP-NOW
    if (esp_now_init() != 0)
    {
        Serial.println("Error initializing ESP-NOW for instrument!");
        return;
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverAddress, 6);
    peerInfo.channel = ESP_NOW_CHANNEL;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(receiverAddress))
    {
        if (esp_now_add_peer(&peerInfo) != ESP_OK)
        {
            log_e("Failed to add broadcast peer");
        }
    }

    // Register receive callback
    esp_now_register_recv_cb(recv);
}

void recv(const esp_now_recv_info* info, const uint8_t* incomingData, int len)
{
    // Health bytes + check byte
    if (len < displayDataSize + 1)
        return;

    if (incomingData[0] != CHECK_BYTE)
        return;

    uint8_t playerHealth = incomingData[1];
    uint8_t bossHealth = incomingData[2];

    // TODO: Update display
}

void debugLCDPrint()
{
    lcd.setCursor(3, 0);
    lcd.print("Hello, world!");
    lcd.setCursor(2, 1);
    lcd.print("Group 40!");
}

void loop()
{
    if (uno.available() >= 10)
    {
        int inputsRead = uno.readBytesUntil('\n', unoInputs, 9);
        // Zero out inputs if we got bad data
        if (inputsRead != 9)
            memset(unoInputs, 0, 9);
    }



    // lcd.clear();
    // lcd.setCursor(0, 0);
    // lcd.print(digitalRead(redButton) ? "Not Pressed" : "Pressed");
    // delay(200);

    if (millis() - lastSendTime > sendMS)
    {
        lastSendTime = millis();
        sendInputs();
    }
}

void sendInputs()
{
}
