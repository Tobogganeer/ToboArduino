// 34, 35, 32, 33
#include "CarComms.h"

// LR is the short one
// FB is the long one
// L is close, R is far
// F is close, B is far

const int leftPin = 34; // Green
const int rightPin = 35; // Blue
const int frontPin = 32; // Yellow
const int backPin = 33; // Orange

CarComms comms(handleCarData);

void setup()
{
    Serial.begin(115200);

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

    GearMsg msg;
    msg.gear = (Gear)gear;
    comms.send(CarDataType::ID_GEAR, &msg, sizeof(GearMsg));

    delay(50); // 20 times a second is enough

    // TODO: Store last few gears and update only if it's been consistent for a few ticks
}

bool isActivated(int pin)
{
    // Base reading is ~0.45 (voltage divider shenanigans)
    const float baseline = 0.45f;
    const float threshold = 0.25f;

    float value = analogRead(pin) / 4095.0f;
    return fabsf(value - baseline) > threshold;
}
