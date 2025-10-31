/*

ESP32
Gets data from pins and from connected Arduino Uno
Sends them to ESP8266 via ESP-NOW which relays to computer/Unity

- ESP PINS
D13 Red Button
D12 Blue Button
D14
D27 Rotary 1
D26 Rotary 2
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

// Receiver: 84:0D:8E:B7:E8:2C
// cc:db:a7:9a:b1:f8

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "Rotary.h"

#include <esp_now.h>
#include <WiFi.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const int blueButton = 12;
const int redButton = 13;

const int rotary1 = 27;
const int rotary2 = 26;

const int wl = 4;
const int wm = 17;
const int wr = 18;
const int bl = 16;
const int br = 5;

// Uno handles keypad
const int unoRX = 33;
const int unoTX = 32;


HardwareSerial uno(1);
Rotary pullcord = Rotary(rotary1, rotary2, 4);

char unoInputs[9];


const int inputsPerSecond = 50;
const int sendMS = 1000 / inputsPerSecond;
long lastSendTime;

// From Tobo CarComms
#define ESP_NOW_CHANNEL 4
#define CHECK_BYTE 0xFC

// 84:0D:8E:B7:E8:2C
uint8_t receiverAddress[6] = { 0x84, 0x0D, 0x8E, 0xB7, 0xE8, 0x2C };
uint8_t dataWithCheckByte[250] = { 0 };

const int displayDataSize = 2;  // Byte for each health bar

int pullcordValue;


uint8_t healthIcon[8] = {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
};


void setup()
{
    initPins();

    Serial.begin(115200);
    uno.begin(9600, SERIAL_8N1, unoRX, unoTX);

    lcd.init();
    lcd.backlight();
    lcd.createChar(0, healthIcon);

    debugLCDPrint();

    initESPNOW();
}

void initPins()
{
    // TODO: Handle debouncing
    pinMode(blueButton, INPUT);  // Have external pulldown resistors so we can use the LEDs
    pinMode(redButton, INPUT);

    pinMode(wl, INPUT_PULLUP);
    pinMode(wm, INPUT_PULLUP);
    pinMode(wr, INPUT_PULLUP);
    pinMode(bl, INPUT_PULLUP);
    pinMode(br, INPUT_PULLUP);
}

void initPullcord()
{
    pullcord.setLeftRotationHandler(pullcordUpdate);
    pullcord.setRightRotationHandler(pullcordUpdate);
}

void pullcordUpdate(Rotary& cord)
{
    pullcordValue = cord.getPosition();
    //cord.getPosition(); // int
    //cord.getDirection(); // byte
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

    lcd.setCursor(0, 0);
    lcd.print("BOSS: ");
    for (int i = 0; i < 10; i++)
        lcd.write(i < bossHealth ? 0 : '_');

    lcd.setCursor(0, 1);
    lcd.print("YOU:  ");
    for (int i = 0; i < 10; i++)
        lcd.write(i < playerHealth ? 0 : '_');
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
        char rawInputs[9];
        int inputsRead = uno.readBytesUntil('\n', rawInputs, 9);
        // Zero out inputs if we got bad data
        if (inputsRead == 9)
            memcpy(unoInputs, rawInputs, 9);
    }

    pullcord.loop();

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
    uint8_t data[22];

    data[0] = CHECK_BYTE;
    memcpy(&data[1], unoInputs, 9);
    // 0123456789012345
    // cxxxxxxxxx4444bb
    uint32_t pullInt = pullcordValue;
    data[10] = (pullInt >> 24) && 0xFF;
    data[11] = (pullInt >> 16) && 0xFF;
    data[12] = (pullInt >> 8) && 0xFF;
    data[13] = pullInt && 0xFF;
    //memcpy(&data[10], &pullInt, 4);

    data[14] = digitalRead(blueButton);
    data[15] = digitalRead(redButton);

    data[16] = !digitalRead(wl);
    data[17] = !digitalRead(wm);
    data[18] = !digitalRead(wr);
    data[19] = !digitalRead(bl);
    data[20] = !digitalRead(br);
    data[21] = '\n';

    esp_now_send(receiverAddress, data, 22);

    // unoInputs -> 9 bytes

    // pullcordValue -> 4 bytes

    // pinMode(blueButton, INPUT);  // Have external pulldown resistors so we can use the LEDs
    // pinMode(redButton, INPUT);

    // pinMode(wl, INPUT_PULLUP);
    // pinMode(wm, INPUT_PULLUP);
    // pinMode(wr, INPUT_PULLUP);
    // pinMode(bl, INPUT_PULLUP);
    // pinMode(br, INPUT_PULLUP);
}
