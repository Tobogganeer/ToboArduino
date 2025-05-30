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

//#define DEBUG_LOG

CarData data;
uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Sending data over ESP-NOW
unsigned long lastDataSendTime = 0;
unsigned long dataSendInterval = 100;  // ms

// CAN
MCP_CAN HSCAN(0);  // Normal CAN bus, CS is pin 3 (GPIO0)
MCP_CAN MSCAN(2);  // MS/LS CAN bus, CS is pin 4 (GPIO2)

#define HSCAN_INT 5  // Interrupt pin for normal CAN
#define MSCAN_INT 4  // Interrupt pin for MS/LS CAN

unsigned long rxId;
byte len;
byte rxBuf[8];
byte txBuf[8];

#ifdef DEBUG_LOG
char msgString[128];
#endif


// Convertors from bytes to values
union Convertor
{
    int32_t intVal;
    uint32_t uintVal;
    int16_t shortVal;
    uint16_t ushortVal;

    byte bytes[4];
};



void setup()
{
    // Start serial bus (not needed for final build)
    Serial.begin(115200);

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

    // Init HSCAN bus, baudrate: 500k@8MHz
    if (HSCAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
    {
        Serial.println("HSCAN initialized");
        HSCAN.setMode(MCP_NORMAL);
    }
    else Serial.println("HSCAN init fail!");

    // Init MSCAN bus, baudrate: 125k@8MHz
    if (MSCAN.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK)
    {
        Serial.println("MSCAN initialized");
        MSCAN.setMode(MCP_NORMAL);
    }
    else Serial.println("MSCAN init fail!");

    SPI.setClockDivider(SPI_CLOCK_DIV2);  // Set SPI to run at 8MHz (16MHz / 2 = 8 MHz)
}




void loop()
{
    if ((millis() - lastDataSendTime) > dataSendInterval)
        sendCarData();

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

void sendCANMessage(MCP_CAN bus, unsigned long id)
{
    // id, extended, length, data
    bus.sendMsgBuf(id, 0, 8, txBuf);
}

/*

bool bt_forward;
	bool bt_reverse;
	bool bt_pause;

	// === CAR INFO ===
	// Speed
	uint16_t rpm;
	uint8_t speed;
	uint8_t throttlePosition;
	uint8_t engineLoad;

	// Trip
	uint32_t odometer;
	uint16_t currentRunTime;

	// Transmission/mechanical
	bool handbrakeOn;
	bool reversing;
	bool clutchDepressed;
    bool brakePressed;
	uint8_t gear;
	float steeringAngle;

	// Fuel
	float fuelEcoInst;
	float fuelEcoAvg;
	uint16_t kmRemaining;
	uint8_t fuelLevel;

	// Temperatures
	int16_t coolantTemp; // This could be a byte, but I want to avoid all conversions needed
	int16_t oilTemp;
	uint16_t oilPressure;

	// Doors
	bool door_frontDriverOpen;
	bool door_frontPassengerOpen;
	bool door_rearDriverOpen;
	bool door_rearPassengerOpen;
	bool door_hatchOpen;

*/

void handleHSMessage()
{
#ifdef DEBUG_LOG
    sprintf(msgString, "HS,0x%.2X,%d,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X", rxId, len, rxBuf[7], rxBuf[6], rxBuf[5], rxBuf[4], rxBuf[3], rxBuf[2], rxBuf[1], rxBuf[0]);
    Serial.println(msgString);
    return;
#endif

    // rxId, rxBuf
    if (rxId == 0x190)
    {
        Serial.print("Throttle: ");
        Serial.println(convert(rxBuf[1], rxBuf[0]).ushortVal);

        // B1-2: Throttle
        // B3-b7: Brakes on
        // B3-b5: Clutch on
        data.brakePressed = (rxBuf[2] & 0b01000000) > 0;

        Serial.print("Brake: ");
        Serial.println(rxBuf[2] & 0b01000000 > 0);
    }
    if (rxId == 0x201)
    {
        Serial.print("RPM: ");
        Serial.println(convert(rxBuf[1], rxBuf[0]).ushortVal);
        // B1-2: RPM,
        // B3-4: Engine torque?
        // B5-6: Vehicle speed,
        // B7: Accelerator pedal

        Serial.print("Speed: ");
        Serial.println(convert(rxBuf[5], rxBuf[4]).ushortVal / 100.0f);

        Serial.print("Accel Pedal: ");
        Serial.println(rxBuf[6] / 2.0f);
    }
    if (rxId == 0x420)
    {
        // B1: Engine oil temperature?
        // Probably actually coolant temp - (B1 - 40 = degrees celsius)
        // B2: Distance (0,2m resolution – resets every 51m)
        // B3: Fuel consumption (cumulative)
    }
    if (rxId == 0x430)
    {
        Serial.print("Fuel level: ");
        Serial.println((rxBuf[0] * 0.25f) / 55.0f * 100.0f);
        // B1: Fuel level. 1 unit = 0,25l - 60.25L total (241 steps)
        // B2: Fuel tank sensor (?)
    }
    if (rxId == 0x433)
    {
        Serial.print("Doors: ");
        printBits(rxBuf[0]);
        Serial.println();
        // B1: Doors. Ex. front left door open: 0x80, trunk open: 0x08
        // B4: bit1 = hand brake, bit2 = reverse gear

        Serial.print("Hand brake: ");
        Serial.println(rxBuf[3] & 0b00000001 > 0);
        Serial.print("Reverse: ");
        Serial.println(rxBuf[3] & 0b00000010 > 0);
        printBits(rxBuf[3]);
        Serial.println();
    }
    if (rxId == 0x4DA)
    {
        // B1-2: Steering angle. Values from about 0x6958 (all way left) to about 0x96A8 all way right).
        // Can have offset, because when turning on dashboard it always resets to zero, regardless of the effective steering angle position.
    }
    if (rxId == 0x4F2)
    {
        Serial.print("Odometer: ");
        Serial.println(convert(rxBuf[2], rxBuf[1]).ushortVal);
        // B2-3: Odometer
        // Might be first byte too to get the range
    }
}


void handleMSMessage()
{
#ifdef DEBUG_LOG
    sprintf(msgString, "MS,0x%.2X,%d,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X,0x%.2X", rxId, len, rxBuf[7], rxBuf[6], rxBuf[5], rxBuf[4], rxBuf[3], rxBuf[2], rxBuf[1], rxBuf[0]);
    Serial.println(msgString);
    return;
#endif

    // rxId, rxBuf

    if (rxId == 0x290)
    {
        // B2-8: First half of display, ASCII
    }
    if (rxId == 0x291)
    {
        // B2-8: Second half of display, ASCII
    }
    if (rxId == 0x400)
    {
        Serial.print("Inst fuel consump: ");
        uint16_t inst = convert(rxBuf[3], rxBuf[2]).ushortVal;
        if (inst == 0xFFF3)
            Serial.println("---");
        else
            Serial.println(inst / 10.0f);

        Serial.print("Avg fuel consump: ");
        Serial.println(convert(rxBuf[5], rxBuf[4]).ushortVal / 100.0f);

        Serial.print("Dist remaining (km): ");
        Serial.println(convert(rxBuf[7], rxBuf[6]).ushortVal);
        // B1-2: Average speed (km/h). Might be one byte only
        // B3-4: Inst. fuel consumption L/100km (x/10, FFF3 = ---)
        // B5-6: Avg. fuel consumption L/100km (x/10). May be one byte only
        // B7-8: Distance in KM remaining
    }
    if (rxId == 0x420)
    {
        Serial.print("Coolant temp: ");
        Serial.println(rxBuf[0] - 40);
        // B1: Engine coolant temperature (B1 - 40 = degrees celsius)
    }
    if (rxId == 0x28F)
    {
        // LCD Display
        /*
        B1-b8: always 1
        B1-b7: “CD IN” symbol
        B1-b6: “MD IN” symbol
        B1-b5: “ST” symbol
        B1-b4: Dolby symbol
        B1-b3: “RPT” symbol
        B1-b2: “RDM” symbol
        B1-b1: “AF” symbol

        B2-b8: “PTY” symbol
        B2-b7: “TA” symbol
        B2-b6: “TP” symbol
        B2-b5: “AUTO-M” symbol
        B2-b4-1: Always 0

        B3: 0x00

        B4-b8-7: 0
        B4-b6: Symbol “:” between 3rd and 4th character
        B4-b5: Symbol “ ‘ ” between 11th and 12th character
        B4-b4: 0
        B4-b3: Symbol “.” between 11th and 12th character
        B4-b2: Symbol “.” between 10th and 11th character
        B4-b1: 1 when turing on radio

        B5-b8: Changes when clicking buttons or rotating knobs
        B5-b7: 0 fixed
        B5-b6: 1 fixed
        B5-b5: 1 = “Clock” button
        B5-b4: 1 = “Info” button (trip computer)
        B5-b3: Always 0
        B5-b2: Send at least 3 times 1 to enable LCD test (all symbols showed)
        B5-b1: Always 0

        B6-8: Always 0x000000
        */
    }
}


void sendCarData()
{
    esp_now_send(broadcastAddress, (uint8_t *)&data, sizeof(data));

    lastDataSendTime = millis();
}









// ====================================================== UTIL ======================================================

Convertor convert(byte b1, byte b2)
{
    Convertor conv;
    conv.bytes[0] = b1;
    conv.bytes[1] = b2;
    return conv;
}

Convertor convert(byte b1, byte b2, byte b3, byte b4)
{
    Convertor conv;
    conv.bytes[0] = b1;
    conv.bytes[1] = b2;
    conv.bytes[2] = b3;
    conv.bytes[3] = b4;
    return conv;
}

void printBits(byte val)
{
    char msgString[8];
    sprintf(msgString, "%c%c%c%c%c%c%c%c",
            ((val)&0x80 ? '1' : '0'),
            ((val)&0x40 ? '1' : '0'),
            ((val)&0x20 ? '1' : '0'),
            ((val)&0x10 ? '1' : '0'),
            ((val)&0x08 ? '1' : '0'),
            ((val)&0x04 ? '1' : '0'),
            ((val)&0x02 ? '1' : '0'),
            ((val)&0x01 ? '1' : '0'));
    Serial.print(msgString);
}