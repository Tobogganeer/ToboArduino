/*

BluetoothAudio code is huge; it can't run a display but it can do ESP-NOW
This board will communicate via ESP-NOW and drive the display/settings

*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <CarComms.h>
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

//#define DEBUG

#ifndef DEBUG
#define log_i(...) {}
#define log_w(...) {}
#define log_e(...) {}
#endif

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

#define DISCONNECT_SET_UNCONNECTABLE_TIME 1000 // When we disconnect, make audio module unconnectable for this time

#define ROTARY_PIN1 D3
#define ROTARY_PIN2 D4
#define ROTARY_BUTTON D5

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
long stateSwitchTimeMS;
bool switchingState;
long lastStateSwitchTimeMS;

bool waitingToSetConnectable;
long disconnectTime;

BTInfoMsg devices;
BTInfoMsg songInfo;
BTInfoMsg connectedDisconnected;
uint8_t playbackStatus;  // esp_avrc_playback_stat_t
uint8_t previousDisplayedStatus = 255;
uint32_t playPosMS;

Rotary dial;
Button2 dialButton;

int selectedOption;
int numOptions;

int selectedDevice;

char cachedTitle[19] = { 0 };
char cachedArtist[19] = { 0 };
char cachedAlbum[19] = { 0 };
char cachedPlayTime[19] = { 0 };
char blankLine[21] = { 0 };


void switchStateInstant(State endState);
void switchStateWithIntermediate(State endState, State intermediateState, uint32_t timeInIntermediateStateMS);
void afterStateSwitched(State from, State to);


void setup()
{
    delay(1000);

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

    memset(blankLine, ' ', 20);  // Set first 20 chars to spaces
}

void initDial()
{
    dial.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_ROTATION);
    dial.setLeftRotationHandler(rotateLeft);
    dial.setRightRotationHandler(rotateRight);

    dialButton.begin(ROTARY_BUTTON);
    dialButton.setClickHandler(click);
    dialButton.setLongClickDetectedHandler(longClick);
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

    // Check if we are waiting for a state switch
    if (stateSwitchTimeMS != 0)
    {   
        // We set stateSwitchTimeMS to millis() + delay, so if it is less than current time it has elapsed
        if (stateSwitchTimeMS < millis())
        {
            stateSwitchTimeMS = 0;
            stateTimerCB(nullptr);
        }
    }

    // Should we turn connection back on?
    if (waitingToSetConnectable && (disconnectTime + DISCONNECT_SET_UNCONNECTABLE_TIME) < millis())
    {
        waitingToSetConnectable = false;
        setConnectable(true);
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

    stateSwitchTimeMS = 0;
    switchingState = false;  // TODO: Maybe move this to stateTimerCB and return from the fn if switchingState?
    state = endState;
    lastStateSwitchTimeMS = millis();

    afterStateSwitched(previous, endState);
}

void switchStateWithIntermediate(State endState, State intermediateState, uint32_t timeInIntermediateStateMS)
{
    switchingState = true;
    State previous = state;

    //esp_timer_stop(stateSwitchTimer);                                          // Stop before (re)starting
    //esp_timer_start_once(stateSwitchTimer, timeInIntermediateStateMS * 1000);  // Convert to us from ms
    stateSwitchTimeMS = millis() + timeInIntermediateStateMS;
    nextState = endState;
    state = intermediateState;
    lastStateSwitchTimeMS = millis();

    afterStateSwitched(previous, intermediateState);
}

void afterStateSwitched(State from, State to)
{
    log_i("Switch from state %d to state %d", from, to);
    selectedOption = 0;

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
            displayMessage("Bluetooth visible as", "\"Daveikis Mobile\"", false);

            // TODO: Disconnect when trying to pair, but if we leave and aren't connected, try reconnecting
            // TODO: Add packet/ability to start a reconnect from this end
            break;
        case STATE_SETTINGS_MAIN:
            numOptions = 4;
            mainSettings_display();
            break;
        case STATE_SETTINGS_DEVICE_LIST:
            numOptions = 6;
            deviceListSettings_display();
            break;
        case STATE_SETTINGS_DEVICE:
            numOptions = 6;
            deviceSettings_display();
            break;
        case STATE_DISPLAY:
            // Clear cached strings so we re-print everything
            lcd.clear();
            memset(cachedTitle, 0, 19);
            memset(cachedArtist, 0, 19);
            memset(cachedAlbum, 0, 19);
            memset(cachedPlayTime, 0, 19);

            // Icons seem to get corrupted eventually - re-init them here
            initIcons();

            displayMusic();
            break;
    }
}



long timeSinceStateSwitchedMS()
{
    return millis() - lastStateSwitchTimeMS;
}

bool isBDAValid(uint8_t* bda)
{
    return !(bda[0] == 0 && bda[1] == 0 && bda[2] == 0 && bda[3] == 0 && bda[4] == 0 && bda[5] == 0);
}

bool isConnected()
{
    return isBDAValid(devices.devices.connected);
}

uint8_t getDeviceIndex(const BTInfoMsg* devices, uint8_t* bda)
{
    if (!devices || devices->devices.count == 0 || !bda)
        return 255;

    for (uint8_t i = 0; i < devices->devices.count; i++)
    {
        // Check if addresses match
        if (memcmp(devices->devices.addresses[i], bda, 6) == 0)
            return i;
    }
    return 255;
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
    //log_i("Got car data. Type: %d", type);

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
                            log_i("Playback: %d", msg->songUpdate.playback);
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
                    playbackStatus = songInfo.songInfo.playStatus;
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
                    bool devicesChanged = memcmp(&devices, msg, sizeof(BTInfoMsg)) != 0;
                    memcpy(&devices, msg, sizeof(BTInfoMsg));
                    switch (state)
                    {
                        case STATE_SPLASHSCREEN:
                            {
                                if (devices.devices.count == 0)
                                    switchStateInstant(STATE_IDLE);
                                else if (devices.devices.reconnecting)
                                    switchStateInstant(STATE_RECONNECTING);
                                else if (isBDAValid(devices.devices.connected))
                                {
                                    // We are connected
                                    onConnected(devices.devices.deviceNames[getDeviceIndex(&devices, devices.devices.connected)]);
                                }
                                break;
                            }
                        case STATE_RECONNECTING:
                            {
                                // If we've stopped reconnecting, we weren't successful
                                if (devices.devices.reconnecting == false)
                                    switchStateInstant(STATE_IDLE);
                                break;
                            }
                        case STATE_SETTINGS_MAIN:
                        case STATE_SETTINGS_DEVICE_LIST:
                        case STATE_SETTINGS_DEVICE:
                            if (devicesChanged)
                                selectedOptionChanged();  // Re-display settings
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
                    onConnected(connectedDisconnected.sourceDevice.deviceName);

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
                    else if (state == STATE_CONNECTING)
                    {
                        switchStateWithIntermediate(STATE_SETTINGS_DEVICE, STATE_TRANSITION_MESSAGE, 2000);
                        displayMessage("Connect failed");
                    }

                    // If we are trying to connect to a different device, we might still be connected
                    if (state != STATE_CONNECTING)
                        memset(devices.devices.connected, 0, 6);
                    //connected = false;
                    // msg->sourceDevice.
                    // uint8_t address[6];
                    // char deviceName[32];
                    break;
                }
        }
    }
}

void onConnected(const char* deviceName)
{
    switchStateWithIntermediate(STATE_DISPLAY, STATE_TRANSITION_MESSAGE, MESSAGE_CONNECT_TIME);
    displayMessage("Connected to", deviceName, true);
    //connected = true;
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
    if (state != STATE_DISPLAY)
        return;

    //lcd.clear();

    // Check if we just stopped
    if (playbackStatus == PLAYBACK_STOPPED)
    {
        if (previousDisplayedStatus != PLAYBACK_STOPPED)
        {
            previousDisplayedStatus = playbackStatus;
            lcd.clear();
            lcd.setCursor(0, 1);
            printCentered("NO SONG PLAYING");
        }
        return;
    }

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
    //int currentMins = (playPosMS / 1000) / 60;
    //int currentSecs = (playPosMS / 1000) % 60;
    int totalMins = (songInfo.songInfo.trackLengthMS / 1000) / 60;
    int totalSecs = (songInfo.songInfo.trackLengthMS / 1000) % 60;

    // 5:06--------PAUSED
    char timeLine[19] = { 0 };
    sprintf(timeLine, "%d:%.2d", totalMins, totalSecs);

    if (playbackStatus == PLAYBACK_PAUSED)
    {
        // Add 'PAUSED' or 'STOPPED' to end of line
        const char* pausedText = "PAUSED";
        int pausedLen = strlen(pausedText);
        int timeLen = strlen(timeLine);
        memcpy(&timeLine[18 - pausedLen], pausedText, strlen(pausedText));
        timeLine[18] = 0;  // Null terminate

        // Fill space between time and PAUSED with spaces
        //int timeLen = strlen(timeLine);
        memset(&timeLine[timeLen], ' ', 18 - pausedLen - timeLen);
        // T 5:06--------PAUSED
        // ''''''''''''''''''''
        // timeLen: 4
        // pausedLen: 6
    }

    if (strncmp(cachedPlayTime, timeLine, 18) != 0)
    {
        memcpy(cachedPlayTime, timeLine, 18);
        lcd.setCursor(0, 3);
        lcd.print(blankLine);
        lcd.setCursor(0, 3);
        lcd.write(ICON_TIME);
        lcd.setCursor(2, 3);
        lcd.print(cachedPlayTime);
    }

    previousDisplayedStatus = playbackStatus;
}

void displayMessage(const char* firstLine, const char* secondLine, bool overflowSecondLine)
{
    lcd.clear();
    if (firstLine)
    {
        lcd.setCursor(0, 1);
        printCentered(firstLine);
    }
    if (secondLine)
    {
        lcd.setCursor(0, 2);
        if (overflowSecondLine)
        {
            int len = strnlen(secondLine, 40);
            if (len <= 20)
                printCentered(secondLine);
            else
            {
                // Split up line into two and print both
                char half[21];
                memcpy(half, secondLine, 20);
                half[20] = 0;  // Null-terminate
                printCentered(half);

                // Print other half
                lcd.setCursor(0, 3);
                memcpy(half, &secondLine[20], 20);
                printCentered(half);
            }
            //lcd.print(secondLine);  // Don't print centered so it can wrap around
        }
        else
            printCentered(secondLine);
    }
}

void displayMessage(const char* firstLine)
{
    displayMessage(firstLine, nullptr, false);
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
    else if (state == STATE_DISPLAY)
        skipBackward();
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
    else if (state == STATE_DISPLAY)
        skipForward();
}

void selectedOptionChanged()
{
    switch (state)
    {
        case STATE_SETTINGS_DEVICE_LIST:
            deviceListSettings_display();
            break;
        case STATE_SETTINGS_DEVICE:
            deviceSettings_display();
            break;
        case STATE_SETTINGS_MAIN:
            mainSettings_display();
            break;
    }
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
        case STATE_CONNECTING:
            // BTAudio will tell us if we've timed out (disconnect() callback)
            // Let us go back anyways just to avoid softlocks if BTAudio cacks out
            switchStateInstant(STATE_SETTINGS_DEVICE_LIST);
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
            switchStateInstant(STATE_SETTINGS_MAIN);
            break;
        case STATE_DISPLAY:
            // afterStateSwitched() handles setting discoverable
            // When playing, long click goes to settings
            if (playbackStatus == PLAYBACK_PLAYING)
                pause();
            else
                play();
            break;
    }
}

void longClick(Button2& btn)
{
    if (state == STATE_DISPLAY)
    {
        switchStateInstant(STATE_SETTINGS_MAIN);
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
    switch (selectedOption)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
            if (selectedOption < devices.devices.count)
            {
                selectedDevice = selectedOption;
                switchStateInstant(STATE_SETTINGS_DEVICE);
            }
            break;
        case 5:  // Go back
            switchStateInstant(STATE_SETTINGS_MAIN);
    }
}

void deviceListSettings_display()
{
    /*
    Device 1
    Device 2
    Device 3
    VVV

    ^^^
    Device 4
    <empty slot>
    Go back
    */

    lcd.clear();

    int numDevices = devices.devices.count;

    // Page 1
    if (selectedOption < 3)
    {
        for (int i = 0; i < 3; i++)
        {
            lcd.setCursor(1, i);
            // Check if device exists
            if (i < numDevices)
            {
                // Write favourite (it is always #1)
                if (i == 0)
                    lcd.write(ICON_FAVOURITE);
                // Write first couple of characters from device name
                char line[19];  // 18 chars + null
                memcpy(line, devices.devices.deviceNames[i], 18);
                line[18] = 0;

                lcd.print(line);
            }
            else
            {
                lcd.print("<empty slot>");
            }
        }

        lcd.setCursor(1, 3);
        lcd.print("vvv");
    }
    // Page 2
    else
    {
        lcd.setCursor(1, 0);
        lcd.print("^^^");

        for (int i = 0; i < 2; i++)
        {
            lcd.setCursor(1, i + 1);
            int deviceIndex = i + 3;  // We already printed 3 on the first page
            // Check if device exists
            if (deviceIndex < numDevices)
            {
                // Write first couple of characters from device name
                char line[19];  // 18 chars + null
                memcpy(line, devices.devices.deviceNames[deviceIndex], 18);
                line[18] = 0;

                lcd.print(line);
            }
            else
            {
                lcd.print("<empty slot>");
            }
        }

        lcd.setCursor(1, 3);
        lcd.print("Go back");
    }

    drawCursorForSelectedOption();
}



