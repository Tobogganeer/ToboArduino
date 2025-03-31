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
unsigned long dataSendInterval = 100;  // ms

// CAN
MCP_CAN HSCAN(0);  // Normal CAN bus, CS is pin 3 (GPIO0)
MCP_CAN MSCAN(2);  // MS/LS CAN bus, CS is pin 4 (GPIO2)

#define HSCAN_INT 5  // Interrupt pin for normal CAN
#define MSCAN_INT 4  // Interrupt pin for MS/LS CAN

unsigned long rxId;
byte len;
byte rxBuf[8];
//byte txBuf[8];


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
    // rxId, rxBuf
    if (rxId == 0x190)
    {
        // B1-2: Throttle
        // B3-b7: Brakes on
        // B3-b5: Clutch on
    }
    if (rxId == 0x201)
    {
        // B1-2: RPM,
        // B3-4: Engine torque?
        // B5-6: Vehicle speed,
        // B7: Accelerator pedal
    }
    if (rxId == 0x231)
    {
        // B1: Gear:
        //  0x00 = Neutral (vechicle moving), 0x01 = Neutral (vehicle stopped), 0xE1 = Reverse,
        //  0x11 = 1st, 0x20 = 2nd, 0x30 = 3rt, 0x40 = 4th, 0x50 = 5th
        // B7: 80 = In gear, C0 = Out of gear
    }
    if (rxId == 0x420)
    {
        // B1: Engine oil temperature?
        // B2: Distance (0,2m resolution – resets every 51m)
        // B3: Fuel consumption (cumulative)
    }
    if (rxId == 0x430)
    {
        // B1: Fuel level. 1 unit = 0,25l
        // B2: Fuel tank sensor (?)
    }
    if (rxId == 0x433)
    {
        // B1: Doors. Ex. front left door open: 0x80, trunk open: 0x08
        // B4: bit1 = hand brake, bit2 = reverse gear
    }
    if (rxId == 0x4DA)
    {
        // B1-2: Steering angle. Values from about 0x6958 (all way left) to about 0x96A8 all way right).
        // Can have offset, because when turning on dashboard it always resets to zero, regardless of the effective steering angle position.
    }
    /*
    if (rxId == 0x)
    {
        
    }
    */
}


void handleMSMessage()
{
    // rxId, rxBuf
}


void sendCarData()
{
    data.rpm = random(300, 700);
    data.speed = 100;

    esp_now_send(broadcastAddress, (uint8_t *)&data, sizeof(data));

    lastDataSendTime = millis();
}