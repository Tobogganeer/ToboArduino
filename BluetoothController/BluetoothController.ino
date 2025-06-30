/*

BluetoothAudio code is huge; it can't run a display but it can do ESP-NOW
This board will communicate via ESP-NOW and drive the display/settings

*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <CarComms.h>
#include "esp_timer.h"
#include "Rotary.h"
#include "Button2.h"

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

#define DEBUG

typedef uint8_t State;

enum : uint8_t
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
};

#define NO_RECV_ERROR_THRESHOLD_MS 5000  // If we get no data after this time, the audio module isn't communicating
#define TIME_FROM_IDLE_TO_SLEEP_MS 10000
#define MESSAGE_SLEEP_TIME 3000
#define MESSAGE_DISCONNECT_TIME 4000
#define MESSAGE_CONNECT_TIME 2000

// TODO: Change these once it's actually connected
#define ROTARY_PIN1 13
#define ROTARY_PIN2 12
#define ROTARY_BUTTON 26

#define CLICKS_PER_ROTATION 4  // The encoder outputs 4 times when rotated once

#define SETTINGS_MAIN_DEVICE_LIST 0
#define SETTINGS_MAIN_PAIR_DEVICE 1
#define SETTINGS_MAIN_DISCONNECT 2
#define SETTINGS_MAIN_GO_BACK 3

#define SETTINGS_DEVICE_CONNECT 0
#define SETTINGS_DEVICE_FAVOURITE 1
#define SETTINGS_DEVICE_DELETE 2
#define SETTINGS_DEVICE_MOVE_UP 3
#define SETTINGS_DEVICE_MOVE_DOWN 4
#define SETTINGS_DEVICE_GO_BACK 5


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

int selectedOption;
int numOptions;

char cachedTitle[19] = { 0 };
char cachedArtist[19] = { 0 };
char cachedAlbum[19] = { 0 };
char cachedPlayTime[21] = { 0 };
char blankLine[21] = { 0 };


void switchStateInstant(State endState);
void switchStateWithIntermediate(State endState, State intermediateState, uint32_t timeInIntermediateStateMS);
void afterStateSwitched(State from, State to);


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

    memset(blankLine, ' ', 20);  // Set first 20 chars to spaces
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
    log_i("Switch to state %d from state %d", to, from);

    // Stuff like turn display back on from sleep

    // FROM STATE
    switch (from)
    {
        case STATE_SLEEP:
            lcd.backlight();  // Turn display back on
            break;
        case STATE_DISCOVERABLE:
            setDiscoverable(false);  // Back into hiding
            break;
    }

    // TO STATE
    switch (to)
    {
        case STATE_SLEEP:
            lcd.clear();
            lcd.noBacklight();  // Turn display off
            break;
        case STATE_IDLE:
            lcd.clear();
            displayMessage("No device connected", "Press to enter menu", false);
            break;
        case STATE_DISCOVERABLE:
            setDiscoverable(true);  // Make visible to pairing
            displayMessage("Bluetooth visible", "as 'Quandale's Whip'", false);

            // TODO: Disconnect when trying to pair, but if we leave and aren't connected, try reconnecting
            // TODO: Add packet/ability to start a reconnect from this end
            break;
        case STATE_SETTINGS_MAIN:
            numOptions = 4;
            break;
        case STATE_SETTINGS_DEVICE_LIST:
        case STATE_SETTINGS_DEVICE:
            numOptions = 6;
            break;
        case STATE_DISPLAY:
            // Clear cached strings so we re-print everything
            memset(cachedTitle, 0, 19);
            memset(cachedArtist, 0, 19);
            memset(cachedAlbum, 0, 19);
            memset(cachedPlayTime, 0, 21);
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
                            displayMusic();
                            break;
                        case BT_SONG_POS_UPDATE_PLAY_STATUS_CHANGE:
                            // msg->songUpdate.
                            // uint8_t playback; // esp_avrc_playback_stat_t
                            log_i("PlaybacK: %d", msg->songUpdate.playback);
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
    //lcd.clear();

    // Only write lines if they have changed

    if (strncmp(cachedTitle, songInfo.songInfo.title, 18) != 0)
    {
        memcpy(cachedTitle, songInfo.songInfo.title, 18);
        lcd.setCursor(0, 0);
        lcd.print(blankLine);
        lcd.setCursor(0, 0);
        lcd.write(ICON_SONG);
        lcd.setCursor(2, 0);
        lcd.print(cachedTitle);
    }

    // Artist
    if (strncmp(cachedArtist, songInfo.songInfo.artist, 18) != 0)
    {
        memcpy(cachedArtist, songInfo.songInfo.artist, 18);
        lcd.setCursor(0, 1);
        lcd.print(blankLine);
        lcd.setCursor(0, 1);
        lcd.write(ICON_ARTIST);
        lcd.setCursor(2, 1);
        lcd.print(cachedArtist);
    }

    // Album
    if (strncmp(cachedAlbum, songInfo.songInfo.album, 18) != 0)
    {
        memcpy(cachedAlbum, songInfo.songInfo.album, 18);
        lcd.setCursor(0, 2);
        lcd.print(blankLine);
        lcd.setCursor(0, 2);
        lcd.write(ICON_ALBUM);
        lcd.setCursor(2, 2);
        lcd.print(cachedAlbum);
    }

    // Song time
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
        memcpy(&fullLine[20 - pausedLen], pausedText, strlen(pausedText));
        fullLine[20] = 0;  // Null terminate

        // Fill space between time and PAUSED with spaces
        int timeLen = strlen(fullLine);
        memset(&fullLine[timeLen], 0, 20 - pausedLen - timeLen);
        // 4:52 / 5:06---PAUSED
        // ''''''''''''''''''''
        // timeLen: 11
        // pausedLen: 6
    }
    if (strncmp(cachedPlayTime, fullLine, 21) != 0)
    {
        memcpy(cachedPlayTime, fullLine, 21);
        lcd.setCursor(0, 3);
        lcd.print(blankLine);
        lcd.setCursor(0, 3);
        lcd.print(cachedPlayTime);
    }
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
    int center = 10 - len / 2;         // Divide will floor so this will always be right
    memset(line, ' ', center);         // Spaces to pad
    memcpy(&line[center], text, len);  // Copy text
    line[center + len] = 0;            // Null terminate
    lcd.print(line);
}

/*
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

    SETTINGS_MAIN_DEVICE_LIST 0
    SETTINGS_MAIN_PAIR_DEVICE 1
    SETTINGS_MAIN_GO_BACK 2
    SETTINGS_MAIN_DISCONNECT 3

    SETTINGS_DEVICE_CONNECT 0
    SETTINGS_DEVICE_MOVE_UP 1
    SETTINGS_DEVICE_MOVE_DOWN 2
    SETTINGS_DEVICE_FAVOURITE 3
    SETTINGS_DEVICE_DELETE 4
    SETTINGS_DEVICE_GO_BACK 5
*/