void device_click()
{
    /*
    SETTINGS_DEVICE_CONNECT 0
    SETTINGS_DEVICE_FAVOURITE 1
    SETTINGS_DEVICE_DELETE 2
    SETTINGS_DEVICE_MOVE_UP 3
    SETTINGS_DEVICE_MOVE_DOWN 4
    SETTINGS_DEVICE_GO_BACK 5
    */

    switch (selectedOption)
    {
        case SETTINGS_DEVICE_CONNECT:  // Actually connect/disconnect
            // Check if connected
            if (memcmp(devices.devices.addresses[selectedDevice], devices.devices.connected, 6) == 0)
            {
                disconnect();
                switchStateWithIntermediate(STATE_SETTINGS_DEVICE, STATE_TRANSITION_MESSAGE, 1000);
                displayMessage("Disconnected", nullptr, false);
            }
            else
            {
                disconnect();
                switchStateInstant(STATE_CONNECTING);
                displayMessage("Connecting to", devices.devices.deviceNames[selectedDevice], true);
                // I don't feel like working out the logic for connecting once the radio is back on
                //  so I'll just call setConnectable from here
                delay(DISCONNECT_SET_UNCONNECTABLE_TIME); // Give time to disconnect and turn connection back on
                setConnectable(true);
                delay(250);
                connect(devices.devices.addresses[selectedDevice]);
            }
            break;
        case SETTINGS_DEVICE_FAVOURITE:
            if (devices.devices.favourite != selectedDevice)
            {
                favourite(devices.devices.addresses[selectedDevice]);
                switchStateWithIntermediate(STATE_SETTINGS_DEVICE, STATE_TRANSITION_MESSAGE, 1000);
                displayMessage("Favourited", devices.devices.deviceNames[selectedDevice], true);
            }
            break;
        case SETTINGS_DEVICE_DELETE:
            {
                // If connected, disconnect first
                if (memcmp(devices.devices.addresses[selectedDevice], devices.devices.connected, 6) == 0)
                    disconnect();

                deleteDevice(devices.devices.addresses[selectedDevice]);

                // Go back to main page if this is the last device
                State next = devices.devices.count == 1 ? STATE_SETTINGS_MAIN : STATE_SETTINGS_DEVICE_LIST;
                switchStateWithIntermediate(next, STATE_TRANSITION_MESSAGE, 1500);
                displayMessage("Deleted", devices.devices.deviceNames[selectedDevice], true);
                break;
            }
        case SETTINGS_DEVICE_MOVE_UP:
            {
                // Don't move favourite/top device
                if (selectedDevice == 0)
                    break;

                // Favourite this device if we are at position #2
                if (selectedDevice == 1)
                    favourite(devices.devices.addresses[selectedDevice]);
                else
                    moveUp(devices.devices.addresses[selectedDevice]);
                selectedDevice--;
                switchStateWithIntermediate(STATE_SETTINGS_DEVICE, STATE_TRANSITION_MESSAGE, 1000);
                char message[21];
                sprintf(message, "Moved to pos. %d", selectedDevice + 1);
                displayMessage(message);
                break;
            }
        case SETTINGS_DEVICE_MOVE_DOWN:
            {
                // Don't move favourite or bottom device
                if (selectedDevice == 0 || selectedDevice == 4)
                    break;

                moveDown(devices.devices.addresses[selectedDevice]);
                selectedDevice++;
                switchStateWithIntermediate(STATE_SETTINGS_DEVICE, STATE_TRANSITION_MESSAGE, 1000);
                char message[21];
                sprintf(message, "Moved to pos. %d", selectedDevice + 1);
                displayMessage(message);
                break;
            }
        case SETTINGS_DEVICE_GO_BACK:
            switchStateInstant(STATE_SETTINGS_DEVICE_LIST);
            break;
    }
}

