// 34, 35, 32, 33
#include "CarComms.h"

// LR is the short one
// FB is the long one
// L is close, R is far
// F is close, B is far

const int leftPin = 34;   // Green
const int rightPin = 35;  // Blue
const int frontPin = 32;  // Yellow
const int backPin = 33;   // Orange

CarComms comms(handleCarData);

uint8_t currentSentGear = 0;
uint8_t previousGear = 0;
unsigned long msWeWentIntoNeutral;

// If we go from first to second, give this long before setting the gear to neutral
const int timeInNeutralBeforeActuallySetting = 500;

#define DEBUG

void setup()
{
#ifdef DEBUG
    Serial.begin(115200);
#endif

    comms.begin();
    comms.receiveTypeMask = 0;  // We don't want to receive at all, actually

    pinMode(leftPin, INPUT);
    pinMode(rightPin, INPUT);
    pinMode(frontPin, INPUT);
    pinMode(backPin, INPUT);
}

void handleCarData(CarDataType type, const uint8_t* data, int len)
{
}

void loop()
{
    // https://github.com/upiir/arduino_gear_indicator/blob/main/ARDUINO_gear_indicator/ARDUINO_gear_indicator.ino

    bool left = isActivated(leftPin);
    bool right = isActivated(rightPin);
    bool forward = isActivated(frontPin);
    bool backward = isActivated(backPin);

    uint8_t gear = 0;

    bool centered = !left && !right;

    if (left && forward)
        gear = FIRST;
    else if (left && backward)
        gear = SECOND;
    else if (centered && forward)
        gear = THIRD;
    else if (centered && backward)
        gear = FOURTH;
    else if (right && forward)
        gear = FIFTH;
    else if (right && backward)
        gear = REVERSE;
    else
        gear = NEUTRAL;

    // Wait a bit before changing state to neutral (to avoid flicker when going from first to second, etc)
    if (gear == NEUTRAL)
    {
        // Check if we just went into "neutral"
        if (previousGear != NEUTRAL)
        {
            msWeWentIntoNeutral = millis();
        }
        // We've been in neutral for a bit
        else
        {
            unsigned long msInNeutral = millis() - msWeWentIntoNeutral;
            if (msInNeutral > timeInNeutralBeforeActuallySetting)
                currentSentGear = gear;
        }
    }
    else
    {
        // If we aren't in neutral, just send the gear immediately
        currentSentGear = gear;
    }

    previousGear = gear;

    GearMsg msg;
    msg.gear = (Gear)currentSentGear;
    comms.send(CarDataType::ID_GEAR, &msg, sizeof(GearMsg));

#ifdef DEBUG
    Serial.print("one:1,low:0,");
    Serial.print("sentgear:" + String(currentSentGear));
    Serial.println(",gear:" + String(gear));
#endif

    delay(50);  // 20 times a second is enough
}

bool isActivated(int pin)
{
    // Base reading is ~0.45 (voltage divider shenanigans)
    const float baseline = 0.45f;
    const float threshold = 0.15f;

    float value = analogRead(pin) / 4095.0f;
#ifdef DEBUG
    Serial.print("pin" + String(pin) + ":" + String(value) + ",");
#endif

    return fabsf(value - baseline) > threshold;
}
