#ifndef BTAUDIO_H
#define BTAUDIO_H

#include "Arduino.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "driver/i2s.h"
#include "esp_avrc_api.h"

#define MAX_PAIRED_DEVICES 5
#define MAX_DEVICE_NAME_LENGTH 32

typedef struct PairedDevices {
    esp_bd_addr_t addresses[MAX_PAIRED_DEVICES]; // 5 x 6 = 30 bytes
    char deviceNames[MAX_PAIRED_DEVICES][MAX_DEVICE_NAME_LENGTH]; // 5 x 32 = 160
    uint8_t count;
    uint8_t favourite;
    esp_bd_addr_t connected;
} PairedDevices;

typedef struct PlaybackStatus {
    uint32_t trackLengthMS;
    uint32_t playPosMS;
    esp_avrc_playback_stat_t playStatus;
    bool playing;
} PlaybackStatus;

class btAudio
{
  public:
    //Constructor
    btAudio(const char *devName);

    // Bluetooth functionality
    void begin();
    void end();
    void connect(esp_bd_addr_t bda);
    void disconnect();
    void reconnect();
    void setSinkCallback(void (*sinkCallback)(const uint8_t *data, uint32_t len));
    void setDiscoverable(bool discoverable);
    void updatePlayStatus();

    // I2S Audio
    void I2S(int bck, int dout, int ws);
    void volume(float vol);

    // meta data
    void updateMeta();

    static void (*devicesSavedCallback)(const PairedDevices* devices);
    static void (*connectedCallback)(const esp_bd_addr_t bda, const char* deviceName, int nameLen);
    static void (*disconnectedCallback)(const esp_bd_addr_t bda, const char* deviceName, int nameLen);
    static void (*metadataUpdatedCallback)();
    static void (*playStatusChangedCallback)(esp_avrc_playback_stat_t status);
    static void (*trackChangedCallback)();
    static void (*playPositionChangedCallback)(uint32_t playPosMS);
    static void (*playStatusCallback)(const PlaybackStatus* status);

    static String title;
    static String artist;
    static String album;
    static uint32_t totalTrackDurationMS;
    static uint32_t currentTrackPosMS;
    static String sourceDeviceName;

    static bool avrcConnected;
    static esp_bd_addr_t avrcAddress;

    void play();
    void pause();
    void next();
    void previous();

    static void saveDevices(const PairedDevices* devices);
    static void loadDevices(PairedDevices* devices);
    static void addOrUpdateDevice(PairedDevices* devices, esp_bd_addr_t bda, const char* deviceName, int nameLen);
    static uint8_t getDeviceIndex(const PairedDevices* devices, esp_bd_addr_t bda);
    static void moveDeviceUp(PairedDevices* devices, esp_bd_addr_t bda);
    static void moveDeviceDown(PairedDevices* devices, esp_bd_addr_t bda);
    static void deleteDevice(PairedDevices* devices, esp_bd_addr_t bda);
    static void favouriteDevice(PairedDevices* devices, esp_bd_addr_t bda);
    static bool isReconnecting() { return reconnecting; }

  private:
    const char* _devName;
    static int32_t _sampleRate;

    static void tryReconnectNextDevice();
    static void reconnectTimeoutCB(void*);

    // static function causes a static infection of variables
    static void i2sCallback(const uint8_t *data, uint32_t len);
    static void a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
    static void avrc_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
    static void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

    static void swapDevices(PairedDevices* devices, uint8_t a, uint8_t b);

    // bluetooth address of connected device
    static uint8_t _address[6];
    static float _vol;
    static bool reconnecting;

    static uint8_t tl;
    static uint8_t nextTL();
};



#endif
