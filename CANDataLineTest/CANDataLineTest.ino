
// ESP32
// Testing if CAN lines actually have data

const int CAN_H = 34;
const int CAN_L = 35;

void setup() {
  Serial.begin(115200);
  delay(1000);
}

void loop() {
  // Reading potentiometer value
  float canH = analogRead(CAN_H);
  float canL = analogRead(CAN_L);
  Serial.print("H=");
  Serial.println(canH);
  Serial.print("L=");
  Serial.println(canL);
  delay(10);
}