void deviceSettings_display()
{
    /*
    Connect/Disconnect
    Favourite
    Delete
    VVV

    ^^^
    Move up
    Move down
    Go back
    */

    lcd.clear();

    // Make sure device is valid
    if (selectedDevice >= devices.devices.count)
    {
        log_e("selectedDevice was >= devices.devices.count (selected a device that doesn't exist)!");
        switchStateInstant(STATE_SETTINGS_MAIN);
        return;
    }

    // Page 1
    if (selectedOption < 3)
    {
        lcd.setCursor(1, 0);
        // Check if connected
        if (memcmp(devices.devices.addresses[selectedDevice], devices.devices.connected, 6) == 0)
            lcd.print("Disconnect");
        else
            lcd.print("Connect");

        lcd.setCursor(1, 1);
        if (!devices.devices.favourite == selectedDevice)
            lcd.print("Favourite");
        else
            lcd.print("<already favourite>");

        lcd.setCursor(1, 2);
        lcd.print("Delete");

        lcd.setCursor(1, 3);
        lcd.print("vvv");
    }
    // Page 2
    else
    {
        lcd.setCursor(1, 0);
        lcd.print("^^^");

        lcd.setCursor(1, 1);
        // TODO: Favourite device if at index 1
        if (selectedDevice > 0)
            lcd.print("Move up");
        else
            lcd.print("<already at top>");

        lcd.setCursor(1, 2);
        if (selectedDevice == 0)
            lcd.print("<cannot move fav.>");
        else if (selectedDevice == 4)
            lcd.print("<already at bottom>");
        else
            lcd.print("Move down");

        lcd.setCursor(1, 3);
        lcd.print("Go back");
    }

    drawCursorForSelectedOption();
}



