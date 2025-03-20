#include "CarDataReceiver.h"

void setup() {
  Serial.begin(115200);
  initCarDataReceiver(handleCarData);
}

void handleCarData(CarData* data)
{
  Serial.print("Got data. RPM: ");
  Serial.print(data->rpm);
  Serial.print(", Speed:");
  Serial.println(data->speed);
}

void loop() {

}
