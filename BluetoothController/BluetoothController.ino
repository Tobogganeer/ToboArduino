/*

BluetoothAudio code is huge; it can't even run a display
This board will communicate via ESP-NOW and drive the display/settings

*/

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

void setup()
{
  lcd.init();                      // initialize the lcd 
  //lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(3,0);
  lcd.print("Quandale Whip");
  lcd.setCursor(2,1);
  lcd.print("V0.0.1");
  lcd.setCursor(0,2);
  lcd.print("Guh");
  lcd.setCursor(2,3);
  lcd.print("B)");
}


int i = 0;

void loop()
{
  lcd.setCursor(0, 0);
  lcd.print(i++);
  delay(1000);
}