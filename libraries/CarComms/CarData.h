#ifndef CARDATA_H
#define CARDATA_H

#include <Arduino.h>

typedef enum : uint8_t {
	ID_CARINFO 			= 1 << 0,
	ID_GEAR 			= 1 << 1,
	ID_BT_INFO 			= 1 << 2,
	ID_BT_SKIP 			= 1 << 3,
	ID_OEM_DISPLAY 		= 1 << 4,
	ID_REVERSEPROXIMITY = 1 << 5,
} CarDataType;
// Update the receiveTimes array in CarComms if any are added/removed

typedef enum : uint8_t {
	Neutral,
	First,
	Second,
	Third,
	Fourth,
	Fifth,
	Reverse,
} Gear;

typedef struct CarInfoMsg {
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
} CarInfoMsg;

typedef struct GearMsg {
	Gear gear;
} GearMsg;

typedef struct BTInfoMsg {
	char title[64];
	char artist[64];
	char album[64];
	int length;
	int currentPlayTime;
} BTInfoMsg;

typedef struct BTSkipMsg {
	bool forward;
	bool reverse;
} BTSkipMsg;

typedef struct OEMDisplayMsg {
	char message[14];
} OEMDisplayMsg;

typedef struct ReverseProximityMessage {
	uint16_t leftmostDistance; // Driver side
	uint16_t middleLeftDistance;
	uint16_t middleRightDistance;
	uint16_t rightmostDistance; // Passenger side
} ReverseProximityMessage;

#endif