void mainSettings_click()
{
    /*
    SETTINGS_MAIN_DEVICE_LIST 0
    SETTINGS_MAIN_PAIR_DEVICE 1
    SETTINGS_MAIN_DISCONNECT 2
    SETTINGS_MAIN_GO_BACK 3
    */

    switch (selectedOption)
    {
        case SETTINGS_MAIN_DEVICE_LIST:
            if (devices.devices.count > 0)
                switchStateInstant(STATE_SETTINGS_DEVICE_LIST);
            break;
        case SETTINGS_MAIN_PAIR_DEVICE:
            // Pair if we have room
            if (devices.devices.count < 5)
            {
                disconnect();
                switchStateInstant(STATE_DISCOVERABLE);
            }
            break;
        case SETTINGS_MAIN_DISCONNECT:
            if (isBDAValid(devices.devices.connected))
            {
                // TODO: See if this activates the "disconnected" callback or not
                disconnect();
                switchStateWithIntermediate(STATE_SETTINGS_MAIN, STATE_TRANSITION_MESSAGE, 1000);
                displayMessage("Disconnected", nullptr, false);
            }
            break;
        case SETTINGS_MAIN_GO_BACK:
            switchStateInstant(isBDAValid(devices.devices.connected) ? STATE_DISPLAY : STATE_IDLE);
            break;
    }
}

