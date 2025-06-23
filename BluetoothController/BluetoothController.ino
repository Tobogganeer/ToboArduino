/*

BluetoothAudio code is huge; it can't even run a display
This board will communicate via ESP-NOW and drive the display/settings

*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

uint8_t songIcon[8] = {
    0b00000,
    0b01100,
    0b01011,
    0b01001,
    0b01001,
    0b11011,
    0b11011,
    0b00000,
};

uint8_t artistIcon[8] = {
    0b01110,
    0b11111,
    0b11111,
    0b01110,
    0b00000,
    0b01110,
    0b11111,
    0b00000,
};

uint8_t albumIcon[8] = {
    0b00000,
    0b01110,
    0b11111,
    0b11011,
    0b11111,
    0b01110,
    0b00000,
    0b00000,
};

/*
Splash screen

===================
...Goofy ah car....
.......V1.0........
...Evan Daveikis...
  System booting...
===================

*/

void setup()
{
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, songIcon);
    lcd.createChar(1, artistIcon);
    lcd.createChar(2, albumIcon);

    // Splash screen
    lcd.setCursor(3, 0);
    lcd.print("Goofy ah car");
    lcd.setCursor(7, 1);
    lcd.print("V1.0");
    lcd.setCursor(3, 2);
    lcd.print("Evan Daveikis");
    lcd.setCursor(2, 3);
    lcd.print("System booting...");

    delay(2000);
    lcd.home();
    lcd.write(byte(0));
    lcd.write(byte(1));
    lcd.write(byte(2));
}


/*
Song details

Song name
Artist name
Album name
Current progress

===================
Song name
Artist name
Album
Current progress
===================

*/

void loop()
{
}