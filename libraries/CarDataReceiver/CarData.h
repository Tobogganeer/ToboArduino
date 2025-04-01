#ifndef CARDATA_H
#define CARDATA_H

#include <Arduino.h>

typedef enum: uint8_t {ID_BTSKIP, ID_CARINFO} CarDataID;

typedef struct CarData {
	CarDataID id;

	// === BLUETOOTH ===
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
} CarData;

#endif