void mainSettings_display()
{
    /*
    Device List
    Pair Device (4/5)
    Disconnect
    Go back
    */
    lcd.clear();
    lcd.setCursor(1, 0);  // Leave room for little > display
    if (devices.devices.count > 0)
    {
        lcd.print("Device List (");
        lcd.print(devices.devices.count);
        lcd.print("/5)");
    }
    else
        lcd.print("<no past devices>");

    lcd.setCursor(1, 1);
    if (devices.devices.count == 5)
        lcd.print("<can't pair >5 >");
    else
        lcd.print("Pair New Device");

    lcd.setCursor(1, 2);
    if (isBDAValid(devices.devices.connected))
        lcd.print("Disconnect");
    else
        lcd.print("<not connected>");

    lcd.setCursor(1, 3);
    lcd.print("Go back");

    drawCursorForSelectedOption();
}

void drawCursorForSelectedOption()
{
    // When we have 6 options there are 3 on each page (first 3 then last 3)
    /*
    1
    2
    3
    VVV

    ^^^
    4
    5
    6
    */
    int selectedOnThisPage = selectedOption;
    if (numOptions == 6 && selectedOption > 2)
        selectedOnThisPage = selectedOption % 3 + 1;

    // Print the cursor (or blank) on each line
    for (int i = 0; i < 4; i++)
    {
        lcd.setCursor(0, i);
        lcd.print(selectedOnThisPage == i ? '>' : ' ');
    }
}

