int beepPin = 6;

bool wasBeeping;
bool doBeep;

uint32_t turnOffBeepTimeout = 1000;

int beepLength = 600;
int blankLength = 400;

bool beeping;

unsigned long beepTimer;

void setup()
{
    doBeep = true;
    pinMode(beepPin, OUTPUT);
}

void loop()
{
    auto currentTime = millis();

    // If we just started beeping, restart timer
    if (doBeep && !wasBeeping)
        beepTimer = currentTime;

    auto elapsedTime = currentTime - beepTimer;

    if (elapsedTime < beepLength)
        TurnOnBeep();
    else
    {
        TurnOffBeep();

        // Reset timer once the beep and blank have finished
        if (elapsedTime > beepLength + blankLength)
            beepTimer = currentTime;
    }

    wasBeeping = doBeep;
}

void TurnOnBeep()
{
    if (beeping)
        return;

    beeping = true;
    blankLength = random(300, 500);

    digitalWrite(beepPin, HIGH);
    //tone(beepPin, random(500, 2000));
}

void TurnOffBeep()
{
    if (!beeping)
        return;

    beeping = false;
    beepLength = random(550, 800);

    digitalWrite(beepPin, LOW);
    //noTone(beepPin);
}
