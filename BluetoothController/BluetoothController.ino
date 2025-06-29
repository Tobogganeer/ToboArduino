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
    STATE_RECONNECTING,
    STATE_IDLE,
    STATE_SLEEP,
    STATE_DISPLAY,
    STATE_SETTINGS_MAIN,
    STATE_SETTINGS_DEVICE,
    STATE_SETTINGS_DEVICE_LIST,
    STATE_CONNECTING,
    STATE_DISCOVERABLE,
    STATE_TRANSITION_MESSAGE,
    STATE_ERROR_NO_MESSAGES
} State;

#define NO_RECV_ERROR_THRESHOLD_MS 5000 // If we get no data after this time, the audio module isn't communicating
#define TIME_FROM_IDLE_TO_SLEEP_MS 10000
#define MESSAGE_SLEEP_TIME 3000
#define MESSAGE_DISCONNECT_TIME 4000
#define MESSAGE_CONNECT_TIME 2000


State state;
State nextState;
esp_timer_handle_t stateSwitchTimer;
bool switchingState;
long lastStateSwitchTimeMS;

BTInfoMsg devices;
BTInfoMsg songInfo;
BTInfoMsg connectedDisconnected;


void setup()
{
    lcd.init();
    lcd.backlight();

    initIcons();  // Icons.h

#ifdef DEBUG
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_INFO);
#endif

    switchStateInstant(STATE_SPLASHSCREEN);
    splashScreen();

    comms.begin();
    comms.receiveTypeMask = CarDataType::ID_BT_TRACK_UPDATE | CarDataType::ID_BT_INFO;

    esp_timer_create_args_t timerArgs = {
        .callback = &stateTimerCB,
        .name = "stateSwitchTimer"
    };
    esp_timer_create(&timerArgs, &stateSwitchTimer);
}


void loop()
{
    // Make sure we are receiving messages
    checkError();

    switch (state)
    {
        case STATE_IDLE:
            if (timeSinceStateSwitchedMS() > TIME_FROM_IDLE_TO_SLEEP_MS)
            {
                switchStateWithIntermediate(STATE_SLEEP, STATE_TRANSITION_MESSAGE, MESSAGE_SLEEP_TIME);
                // TODO: Display "going to sleep" message here
            }
    }
}

void checkError()
{
    bool hasReceivedAnyMessage = comms.getTimeSinceLastReceiveMS() != -1;
    long currentTimeMS = millis();

    if (!hasReceivedAnyMessage && currentTimeMS > NO_RECV_ERROR_THRESHOLD_MS)
    {
        if (state != STATE_ERROR_NO_MESSAGES)
        {
            // Switch no matter what - no point going to settings if we can't set anything
            switchStateInstant(STATE_ERROR_NO_MESSAGES);
        }
    }
}



void stateTimerCB(void* arg)
{
    switchStateInstant(nextState);
}

void switchStateInstant(State endState)
{
    State previous = state;

    esp_timer_stop(stateSwitchTimer);
    switchingState = false; // TODO: Maybe move this to stateTimerCB and return from the fn if switchingState?
    state = endState;
    lastStateSwitchTimeMS = millis();

    afterStateSwitched(previous, endState);
}

void switchStateWithIntermediate(State endState, State intermediateState, uint32_t timeInIntermediateStateMS)
{
    switchingState = true;
    State previous = state;

    esp_timer_stop(stateSwitchTimer);                                          // Stop before (re)starting
    esp_timer_start_once(stateSwitchTimer, timeInIntermediateStateMS * 1000);  // Convert to us from ms
    nextState = endState;
    state = intermediateState;
    lastStateSwitchTimeMS = millis();

    afterStateSwitched(previous, intermediateState);
}

void afterStateSwitched(State from, State to)
{
    log_i("Switch to state %d from state %d", endState, state);

    // TODO: Stuff like turn display back on from sleep
}



long timeSinceStateSwitchedMS()
{
    return millis() - lastStateSwitchTimeMS;
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
                    // Store song info
                    memcpy(&songInfo, msg, sizeof(BTInfoMsg));
                    displayMetadata();
                    // msg->songInfo.
                    // char title[BT_SONG_INFO_MAX_STR_LEN]; // 64 bytes
                    // char artist[BT_SONG_INFO_MAX_STR_LEN]; // 128
                    // char album[BT_SONG_INFO_MAX_STR_LEN]; // 192
                    // uint32_t trackLengthMS; // 196
                    break;
                }
            case BT_INFO_DEVICES:
                {
                    // Store devices
                    memcpy(&devices, msg, sizeof(BTInfoMsg));
                    if (state == STATE_SPLASHSCREEN)
                    {
                        if (devices.devices.count == 0)
                            switchStateInstant(STATE_IDLE);
                        else if (devices.devices.reconnecting)
                            switchStateInstant(STATE_RECONNECTING);
                    }
                    else if (state == STATE_RECONNECTING)
                    {
                        // If we've stopped reconnecting, we weren't successful
                        if (devices.devices.reconnecting == false)
                            switchStateInstant(STATE_IDLE);
                    }

                    // msg->devices.
                    // uint8_t addresses[5][6]; // 5 x 6 = 30 bytes
                    // char deviceNames[5][32]; // 5 x 32 = 160
                    // uint8_t count; // 161
                    // uint8_t favourite; // 162
                    // uint8_t connected[6]; // 168, address of connected
                    // bool reconnecting;
                    break;
                }
            case BT_INFO_CONNECTED:
                {
                    memcpy(&connectedDisconnected, msg, sizeof(BTInfoMsg));
                    switchStateWithIntermediate(STATE_DISPLAY, STATE_TRANSITION_MESSAGE, MESSAGE_CONNECT_TIME);
                    // TODO: Show 'connected to X' message here

                    // msg->sourceDevice.
                    // uint8_t address[6];
                    // char deviceName[32];
                    break;
                }
            case BT_INFO_DISCONNECTED:
                {
                    memcpy(&connectedDisconnected, msg, sizeof(BTInfoMsg));
                    // Only say 'disconnected' if we are showing info - disconnecting while in settings in intentional
                    if (state == STATE_IDLE || state == STATE_DISPLAY)
                    {
                        switchStateWithIntermediate(STATE_IDLE, STATE_TRANSITION_MESSAGE, MESSAGE_DISCONNECT_TIME);
                        // TODO: Show 'disconnected from X' message here
                    }
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

void displayMetadata()
{
    lcd.clear();
    char line[19] = { 0 };

    lcd.setCursor(0, 0);
    lcd.write(ICON_SONG);
    lcd.setCursor(2, 0);
    memcpy(line, songInfo.songInfo.title, 18);
    lcd.print(line);

    lcd.setCursor(0, 1);
    lcd.write(ICON_ARTIST);
    lcd.setCursor(2, 1);
    memcpy(line, songInfo.songInfo.artist, 18);
    lcd.print(line);

    lcd.setCursor(0, 2);
    lcd.write(ICON_ALBUM);
    lcd.setCursor(2, 2);
    memcpy(line, songInfo.songInfo.album, 18);
    lcd.print(line);

    lcd.setCursor(0, 3);
    lcd.print(songInfo.songInfo.trackLengthMS / 1000);
}