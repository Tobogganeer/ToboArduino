/*

When plotting data in Excel I wasn't getting any MSCAN messages
I just want to make sure I'm getting them

*/

#include <mcp_can.h>
#include <SPI.h>

// CAN
MCP_CAN HSCAN(0);  // Normal CAN bus, CS is pin 3 (GPIO0)
MCP_CAN MSCAN(2);  // MS/LS CAN bus, CS is pin 4 (GPIO2)

#define HSCAN_INT 5  // Interrupt pin for normal CAN
#define MSCAN_INT 4  // Interrupt pin for MS/LS CAN

unsigned long rxId;
byte len;
byte rxBuf[8];
char msgString[128];

void setup()
{
    // Start serial bus (not needed for final build)
    Serial.begin(115200);

    while (!Serial)
        ;

    initCAN();
}

void initCAN()
{
    pinMode(HSCAN_INT, INPUT_PULLUP);
    pinMode(MSCAN_INT, INPUT_PULLUP);

    // Init HSCAN bus, baudrate: 500k@8MHz
    if (HSCAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
    {
        Serial.println("HSCAN initialized");
        HSCAN.setMode(MCP_NORMAL);
    }
    else Serial.println("HSCAN init fail!");

    // Init MSCAN bus, baudrate: 250k@8MHz
    if (MSCAN.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK)
    {
        Serial.println("MSCAN initialized");
        MSCAN.setMode(MCP_NORMAL);
    }
    else Serial.println("MSCAN init fail!");

    SPI.setClockDivider(SPI_CLOCK_DIV2);  // Set SPI to run at 8MHz (16MHz / 2 = 8 MHz)
}

void loop()
{
    readCAN();
}

void readCAN()
{
    // Check for HSCAN data
    if (!digitalRead(HSCAN_INT))
    {
        HSCAN.readMsgBuf(&rxId, &len, rxBuf);  // Read data: len = data length, buf = data byte(s)
        handleHSMessage();
    }

    // Check for MSCAN data
    if (!digitalRead(MSCAN_INT))
    {
        MSCAN.readMsgBuf(&rxId, &len, rxBuf);  // Read data: len = data length, buf = data byte(s)
        handleMSMessage();
    }
}

void handleHSMessage()
{
    sprintf(msgString, "HS 0x%.2X", rxId);
    Serial.println(msgString);
}


void handleMSMessage()
{
    sprintf(msgString, "MS 0x%.2X", rxId);
    Serial.println(msgString);
}