/*

ESP8266 (D1 Mini clone)
Gets data from CAN and LSCAN buses in car and transmits to other modules with ESP-NOW
2010 Mazda 3

Evan Daveikis

HS: https://docs.google.com/spreadsheets/d/1SKfXAyo6fbAfMUENw1KR3w4Fvx_Ihj6sTPSVXBdOXKk/edit?gid=0#gid=0
MS: https://docs.google.com/spreadsheets/d/1wjpo5WGLxsswjUi0MUDwKySKp3XejPfE5vdeieBFiGY/edit?gid=0#gid=0


PINOUT:
D5 -> SCLK (HSCAN and MSCAN)
D6 -> MISO (HSCAN and MSCAN)
D7 -> MOSI (HSCAN and MSCAN)
D3 -> CS (HSCAN)
D4 -> CS (MSCAN)
D1 -> INT (HSCAN)
D2 -> INT (MSCAN)

*/

#include <mcp_can.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "CarData.h"


CarData data;
uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Sending data over ESP-NOW
unsigned long lastDataSendTime = 0;
unsigned long dataSendInterval = 2000;  // ms

// CAN
MCP_CAN HSCAN(0);  // Normal CAN bus, CS is pin 3 (GPIO0)
MCP_CAN MSCAN(2);  // MS/LS CAN bus, CS is pin 4 (GPIO2)

#define HSCAN_INT 1  // Interrupt pin for normal CAN
#define MSCAN_INT 2  // Interrupt pin for MS/LS CAN

unsigned long rxId;
byte len;
byte rxBuf[8];
byte txBuf[8];

char msgString[128];


void setup()
{
    // Start serial bus (not needed for final build)
    Serial.begin(921600);

    while (!Serial)
        ;


    initESPNow();
    initCAN();
}

// https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
void initESPNow()
{
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != 0)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Set ourselves to be the controller/sender
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
}

void initCAN()
{
    pinMode(HSCAN_INT, INPUT_PULLUP);
    pinMode(MSCAN_INT, INPUT_PULLUP);

    // Init HSCAN bus, baudrate: 500k@16MHz
    if (HSCAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
    {
        Serial.println("HSCAN initialized");
        HSCAN.setMode(MCP_NORMAL);
    }
    else Serial.println("HSCAN init fail!");

    // Init MSCAN bus, baudrate: 250k@16MHz
    if (MSCAN.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK)
    {
        Serial.println("MSCAN initialized");
        MSCAN.setMode(MCP_NORMAL);
    }
    else Serial.println("MSCAN init fail!");

    SPI.setClockDivider(SPI_CLOCK_DIV2);  // Set SPI to run at 8MHz (16MHz / 2 = 8 MHz)

    //HSCAN.sendMsgBuf(0x1000000, 1, 8, txBuf0);
}


void loop()
{
    if ((millis() - lastDataSendTime) > dataSendInterval)
        sendCarData();

    readCAN();

    delay(1);
}

void readCAN()
{
    
    // ======================== HS
    if (!digitalRead(HSCAN_INT))
    {  // If interrupt pin is low, read HSCAN receive buffer
        
        //Serial.println("HSCAN receive buffer:");
        HSCAN.readMsgBuf(&rxId, &len, rxBuf);  // Read data: len = data length, buf = data byte(s)
        if (rxId == 0x201)
        {
            Serial.println("Got RPM");
        }
        //CAN1.sendMsgBuf(rxId, 1, len, rxBuf);  // Immediately send message out CAN1 interface

        /*
        if ((rxId & 0x80000000) == 0x80000000)  // Determine if ID is standard (11 bits) or extended (29 bits)
            sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
        else
            sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);

        Serial.print(msgString);

        if ((rxId & 0x40000000) == 0x40000000)
        {  // Determine if message is a remote request frame.
            sprintf(msgString, " REMOTE REQUEST FRAME");
            Serial.print(msgString);
        }
        else
        {
            for (byte i = 0; i < len; i++)
            {
                sprintf(msgString, " 0x%.2X", rxBuf[i]);
                Serial.print(msgString);
            }
        }

        Serial.println();
        */
    }

    // ======================== MS
    if (!digitalRead(MSCAN_INT))
    {  // If interrupt pin is low, read MSCAN receive buffer
    /*
        Serial.println("MSCAN receive buffer:");
        MSCAN.readMsgBuf(&rxId, &len, rxBuf);  // Read data: len = data length, buf = data byte(s)
        //CAN0.sendMsgBuf(rxId, 1, len, rxBuf);  // Immediately send message out CAN0 interface

        if ((rxId & 0x80000000) == 0x80000000)  // Determine if ID is standard (11 bits) or extended (29 bits)
            sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
        else
            sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);

        Serial.print(msgString);

        if ((rxId & 0x40000000) == 0x40000000)
        {  // Determine if message is a remote request frame.
            sprintf(msgString, " REMOTE REQUEST FRAME");
            Serial.print(msgString);
        }
        else
        {
            for (byte i = 0; i < len; i++)
            {
                sprintf(msgString, " 0x%.2X", rxBuf[i]);
                Serial.print(msgString);
            }
        }

        Serial.println();
        */
    }
}

void sendCarData()
{
    data.rpm = random(300, 700);
    data.speed = 100;

    esp_now_send(broadcastAddress, (uint8_t *)&data, sizeof(data));
    
    Serial.print("Sent data. RPM: ");
    Serial.print(data.rpm);
    Serial.print(", Speed:");
    Serial.println(data.speed);
    

    lastDataSendTime = millis();
}