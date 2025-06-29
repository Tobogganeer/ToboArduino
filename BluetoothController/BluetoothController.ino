/*

BluetoothAudio code is huge; it can't run a display but it can do ESP-NOW
This board will communicate via ESP-NOW and drive the display/settings

*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <CarComms.h>
#include "esp_timer.h"
#include "Rotary.h"
#include "Button2.h";

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

#define NO_RECV_ERROR_THRESHOLD_MS 5000  // If we get no data after this time, the audio module isn't communicating
#define TIME_FROM_IDLE_TO_SLEEP_MS 10000
#define MESSAGE_SLEEP_TIME 3000
#define MESSAGE_DISCONNECT_TIME 4000
#define MESSAGE_CONNECT_TIME 2000

// TODO: Change these once it's actually connected
#define ROTARY_PIN1 2
#define ROTARY_PIN2 0
#define ROTARY_BUTTON 3

#define CLICKS_PER_ROTATION 4  // The encoder outputs 4 times when rotated once


State state;
State nextState;
esp_timer_handle_t stateSwitchTimer;
bool switchingState;
long lastStateSwitchTimeMS;

BTInfoMsg devices;
BTInfoMsg songInfo;
BTInfoMsg connectedDisconnected;
uint8_t playbackStatus;  // esp_avrc_playback_stat_t
uint32_t playPosMS;

Rotary dial;
Button2 dialButton;

void setup()
{
    lcd.init();
    lcd.backlight();

    initIcons();  // Icons.h
    initDial();

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

void initDial()
{
    dial.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_ROTATION);
    dial.setLeftRotationHandler(rotateLeft);
    dial.setRightRotationHandler(rotateRight);

    dialButton.begin(ROTARY_BUTTON);
    dialButton.setTapHandler(click);
}


void loop()
{
    // Make sure we are receiving messages
    checkError();

    dial.loop();
    dialButton.loop();

    switch (state)
    {
        case STATE_IDLE:
            if (timeSinceStateSwitchedMS() > TIME_FROM_IDLE_TO_SLEEP_MS)
            {
                switchStateWithIntermediate(STATE_SLEEP, STATE_TRANSITION_MESSAGE, MESSAGE_SLEEP_TIME);
                displayMessage("Sleeping in", "3 seconds...", false);
            }
            break;
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
    switchingState = false;  // TODO: Maybe move this to stateTimerCB and return from the fn if switchingState?
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

    // Stuff like turn display back on from sleep

    // FROM STATE
    switch (from)
    {
        case STATE_SLEEP:
            lcd.backLight();  // Turn display back on
            break;
    }

    // TO STATE
    switch (to)
    {
        case STATE_SLEEP:
            lcd.clear();
            lcd.noBackLight();  // Turn display off
            break;
        case STATE_IDLE:
            lcd.clear();
            displayMessage("No device connected", "Press to pair device", false);
            break;
    }
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
                            playbackStatus = msg->songUpdate.playback;
                            displayMusic();
                            break;
                        case BT_SONG_POS_UPDATE_PLAY_POS_CHANGED:
                            // msg->songUpdate.
                            // uint32_t playPosMS;
                            playPosMS = msg->songUpdate.playPosMS;
                            displayMusic();
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
                    displayMusic();
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
                    displayMessage("Connected to", connectedDisconnected.sourceDevice.deviceName, true);
                    // TODO: Move this to afterStateSwitched for consistency?

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
                        displayMessage("Disconnected from", connectedDisconnected.sourceDevice.deviceName, true);
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

void displayMusic()
{
    lcd.clear();
    char line[19] = { 0 };

    // Title
    lcd.setCursor(0, 0);
    lcd.write(ICON_SONG);
    lcd.setCursor(2, 0);
    memcpy(line, songInfo.songInfo.title, 18);
    lcd.print(line);

    // Artist
    lcd.setCursor(0, 1);
    lcd.write(ICON_ARTIST);
    lcd.setCursor(2, 1);
    memcpy(line, songInfo.songInfo.artist, 18);
    lcd.print(line);

    // Album
    lcd.setCursor(0, 2);
    lcd.write(ICON_ALBUM);
    lcd.setCursor(2, 2);
    memcpy(line, songInfo.songInfo.album, 18);
    lcd.print(line);

    // Song time
    lcd.setCursor(0, 3);

    int currentMins = (playPosMS / 1000) / 60;
    int currentSecs = (playPosMS / 1000) % 60;
    int totalMins = (songInfo.songInfo.trackLengthMS / 1000) / 60;
    int totalSecs = (songInfo.songInfo.trackLengthMS / 1000) % 60;
    // e.g. 4:52 / 5:06
    // PAUSED---4:52 / 5:06
    // 4:52 / 5:06---PAUSED
    // ''''''''''''''''''''
    char fullLine[21] = { 0 };  // Other 'line' is shorter as it ignores the symbol and space (e.g. the artist symbol)
    sprintf(fullLine, "%d:%.2d / %d:%.2d", currentMins, currentSecs, totalMins, totalSecs);

    if (playbackStatus == PLAYBACK_PAUSED)
    {
        // Add 'PAUSED' to end of line
        const char* pausedText = "PAUSED";
        int pausedLen = strlen(pausedText);
        memcpy(fullLine[20 - pausedLen], pausedText, strlen(pausedText));
        fullLine[20] = 0;  // Null terminate

        // Fill space between time and PAUSED with spaces
        int timeLen = strlen(fullLine);
        memset(fullLine[timeLen], 0, 20 - pausedLen - timeLen);
        // 4:52 / 5:06---PAUSED
        // ''''''''''''''''''''
        // timeLen: 11
        // pausedLen: 6
    }
    lcd.print(fullLine);
    // TODO: Take status into account (paused at least)
}

void displayMessage(const char* firstLine, const char* secondLine, bool overflowSecondLine)
{
    lcd.clear();
    lcd.setCursor(0, 1);
    printCentered(firstLine);
    lcd.setCursor(0, 2);
    if (overflowSecondLine)
        lcd.print(secondLine);  // Don't print centered so it can wrap around
    else
        printCentered(secondLine);
}

void printCentered(const char* text)
{
    if (text == nullptr)
        return;

    // e.g. Hello
    // len = 5
    // center = 10 - (5 / 2) = 8
    // ''''''''------------
    // ''''''''Hello-------
    // ''''''''Hello0------

    char line[21] = { 0 };
    int len = strnlen(text, 20);
    int center = 10 - len / 2;        // Divide will floor so this will always be right
    memset(line, ' ', center);        // Spaces to pad
    memcpy(line[center], text, len);  // Copy text
    line[center + len] = 0;           // Null terminate
    lcd.print(line);
}


void rotateLeft(Rotary& dial)
{
    // Left
}

void rotateRight(Rotary& dial)
{
    // Right
}

void click(Button2& btn)
{
    // Click
}