void connect(uint8_t* device)
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_DEVICE_CONNECT;
    memcpy(msg.device, device, 6);
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void favourite(uint8_t* device)
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_DEVICE_FAVOURITE;
    memcpy(msg.device, device, 6);
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void moveUp(uint8_t* device)
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_DEVICE_MOVE_UP;
    memcpy(msg.device, device, 6);
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void moveDown(uint8_t* device)
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_DEVICE_MOVE_DOWN;
    memcpy(msg.device, device, 6);
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void deleteDevice(uint8_t* device)
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_DEVICE_DELETE;
    memcpy(msg.device, device, 6);
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void disconnect()
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_DEVICE_DISCONNECT;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));

    //connected = false;
    memset(devices.devices.connected, 0, 6);

    // Turn connection "off" so phone doesn't reconnect immediately
    disconnectTime = millis();
    waitingToSetConnectable = true;
    setConnectable(false);
}

void skipForward()
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_SKIP;
    msg.skipUpdate.forward = true;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void skipBackward()
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_SKIP;
    msg.skipUpdate.reverse = true;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void pause()
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_SKIP;
    msg.skipUpdate.pause = true;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));

    playbackStatus = PLAYBACK_PAUSED;
}

void play()
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_SKIP;
    msg.skipUpdate.play = true;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));

    playbackStatus = PLAYBACK_PLAYING;
}

void setDiscoverable(bool discoverable)
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_SET_DISCOVERABLE;
    msg.discoverable = discoverable;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void setConnectable(bool connectable)
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_SET_CONNECTABLE;
    msg.connectable = connectable;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}