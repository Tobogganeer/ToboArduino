
// Arduino UNO
// Testing if CAN lines actually have data

const int CAN_H = A0;
const int CAN_L = A1;

void setup()
{
    Serial.begin(115200);
    delay(1000);
}

void loop()
{
    // Reading potentiometer value
    float canH = analogRead(CAN_H);
    float canL = analogRead(CAN_L);
    Serial.print("H:");
    Serial.print(canH);
    Serial.print(",L:");
    Serial.print(canL);
    Serial.print(",z:");
    Serial.print(0);
    Serial.print(",o:");
    Serial.println(1023);
    delay(10);
}
