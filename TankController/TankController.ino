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
const int echoPin = 13;

int bottleDistance;

enum MotorState
{
    STOPPED = 0,
    FORWARD = 1,
    EXTENDED = 2,
    REVERSE = -1,
    RETRACTED = -2
};

typedef struct Motor
{
    int in1Pin;
    int in2Pin;
    int enablePin;
    MotorState state;
    FireTimer timer;

    Motor(int in1, int in2, int enable)
      : in1Pin(in1), in2Pin(in2), enablePin(enable), state(STOPPED), timer() {}
} Motor;

Motor springMotor(in3Pin, in4Pin, enableBPin);
Motor doorMotor(in1Pin, in2Pin, enableAPin);

FireTimer distanceTimer;

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

    distanceTimer.begin(250);

    Serial.begin(9600);
}

void loop()
{
    digitalWrite(fireLEDPin, digitalRead(fireButtonPin));    // Button and LED
    digitalWrite(fireRelayPin, !digitalRead(doorLeverPin));  // Door lever and fire relay

    updateMotor(springMotor);
    updateMotor(doorMotor);

    if (distanceTimer.fire())
        Serial.println(updateBottleDistance());
}

void updateMotor(Motor& motor)
{
    if (motor.timer.fire(false))
    {
        switch (motor.state)
        {
            case STOPPED:  // First time
                changeMotorState(motor, FORWARD);
                motor.timer.update(500);
                break;
            case FORWARD:
                changeMotorState(motor, EXTENDED);
                motor.timer.update(1000);
                break;
            case REVERSE:
                changeMotorState(motor, RETRACTED);
                motor.timer.update(1000);
                break;
            case EXTENDED:
                changeMotorState(motor, REVERSE);
                motor.timer.update(500);
                break;
            case RETRACTED:
                changeMotorState(motor, FORWARD);
                motor.timer.update(500);
                break;
        }
    }
}

void changeMotorState(Motor& motor, MotorState newState)
{
    motor.state = newState;

    switch (newState)
    {
        case STOPPED:
        case EXTENDED:
        case RETRACTED:
            digitalWrite(motor.in1Pin, LOW);
            digitalWrite(motor.in2Pin, LOW);
            digitalWrite(motor.enablePin, LOW);
            break;
        case FORWARD:
            digitalWrite(motor.in1Pin, HIGH);
            digitalWrite(motor.in2Pin, LOW);
            digitalWrite(motor.enablePin, HIGH);
            break;
        case REVERSE:
            digitalWrite(motor.in1Pin, LOW);
            digitalWrite(motor.in2Pin, HIGH);
            digitalWrite(motor.enablePin, HIGH);
            break;
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

    unsigned long echoDuration = pulseIn(echoPin, HIGH, 50 * 1000); // 50ms max
    bottleDistance = (echoDuration * 0.0343f) / 2;

    return bottleDistance;
}
