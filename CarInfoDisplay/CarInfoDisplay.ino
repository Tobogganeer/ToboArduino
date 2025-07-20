/*

    CarInfoDisplay
    Show info like km remaining, coolant temp

*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <CarComms.h>

// #ifdef U8X8_HAVE_HW_SPI
// #include <SPI.h>
// #endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
//U8G2_SSD1306_128X64_ALT0_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   // same as the NONAME variant, but may solve the "every 2nd line skipped" problem
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* reset=*/ 8);
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 16, /* data=*/ 17, /* reset=*/ U8X8_PIN_NONE);   // ESP32 Thing, pure SW emulated I2C
//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 16, /* data=*/ 17);   // ESP32 Thing, HW I2C with pin remapping

// https://github.com/olikraus/u8g2/discussions/2028
#define ALIGN_CENTER(t) (u8g2.getStrWidth(t) / 2)
#define ALIGN_RIGHT(t) (u8g2.getStrWidth(t))
#define ALIGN_LEFT 0

#define INFO_SCREEN_DELAY 3000

CarComms comms(handleCarData);

#define MIN_DISPLAY_DELAY_MS 500

long lastDisplayTime;

#define FONT_KM_REMAINING u8g2_font_spleen16x32_mn
#define FONT_LARGE u8g2_font_7x13_tr
#define FONT_SMALL u8g2_font_5x7_tr
#define FONT_TINY u8g2_font_tiny5_tr

void setup(void)
{
    u8g2.begin();
    comms.begin();
    comms.receiveTypeMask = CarDataType::ID_CARINFO;

    u8g2.clearBuffer();

    u8g2.setCursor(20, 38);
    u8g2.setFont(FONT_LARGE);
    u8g2.print("System booting...");

    u8g2.sendBuffer();
}

void displayInfo(CarInfoMsg& info)
{
    // Don't refresh every 100ms
    if (millis() - lastDisplayTime < MIN_DISPLAY_DELAY_MS)
        return;

    lastDisplayTime = millis() + MIN_DISPLAY_DELAY_MS;


    // TODO: Don't clear the buffer every time, draw spaces instead
    // TODO: Only redraw info that has changed (see BluetoothController)
    // TODO: Make actual proper display lol
    u8g2.clearBuffer();

    u8g2.setFont(FONT_KM_REMAINING);
    u8g2.setCursor(50 - ALIGN_RIGHT(String(info.kmRemaining).c_str()), 30);
    u8g2.print("Range: ");
    u8g2.print(info.kmRemaining);
    u8g2.print("km");

    u8g2.setCursor(DISPLAY_X, DISPLAY_Y + LINE_SPACING * 1);
    u8g2.print("Eco (inst): ");
    u8g2.print(info.fuelEcoInst);
    u8g2.print("L/100km");

    u8g2.setCursor(DISPLAY_X, DISPLAY_Y + LINE_SPACING * 2);
    u8g2.print("Eco (avg): ");
    u8g2.print(info.fuelEcoAvg);
    u8g2.print("L/100km");

    u8g2.setCursor(DISPLAY_X, DISPLAY_Y + LINE_SPACING * 3);
    u8g2.print("Coolant temp: ");
    u8g2.print(info.coolantTemp);
    u8g2.print("c");

    u8g2.sendBuffer();
}

void handleCarData(CarDataType type, const uint8_t* data, int len)
{
    if (type == CarDataType::ID_CARINFO)
    {
        CarInfoMsg info;
        memcpy(&info, data, sizeof(CarInfoMsg));
        displayInfo(info);
        /*  
        CarInfoMsg* info = (CarInfoMsg*)data;
        Serial.print("Got data. RPM: ");
        Serial.print(info->rpm);
        Serial.print(", Speed:");
        Serial.println(info->speed);
        */
    }
}

void loop(void)
{
}
