/*

Glorified demultiplexer
Read inputs and send them to the main ESP32 board

*/

void setup() {
  Serial.begin(9600);
}

void loop() {
  static int e;
  e++;
  Serial.println("Yello :) " + String(e));
  delay(500);
}
