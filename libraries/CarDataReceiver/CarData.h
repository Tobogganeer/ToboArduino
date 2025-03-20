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

	// Trip
	uint32_t odometer;
	uint16_t currentRunTime;

	// Fuel
	uint16_t fuelUseLitersPerHour;
	uint8_t fuelLevel;

	// Throttle
	uint8_t throttlePosition;
	uint8_t engineLoad;

	// Temperatures
	int16_t coolantTemp; // This could be a byte, but I want to avoid all conversions needed
	int16_t oilTemp;
	uint16_t oilPressure;
} CarData;

#endif