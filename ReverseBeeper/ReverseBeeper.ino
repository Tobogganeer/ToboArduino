#include "CarComms.h"

CarComms comms(handleCarData);

bool wasBeeping;
bool doBeep;

uint32_t turnOffBeepTimeout = 1000;

int beepLength = 600;
int blankLength = 400;

bool beeping;

unsigned long beepTimer;

void setup()
{
    Serial.begin(115200);

    comms.begin();
    // Only want to receive info (contains whether or not we are in reverse)
    comms.receiveTypeMask = CarDataType::ID_CARINFO;
}

void handleCarData(CarDataType type, const uint8_t* data, int len)
{
    if (type == CarDataType::ID_CARINFO)
    {
        CarInfoMsg* info = (CarInfoMsg*)data;
        bool carRunning = info.rpm > 0;
        bool moving = info.speed > 0;
        //bool pressingThrottle = info.throttlePosition > 0;
        bool inReverse = info.reversing;

        doBeep = carRunning && moving /*&& pressingThrottle*/ && inReverse;
    }
}

void loop()
{
    // If no messages for a while (e.g. in ACC) turn off beeper
    if (comms.getTimeSinceLastReceiveMS(CarDataType::ID_CARINFO) > turnOffBeepTimeout)
        doBeep = false;

    auto currentTime = millis();

    // If we just started beeping, restart timer
    if (doBeep && !wasBeeping)
        beepTimer = currentTime;

    auto elapsedTime = currentTime - beepTimer;

    if (elapsedTimer < beepLength)
        TurnOnBeep();
    else
    {
        TurnOffBeep();

        // Reset timer once the beep and blank have finished
        if (elapsedTimer > beepLength + blankLength)
            beepTimer = currentTime;
    }

    wasBeeping = doBeep;
}

void TurnOnBeep()
{
    if (beeping)
        return;

    beeping = true;

    // TODO: Turn on beeper
}

void TurnOffBeep()
{
    if (!beeping)
        return;

    beeping = false;

    // TODO: Turn off beeper
}
