#include <FireTimer.h>

const int fireRelayPin = 2;

const int doorLeverPin = 3;
const int fireButtonPin = 4;
const int fireLEDPin = 5;

const int enableAPin = 6;  // Door motor
const int in1Pin = 7;
const int in2Pin = 8;
const int in3Pin = 9;
const int in4Pin = 10;
const int enableBPin = 11;  // Spring compressor

const int trigPin = 12;
const int echoPin = A6;

int bottleDistance;

enum MotorState
{
    STOPPED = 0,
    FORWARD = 1,
    REVERSE = -1
};

MotorState springMotorState;
MotorState doorMotorState;

FireTimer springMotorTimer;
FireTimer doorMotorTimer;

void setup()
{
    pinMode(fireRelayPin, OUTPUT);

    pinMode(doorLeverPin, INPUT_PULLUP);
    pinMode(fireButtonPin, INPUT_PULLUP);
    pinMode(fireLEDPin, OUTPUT);

    pinMode(enableAPin, OUTPUT);
    pinMode(in1Pin, OUTPUT);
    pinMode(in2Pin, OUTPUT);
    pinMode(in3Pin, OUTPUT);
    pinMode(in4Pin, OUTPUT);
    pinMode(enableBPin, OUTPUT);

    pinMode(echoPin, INPUT);
    pinMode(trigPin, OUTPUT);

    Serial.begin(9600);
}

void loop()
{
    digitalWrite(fireLEDPin, digitalRead(fireButtonPin));    // Button and LED
    digitalWrite(fireRelayPin, !digitalRead(doorLeverPin));  // Door lever and fire relay

    if (springMotorTimer.fire(false))
    {
      
    }

    if (doorMotorTimer.fire(false))
    {
    }
}

int updateBottleDistance()
{
    // https://projecthub.arduino.cc/Isaac100/getting-started-with-the-hc-sr04-ultrasonic-sensor-7cabe1
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    unsigned long echoDuration = pulseIn(echoPin, HIGH);
    bottleDistance = (duration * 0.0343f) / 2;

    return bottleDistance;
}
