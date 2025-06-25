#include "CarComms.h"

CarComms comms(handleCarData);

void setup()
{
    Serial.begin(115200);

    comms.begin();
    // We only want to receive info and gear. This can be set at any time.
    comms.receiveTypeMask = CarDataType::ID_CARINFO | CarDataType::ID_GEAR;
}

void handleCarData(CarDataType type, const uint8_t* data, int len)
{
    if (type == CarDataType::ID_CARINFO)
    {
        CarInfoMsg* info = (CarInfoMsg*)data;
        Serial.print("Got data. RPM: ");
        Serial.print(info->rpm);
        Serial.print(", Speed:");
        Serial.println(info->speed);
    }
}

void loop()
{
    // Don't need to call any sort of comms.loop()

    if (comms.getTimeSinceLastReceiveMS() > 5000)
    {
        // ... No messages for 5 seconds
    }
    if (comms.getTimeSinceLastReceiveMS(CarDataType::ID_GEAR) > 1000)
    {
        // ... More than 1 second since last gear message
    }
}
