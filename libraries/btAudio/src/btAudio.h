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
} PairedDevices;

class btAudio
{
  public:
    //Constructor
    btAudio(const char *devName);

    // Bluetooth functionality
    void begin();
    void end();
    void disconnect();
    void reconnect();
    void setSinkCallback(void (*sinkCallback)(const uint8_t *data, uint32_t len));

    // I2S Audio
    void I2S(int bck, int dout, int ws);
    void volume(float vol);

    // meta data
    void updateMeta();

    static String title;
    static String artist;
    static String album;
    static String sourceDeviceName;

    static bool avrcConnected;
    static esp_bd_addr_t avrcAddress;

    static void saveDevices(const PairedDevices* devices);
    static void loadDevices(PairedDevices* devices);
    static void addOrUpdateDevice(PairedDevices* devices, esp_bd_addr_t bda, const char* deviceName, int nameLen);
    static uint8_t getDeviceIndex(const PairedDevices* devices, esp_bd_addr_t bda);
    static void moveDeviceUp(PairedDevices* devices, esp_bd_addr_t bda);
    static void moveDeviceDown(PairedDevices* devices, esp_bd_addr_t bda);
    static void deleteDevice(PairedDevices* devices, esp_bd_addr_t bda);
    static void favouriteDevice(PairedDevices* devices, esp_bd_addr_t bda);

  private:
    const char* _devName;
    static int32_t _sampleRate;

    // static function causes a static infection of variables
    static void i2sCallback(const uint8_t *data, uint32_t len);
    static void a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
    static void avrc_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
    static void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

    static void btConnected(uint8_t *bda);

    static void swapDevices(PairedDevices* devices, uint8_t a, uint8_t b);

    // bluetooth address of connected device
    static uint8_t _address[6];
    static float _vol;
};



#endif
