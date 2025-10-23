// 34, 35, 32, 33
#include "CarComms.h"

// LR is the short one
// FB is the long one
// L is close, R is far
// F is close, B is far

const int leftPin = 34;
const int rightPin = 35;
const int frontPin = 32;
const int backPin = 33;

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

    float left = analogRead(leftPin) / 4095.0f;
    float right = analogRead(rightPin) / 4095.0f;
    float front = analogRead(frontPin) / 4095.0f;
    float back = analogRead(backPin) / 4095.0f;

    Serial.print("left:");
    Serial.print(left);
    Serial.print(",");

    Serial.print("right:");
    Serial.print(right);
    Serial.print(",");

    Serial.print("front:");
    Serial.print(front);
    Serial.print(",");

    Serial.print("back:");
    Serial.print(back);
    Serial.println();

    /*
    GearMsg msg;
    msg.gear = Gear::Neutral;
    comms.send(CarDataType::ID_GEAR, &msg, sizeof(GearMsg));
    */
}
