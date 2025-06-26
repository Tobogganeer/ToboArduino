#include <btAudio.h>
#include <CarComms.h>
#include "esp_timer.h"

// Sets the name of the audio device
btAudio audio = btAudio("Quandale Dingle's Whip");
CarComms comms(handleCarData);

esp_timer_handle_t refreshMetadataTimer;
esp_timer_handle_t sendDeviceInfoTimer;

#define METADATA_REFRESH_TIME_MS 1000
#define SEND_DEVICE_INFO_TIME_MS 5000

// https://www.youtube.com/watch?v=QixtxaAda18

//#define DEBUG

void setup()
{
#ifdef DEBUG
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_INFO);
#endif

    // Streams audio data to the ESP32
    audio.begin();
    audio.setDiscoverable(false); // Set non-discoverable by default - user will go into settings and pair new device

    // Tries reconnecting to last 5 connected devices
    audio.reconnect();

    int ws = 16;
    int dout = 17;
    int bck = 5;
    audio.I2S(bck, dout, ws);

    comms.begin();
    comms.receiveTypeMask = CarDataType::ID_BT_TRACK_UPDATE;

    // Start timer to refresh metadata
    esp_timer_create_args_t timerArgs = {
        .callback = &refreshMetadata,
        .name = "refreshMetadataTimer"
    };
    esp_timer_create(&timerArgs, &refreshMetadataTimer);
    log_i("Start meta refresh timer");
    esp_timer_start_periodic(refreshMetadataTimer, METADATA_REFRESH_TIME_MS * 1000); // Units are us

    esp_timer_create_args_t deviceInfoTimerArgs = {
        .callback = &sendDeviceInfo,
        .name = "sendDeviceInfoTimer"
    };
    esp_timer_create(&deviceInfoTimerArgs, &sendDeviceInfoTimer);
    esp_timer_start_periodic(sendDeviceInfoTimer, SEND_DEVICE_INFO_TIME_MS * 1000); // Units are us

    // "Subscribe" to callbacks
    audio.devicesSavedCallback = devicesSavedCallback;
    audio.connectedCallback = connectedCallback;
    audio.disconnectedCallback = disconnectedCallback;
    audio.metadataUpdatedCallback = metadataUpdatedCallback;
    audio.playStatusChangedCallback = playStatusChangedCallback;
    audio.trackChangedCallback = trackChangedCallback;
    audio.playPositionChangedCallback = playPositionChangedCallback;
}

void refreshMetadata(void* arg)
{
    // It will give us the metadataUpdatedCallback when complete
    log_i("Refresh metadata. AVRC %s", audio.avrcConnected ? "connected" : "not connected");
    if (audio.avrcConnected)
        audio.updateMeta();
}

void sendDeviceInfo(void* arg)
{
    // Send all device info occasionally in case the controls module initializes a bit later (and to keep it up to date)
    PairedDevices devices;
    audio.loadDevices(&devices);
    devicesSavedCallback(&devices);
}

void devicesSavedCallback(const PairedDevices* devices)
{
    // Send device list out when saved (they are saved whenever modified)
    BTInfoMsg msg;
    msg.type = BTInfoType::BT_INFO_DEVICES;
    memcpy(&msg.devices, devices, sizeof(PairedDevices));
    bool success = comms.send(CarDataType::ID_BT_INFO, (uint8_t*)&msg, sizeof(BTInfoMsg));
    log_i("Sent devices list. Success? %s", success ? "true" : "false");
}

void connectedCallback(const esp_bd_addr_t bda, const char* deviceName, int nameLen)
{
    log_i("Sending Connected message");
    BTInfoMsg msg;
    msg.type = BTInfoType::BT_INFO_CONNECTED;
    memcpy(&msg.sourceDevice.address, &bda, sizeof(esp_bd_addr_t));
    memcpy(&msg.sourceDevice.deviceName, deviceName, min(nameLen, 32));
    msg.sourceDevice.deviceName[31] = 0; // Null-terminate last character (name limit is 32 chars);
    comms.send(CarDataType::ID_BT_INFO, (uint8_t*)&msg, sizeof(BTInfoMsg));
}

