/*

BluetoothAudio code is huge; it can't run a display but it can do ESP-NOW
This board will communicate via ESP-NOW and drive the display/settings

*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <CarComms.h>
#include "esp_timer.h"

LiquidCrystal_I2C lcd(0x27, 20, 4);
CarComms comms(handleCarData);

#include "Icons.h"

// Taken from esp_avrc_playback_stat_t
#define PLAYBACK_STOPPED 0
#define PLAYBACK_PLAYING 1
#define PLAYBACK_PAUSED 2
#define PLAYBACK_FWD_SEEK 3
#define PLAYBACK_REV_SEEK 4
#define PLAYBACK_ERROR 0xFF

//#define DEBUG


typedef enum : uint8_t
{
    STATE_SPLASHSCREEN,
    STATE_IDLE,
    STATE_SETTINGS_MAIN,
    STATE_CONNECTED
} State;


State state;


void setup()
{
    lcd.init();
    lcd.backlight();

    initIcons();  // Icons.h

#ifdef DEBUG
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_INFO);
#endif

    splashScreen();

    comms.begin();
    comms.receiveTypeMask = CarDataType::ID_BT_TRACK_UPDATE | CarDataType::ID_BT_INFO;
}

void splashScreen()
{
    lcd.setCursor(3, 0);
    lcd.print("Goofy ah car");
    lcd.setCursor(7, 1);
    lcd.print("V1.0");
    lcd.setCursor(3, 2);
    lcd.print("Evan Daveikis");
    lcd.setCursor(2, 3);
    lcd.print("System booting...");
}

void handleCarData(CarDataType type, const uint8_t* data, int len)
{
    log_i("Got car data. Type: %d", type);

    if (type == ID_BT_TRACK_UPDATE)
    {
        BTTrackUpdateMsg* msg = (BTTrackUpdateMsg*)data;

        switch (msg->type)
        {
            case BT_UPDATE_SONG_POS:
                {
                    switch (msg->songUpdate.updateType)
                    {
                        case BT_SONG_POS_UPDATE_TRACK_CHANGE:
                            break;
                        case BT_SONG_POS_UPDATE_PLAY_STATUS_CHANGE:
                            // msg->songUpdate.
                            // uint8_t playback; // esp_avrc_playback_stat_t
                            break;
                        case BT_SONG_POS_UPDATE_PLAY_POS_CHANGED:
                            // msg->songUpdate.
                            // uint32_t playPosMS;
                            break;
                    }
                    break;
                }
        }
    }
    else if (type == ID_BT_INFO)
    {
        BTInfoMsg* msg = (BTInfoMsg*)data;

        switch (msg->type)
        {
            case BT_INFO_METADATA:
                {
                    displayMetadata(msg);
                    // msg->songInfo.
                    // char title[BT_SONG_INFO_MAX_STR_LEN]; // 64 bytes
                    // char artist[BT_SONG_INFO_MAX_STR_LEN]; // 128
                    // char album[BT_SONG_INFO_MAX_STR_LEN]; // 192
                    // uint32_t trackLengthMS; // 196
                    break;
                }
            case BT_INFO_DEVICES:
                {
                    // msg->devices.
                    // uint8_t addresses[5][6]; // 5 x 6 = 30 bytes
                    // char deviceNames[5][32]; // 5 x 32 = 160
                    // uint8_t count; // 161
                    // uint8_t favourite; // 162
                    // uint8_t connected; // 163
                    break;
                }
            case BT_INFO_CONNECTED:
                {
                    // msg->sourceDevice.
                    // uint8_t address[6];
                    // char deviceName[32];
                    break;
                }
            case BT_INFO_DISCONNECTED:
                {
                    // msg->sourceDevice.
                    // uint8_t address[6];
                    // char deviceName[32];
                    break;
                }
        }
    }
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

void displayMetadata(BTInfoMsg* msg)
{
    char line[19] = { 0 };

    lcd.setCursor(0, 0);
    lcd.write(ICON_SONG);
    lcd.setCursor(2, 0);
    memcpy(line, msg->songInfo.title, 18);
    lcd.print(line);

    lcd.setCursor(0, 1);
    lcd.write(ICON_ARTIST);
    lcd.setCursor(2, 1);
    memcpy(line, msg->songInfo.artist, 18);
    lcd.print(line);

    lcd.setCursor(0, 2);
    lcd.write(ICON_ALBUM);
    lcd.setCursor(2, 2);
    memcpy(line, msg->songInfo.album, 18);
    lcd.print(line);

    lcd.setCursor(0, 3);
    lcd.print(msg->songInfo.trackLengthMS / 1000);
}

void loop()
{
}