void rotateLeft(Rotary& dial)
{
    log_i("Left");

    if (state == STATE_SLEEP)
        switchStateInstant(STATE_IDLE);
    else if (state == STATE_SETTINGS_MAIN || state == STATE_SETTINGS_DEVICE || state == STATE_SETTINGS_DEVICE_LIST)
    {
        // Decrement option, wrapping around
        // Note: numOptions is set when the state is changed
        selectedOption--;
        if (selectedOption < 0)
            selectedOption = numOptions - 1;

        selectedOptionChanged();
    }
}

void rotateRight(Rotary& dial)
{
    log_i("Right");

    if (state == STATE_SLEEP)
        switchStateInstant(STATE_IDLE);
    else if (state == STATE_SETTINGS_MAIN || state == STATE_SETTINGS_DEVICE || state == STATE_SETTINGS_DEVICE_LIST)
    {
        // Increment option, wrapping around
        // Note: numOptions is set when the state is changed
        selectedOption++;
        if (selectedOption >= numOptions)
            selectedOption = 0;

        selectedOptionChanged();
    }
}

void selectedOptionChanged()
{
    // TODO: Refresh current screen
}

void click(Button2& btn)
{
    log_i("Click");

    switch (state)
    {
        case STATE_SETTINGS_DEVICE_LIST:
            deviceList_click();
            break;
        case STATE_SETTINGS_DEVICE:
            device_click();
            break;
        case STATE_SETTINGS_MAIN:
            mainSettings_click();
            break;
        case STATE_SPLASHSCREEN:
        case STATE_ERROR_NO_MESSAGES:
        case STATE_CONNECTING:  // TODO: Timeout timer in BTAudio when we try to connect
            // States where you shouldn't be able to click
            break;
        case STATE_TRANSITION_MESSAGE:
            // Skip message
            stateTimerCB(nullptr);
            break;
        case STATE_SLEEP:
            // Wake up
            switchStateInstant(STATE_IDLE);
            break;
        case STATE_DISCOVERABLE:
        case STATE_IDLE:
        case STATE_DISPLAY:
            // afterStateSwitched() handles setting discoverable
            switchStateInstant(STATE_SETTINGS_MAIN);
            break;
    }
}

/*

=== Main
Device List
Pair Device (4/5)
Disconnect
Go back

=== Device
Connect/Disconnect
Favourite
Delete
VVV

^^^
Move up
Move down
Go back

=== Device List
Device 1
Device 2
Device 3
VVV

^^^
Device 4
<empty slot>
Go back

*/

void deviceList_click()
{
}

void device_click()
{
}

void mainSettings_click()
{
}

void setDiscoverable(bool discoverable)
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_SET_DISCOVERABLE;
    msg.discoverable = discoverable;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}