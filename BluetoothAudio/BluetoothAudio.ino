#include <btAudio.h>
#include <CarComms.h>
#include "esp_timer.h"

// Sets the name of the audio device
btAudio audio = btAudio("Quandale Dingle's Whip");
CarComms comms(handleCarData);

esp_timer_handle_t refreshMetadataTimer;

#define METADATA_REFRESH_TIME_MS 1000

// https://www.youtube.com/watch?v=QixtxaAda18

void setup()
{
    // Streams audio data to the ESP32
    audio.begin();

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
    esp_timer_start_periodic(&refreshMetadataTimer, METADATA_REFRESH_TIME_MS * 1000); // Units are us

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
    if (audio.avrcConnected)
        audio.updateMeta();
}

static void devicesSavedCallback(const PairedDevices* devices)
{
    BTInfoMsg msg;
    //msg.type = BTInfoType::
}

static void connectedCallback(const esp_bd_addr_t bda, const char* deviceName, int nameLen)
{

}

static void disconnectedCallback(const esp_bd_addr_t bda, const char* deviceName, int nameLen)
{

}

static void metadataUpdatedCallback()
{

}

static void playStatusChangedCallback(esp_avrc_playback_stat_t status)
{

}

static void trackChangedCallback()
{

}

static void playPositionChangedCallback(uint32_t playPosMS)
{

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
    }
}

// comms.send(CarDataType::ID_BUZZER, (byte*)&buzz, sizeof(BuzzerMsg));