void disconnectedCallback(const esp_bd_addr_t bda, const char* deviceName, int nameLen)
{
    BTInfoMsg msg;
    msg.type = BTInfoType::BT_INFO_DISCONNECTED;
    memcpy(&msg.sourceDevice.address, &bda, sizeof(esp_bd_addr_t));
    memcpy(&msg.sourceDevice.deviceName, deviceName, min(nameLen, 32));
    msg.sourceDevice.deviceName[31] = 0; // Null-terminate last character (name limit is 32 chars);
    comms.send(CarDataType::ID_BT_INFO, (uint8_t*)&msg, sizeof(BTInfoMsg));
}

void copyMetadataString(uint8_t* dst, String src)
{
    uint maxLength = BT_SONG_INFO_MAX_STR_LEN - 1; // Leave room at the end
    int len = min(src.length(), maxLength);
    memcpy(dst, src.c_str(), len);
    dst[len] = 0; // Null-terminate
}

void metadataUpdatedCallback()
{
    BTInfoMsg msg;
    msg.type = BTInfoType::BT_INFO_METADATA;

    copyMetadataString((uint8_t*)&msg.songInfo.title, audio.title);
    copyMetadataString((uint8_t*)&msg.songInfo.artist, audio.artist);
    copyMetadataString((uint8_t*)&msg.songInfo.album, audio.album);
    msg.songInfo.trackLengthMS = audio.totalTrackDurationMS;

    comms.send(CarDataType::ID_BT_INFO, (uint8_t*)&msg, sizeof(BTInfoMsg));
}

void playStatusChangedCallback(esp_avrc_playback_stat_t status)
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_SONG_POS;
    msg.songUpdate.updateType = BTTrackSongPosUpdateType::BT_SONG_POS_UPDATE_PLAY_STATUS_CHANGE;
    msg.songUpdate.playback = status;

    comms.send(CarDataType::ID_BT_TRACK_UPDATE, (uint8_t*)&msg, sizeof(BTTrackUpdateMsg));
}

void trackChangedCallback()
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_SONG_POS;
    msg.songUpdate.updateType = BTTrackSongPosUpdateType::BT_SONG_POS_UPDATE_TRACK_CHANGE;

    comms.send(CarDataType::ID_BT_TRACK_UPDATE, (uint8_t*)&msg, sizeof(BTTrackUpdateMsg));
}

void playPositionChangedCallback(uint32_t playPosMS)
{
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_SONG_POS;
    msg.songUpdate.updateType = BTTrackSongPosUpdateType::BT_SONG_POS_UPDATE_PLAY_POS_CHANGED;
    msg.songUpdate.playPosMS = playPosMS;

    comms.send(CarDataType::ID_BT_TRACK_UPDATE, (uint8_t*)&msg, sizeof(BTTrackUpdateMsg));
}





void loop()
{
}

void handleCarData(CarDataType type, const uint8_t* data, int len)
{
    if (type != CarDataType::ID_BT_TRACK_UPDATE)
    {
        log_e("Handled message that wasn't a BT track update (controls, etc)!");
        return;
    }

    BTTrackUpdateMsg* msg = (BTTrackUpdateMsg*)data;
    PairedDevices devices;
    audio.loadDevices(&devices);

    switch (msg->type)
    {
        case BT_UPDATE_SKIP:
            if (msg->skipUpdate.forward)
                audio.next();
            else if (msg->skipUpdate.reverse)
                audio.previous();
            else if (msg->skipUpdate.pause)
                audio.pause();
            else if (msg->skipUpdate.play)
                audio.play();
            break;
        case BT_UPDATE_DEVICE_MOVE_UP:
            audio.moveDeviceUp(&devices, msg->device);
            break;
        case BT_UPDATE_DEVICE_MOVE_DOWN:
            audio.moveDeviceDown(&devices, msg->device);
            break;
        case BT_UPDATE_DEVICE_FAVOURITE:
            audio.favouriteDevice(&devices, msg->device);
            break;
        case BT_UPDATE_DEVICE_DELETE:
            audio.deleteDevice(&devices, msg->device);
            break;
        case BT_UPDATE_SET_DISCOVERABLE:
            audio.setDiscoverable(msg->discoverable);
            break;
    }
}

// comms.send(CarDataType::ID_BUZZER, (byte*)&buzz, sizeof(BuzzerMsg));