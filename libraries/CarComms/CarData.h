#ifndef CARDATA_H
#define CARDATA_H

#include <Arduino.h>

typedef enum : uint8_t {
	ID_CARINFO 			= 1 << 0,
	ID_GEAR 			= 1 << 1,
	ID_BT_INFO 			= 1 << 2,
	ID_BT_TRACK_UPDATE 	= 1 << 3,
	ID_OEM_DISPLAY 		= 1 << 4,
	ID_REVERSEPROXIMITY = 1 << 5,
	ID_BUZZER			= 1 << 6,
	ID_AUDIO_SOURCE		= 1 << 7,
} CarDataType;
// Update the receiveTimes array in CarComms if any are added/removed

// Constants so we can check for gears 1-5 using ints, then use these
#define GEAR_NEUTRAL 0
#define GEAR_REVERSE 6

typedef enum : uint8_t {
	Neutral,
	First,
	Second,
	Third,
	Fourth,
	Fifth,
	Reverse,
} Gear;

typedef enum : uint8_t {
	BT_INFO_METADATA,
	BT_INFO_DEVICES,
	BT_INFO_CONNECTED,
	BT_INFO_DISCONNECTED,
} BTInfoType;

typedef enum : uint8_t {
	BT_UPDATE_SONG_POS,
	BT_UPDATE_SKIP,
	BT_UPDATE_DEVICE_FAVOURITE,
	BT_UPDATE_DEVICE_DELETE,
	BT_UPDATE_DEVICE_MOVE_UP,
	BT_UPDATE_DEVICE_MOVE_DOWN,
	BT_UPDATE_SET_DISCOVERABLE,
	BT_UPDATE_DEVICE_CONNECT, // These are in TrackUpdate so the main BTAudio can listen for the packets
	BT_UPDATE_DEVICE_DISCONNECT,
	BT_UPDATE_SET_CONNECTABLE,
} BTTrackUpdateType;

typedef enum : uint8_t {
	BT_SONG_POS_UPDATE_PLAY_STATUS_CHANGE,
	BT_SONG_POS_UPDATE_TRACK_CHANGE,
	BT_SONG_POS_UPDATE_PLAY_POS_CHANGED
} BTTrackSongPosUpdateType;

#define BT_SONG_INFO_MAX_STR_LEN 64

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

// Larger chunks of info
typedef struct BTInfoMsg {
	BTInfoType type; // 1 byte

	union {
		struct {
			char title[BT_SONG_INFO_MAX_STR_LEN]; // 64 bytes
			char artist[BT_SONG_INFO_MAX_STR_LEN]; // 128
			char album[BT_SONG_INFO_MAX_STR_LEN]; // 192
			uint32_t trackLengthMS; // 196
			uint8_t playStatus; // 197
		} songInfo;
		struct {
			// Extracting constants from btAudio.h
			// Change these if those change (this is lazy but I don't really care)
			uint8_t addresses[5][6]; // 5 x 6 = 30 bytes
			char deviceNames[5][32]; // 5 x 32 = 160
			uint8_t count; // 161
			uint8_t favourite; // 162
			uint8_t connected[6]; // 168
			bool reconnecting;
		} devices;
		struct {
			uint8_t address[6]; // Used for connected/disconnected
			char deviceName[32];
		} sourceDevice;
	};
	
} BTInfoMsg;

// Smaller updates
typedef struct BTTrackUpdateMsg {
	BTTrackUpdateType type;

	union
	{
		uint8_t device[6]; // For favouriting, deleting, connecting, etc
		bool discoverable;
		bool connectable;
		struct {
			BTTrackSongPosUpdateType updateType;
			union {
				uint8_t playback; // esp_avrc_playback_stat_t
				uint32_t playPosMS;
			};
		} songUpdate;
		struct {
			// TODO: Make an enum?
			bool forward;
			bool reverse;
			bool pause;
			bool play;
		} skipUpdate;
	};
	
} BTTrackUpdateMsg;

typedef struct OEMDisplayMsg {
	bool sendToOEMDisplay;
	char message[12];
} OEMDisplayMsg;

typedef struct ReverseProximityMsg {
	uint16_t leftmostDistance; // Driver side
	uint16_t middleLeftDistance;
	uint16_t middleRightDistance;
	uint16_t rightmostDistance; // Passenger side
} ReverseProximityMsg;

typedef struct BuzzerMsg {
	uint32_t frequency1;
	uint32_t frequency2;
	uint16_t duration;
} BuzzerMsg;

typedef struct AudioSourceMsg {
	uint8_t audioSource;
} AudioSourceMsg;

#endif