#include "btAudio.h"
#include <Preferences.h>  // For saving audio source BT addr for auto-reconnect
                          ////////////////////////////////////////////////////////////////////
                          ////////////// Nasty statics for i2sCallback ///////////////////////
                          ////////////////////////////////////////////////////////////////////
float btAudio::_vol = 0.95;
esp_bd_addr_t btAudio::_address;
int32_t btAudio::_sampleRate = 44100;

String btAudio::title = "";
String btAudio::album = "";
String btAudio::artist = "";
String btAudio::sourceDeviceName = "";

bool btAudio::avrcConnected = false;
esp_bd_addr_t btAudio::connectedAddress;

Preferences preferences;

#define PREF_NAMESPACE "bt_devices"
#define PREF_KEY "devices"

////////////////////////////////////////////////////////////////////
////////////////////////// Constructor /////////////////////////////
////////////////////////////////////////////////////////////////////
btAudio::btAudio(const char *devName)
{
    _devName = devName;
}

////////////////////////////////////////////////////////////////////
////////////////// Bluetooth Functionality /////////////////////////
////////////////////////////////////////////////////////////////////
void btAudio::begin()
{

    //Arduino bluetooth initialisation
    btStart();

    // bluedroid allows for bluetooth classic
    esp_bluedroid_init();
    esp_bluedroid_enable();

    //set up device name
    esp_bt_dev_set_device_name(_devName);

    // initialize AVRCP controller
    esp_avrc_ct_init();
    esp_avrc_ct_register_callback(avrc_callback);
    esp_bt_gap_register_callback(gap_cb);

    // this sets up the audio receive
    esp_a2d_sink_init();

    esp_a2d_register_callback(a2d_cb);

    // set discoverable and connectable mode, wait to be connected
#if ESP_IDF_VERSION_MAJOR > 3
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
#else
    esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
#endif
}

void btAudio::end()
{
    esp_a2d_sink_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    btStop();
}

void btAudio::reconnect()
{
    // Load rememebered device address from flash
    preferences.begin(PREF_NAMESPACE, false);
    _address[0] = preferences.getUChar("btaddr0", 0);
    _address[1] = preferences.getUChar("btaddr1", 0);
    _address[2] = preferences.getUChar("btaddr2", 0);
    _address[3] = preferences.getUChar("btaddr3", 0);
    _address[4] = preferences.getUChar("btaddr4", 0);
    _address[5] = preferences.getUChar("btaddr5", 0);
    preferences.end();

    // Only attempt connection if address exists
    if (_address[0] + _address[1] + _address[2] + _address[3] + _address[4] + _address[5] != 0)
    {
        ESP_LOGI("btAudio", "Connecting to remembered BT device: %d %d %d %d %d %d",
                 _address[0], _address[1],
                 _address[2], _address[3],
                 _address[4], _address[5]);
        // Connect to remembered device
        esp_a2d_sink_connect(_address);
    }
}

void btAudio::disconnect()
{
    esp_a2d_sink_disconnect(_address);
}

void btAudio::btConnected(esp_bd_addr_t bda)
{
    esp_bt_gap_read_remote_name(bda);
}

void btAudio::a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    esp_a2d_cb_param_t *a2d = (esp_a2d_cb_param_t *)(param);
    switch (event)
    {
        case ESP_A2D_CONNECTION_STATE_EVT:
            {
                esp_bd_addr_t temp = a2d->conn_stat.remote_bda;
                // TODO: Handle ESP_A2D_CONNECTION_STATE_DISCONNECTED, ESP_A2D_CONNECTION_STATE_DISCONNECTING
                if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
                {
                    _address[0] = *temp;
                    _address[1] = *(temp + 1);
                    _address[2] = *(temp + 2);
                    _address[3] = *(temp + 3);
                    _address[4] = *(temp + 4);
                    _address[5] = *(temp + 5);
                    ESP_LOGI("btAudio", "Connected to BT device: %d %d %d %d %d %d", _address[0], _address[1], _address[2], _address[3], _address[4], _address[5]);

                    btConnected(conn_stat.remote_bda);

                    // Store connected BT address for use by reconnect()
                    preferences.begin("btAudio", false);
                    if (preferences.getUChar("btaddr0", 0) != _address[0])
                    {
                        preferences.putUChar("btaddr0", _address[0]);
                        ESP_LOGI("btAudio", "Writing BTaddr0");
                    }
                    if (preferences.getUChar("btaddr1", 0) != _address[1])
                    {
                        preferences.putUChar("btaddr1", _address[1]);
                        ESP_LOGI("btAudio", "Writing BTaddr1");
                    }
                    if (preferences.getUChar("btaddr2", 0) != _address[2])
                    {
                        preferences.putUChar("btaddr2", _address[2]);
                        ESP_LOGI("btAudio", "Writing BTaddr2");
                    }
                    if (preferences.getUChar("btaddr3", 0) != _address[3])
                    {
                        preferences.putUChar("btaddr3", _address[3]);
                        ESP_LOGI("btAudio", "Writing BTaddr3");
                    }
                    if (preferences.getUChar("btaddr4", 0) != _address[4])
                    {
                        preferences.putUChar("btaddr4", _address[4]);
                        ESP_LOGI("btAudio", "Writing BTaddr4");
                    }
                    if (preferences.getUChar("btaddr5", 0) != _address[5])
                    {
                        preferences.putUChar("btaddr5", _address[5]);
                        ESP_LOGI("btAudio", "Writing BTaddr5");
                    }
                    preferences.end();
                    break;
                }
            }
        case ESP_A2D_AUDIO_CFG_EVT:
            {
                ESP_LOGI("BT_AV", "A2DP audio stream configuration, codec type %d", a2d->audio_cfg.mcc.type);
                // for now only SBC stream is supported
                if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC)
                {
                    _sampleRate = 16000;
                    char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];
                    if (oct0 & (0x01 << 6))
                    {
                        _sampleRate = 32000;
                    }
                    else if (oct0 & (0x01 << 5))
                    {
                        _sampleRate = 44100;
                    }
                    else if (oct0 & (0x01 << 4))
                    {
                        _sampleRate = 48000;
                    }
                    ESP_LOGI("BT_AV", "Configure audio player %x-%x-%x-%x",
                             a2d->audio_cfg.mcc.cie.sbc[0],
                             a2d->audio_cfg.mcc.cie.sbc[1],
                             a2d->audio_cfg.mcc.cie.sbc[2],
                             a2d->audio_cfg.mcc.cie.sbc[3]);
                    if (i2s_set_sample_rates(I2S_NUM_0, _sampleRate) == ESP_OK)
                    {
                        ESP_LOGI("BT_AV", "Audio player configured, sample rate=%d", _sampleRate);
                    }
                }

                break;
            }
        default:
            log_e("a2dp invalid cb event: %d", event);
            break;
    }
}
void btAudio::updateMeta()
{
    if (!avrcConnected)
    {
        log_w("Tried to update metadata while not connected to AVRC");
        return;
    }
    uint8_t attr_mask = ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM;
    esp_avrc_ct_send_metadata_cmd(1, attr_mask);
}
void btAudio::avrc_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(param);
    char *attr_text;
    String mystr;

    switch (event)
    {
        case ESP_AVRC_CT_CONNECTION_STATE_EVT:
            // https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/bluetooth/esp_avrc.html#_CPPv442esp_avrc_ct_send_register_notification_cmd7uint8_t7uint8_t8uint32_t
            avrcConnected = rc->conn_stat.connected;
            memcpy(avrcAddress, rc->conn_stat.remote_bda, 6);

            if (avrcConnected)
            {
                // Ask it to tell us when the time, track etc changes every 1000ms
                uint8_t attr_mask = ESP_AVRC_RN_PLAY_STATUS_CHANGE | ESP_AVRC_RN_TRACK_CHANGE | ESP_AVRC_RN_PLAY_POS_CHANGED;
                esp_avrc_ct_send_register_notification_cmd(1, attr_mask, 200);
            }
            break;
        case ESP_AVRC_CT_METADATA_RSP_EVT:
            {
                attr_text = (char *)malloc(rc->meta_rsp.attr_length + 1);
                memcpy(attr_text, rc->meta_rsp.attr_text, rc->meta_rsp.attr_length);
                attr_text[rc->meta_rsp.attr_length] = 0;
                mystr = String(attr_text);

                switch (rc->meta_rsp.attr_id)
                {
                    case ESP_AVRC_MD_ATTR_TITLE:
                        //Serial.print("Title: ");
                        //Serial.println(mystr);
                        title = mystr;
                        break;
                    case ESP_AVRC_MD_ATTR_ARTIST:
                        //Serial.print("Artist: ");
                        //Serial.println(mystr);
                        artist = mystr;
                        break;
                    case ESP_AVRC_MD_ATTR_ALBUM:
                        //Serial.print("Album: ");
                        //Serial.println(mystr);
                        album = mystr;
                        break;
                }
                free(attr_text);
            }
            break;
        case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
            uint8_t event = rc->change_ntf.event_id;
            switch (event)
            {
                case ESP_AVRC_RN_PLAY_STATUS_CHANGE:
                    // TODO: Message about track being paused (or maybe do nothing?)
                    esp_avrc_playback_stat_t playback = rc->change_ntf.event_parameter.playback;
                    /*
                    ESP_AVRC_PLAYBACK_STOPPED = 0
                    ESP_AVRC_PLAYBACK_PLAYING = 1
                    ESP_AVRC_PLAYBACK_PAUSED = 2
                    ESP_AVRC_PLAYBACK_FWD_SEEK = 3
                    ESP_AVRC_PLAYBACK_REV_SEEK = 4
                    */
                    break;
                case ESP_AVRC_RN_TRACK_CHANGE:
                    // TODO: Send ESP-NOW message as track has changed + update metadata
                    // rc->change_ntf.event_parameter.elm_id is a uint8_t[8], just a song ID used to get metadata
                    break;
                case ESP_AVRC_RN_PLAY_POS_CHANGED:
                    // TODO: Message about play pos
                    uint32_t playPosMS = rc->change_ntf.event_parameter.play_pos;
                    break;
            }
            break;
        default:
            ESP_LOGE("RCCT", "unhandled AVRC event: %d", event);
            break;
    }
}

void btAudio::gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
        case ESP_BT_GAP_READ_REMOTE_NAME_EVT:
            if (param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGI("RCCT", "Remote device name: %s", param->read_rmt_name.rmt_name);

                // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/esp_gap_bt.html#_CPPv4N21esp_bt_gap_cb_param_t19read_rmt_name_param8rmt_nameE
                sourceDeviceName = String(param->read_rmt_name.rmt_name);
                // TODO: Send message?
            }
            else
            {
                ESP_LOGW("RCCT", "Failed to read remote device name");
            }
            break;
        default:
            ESP_LOGI("RCCT", "GAP event: %d", event);
            break;
    }
}

void btAudio::setSinkCallback(void (*sinkCallback)(const uint8_t *data, uint32_t len))
{
    esp_a2d_sink_register_data_callback(sinkCallback);
}
////////////////////////////////////////////////////////////////////
////////////////// I2S Audio Functionality /////////////////////////
////////////////////////////////////////////////////////////////////
void btAudio::I2S(int bck, int dout, int ws)
{
    // i2s configuration
    uint32_t usamplerate = (uint32_t)_sampleRate;
    static const i2s_config_t i2s_config = {
        .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = usamplerate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
#if ESP_IDF_VERSION_MAJOR > 3
        .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB),
#else
        .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
#endif
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // default interrupt priority
        .dma_buf_count = 3,
        .dma_buf_len = 600,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    // i2s pinout
    static const i2s_pin_config_t pin_config = {
        .bck_io_num = bck,     //26
        .ws_io_num = ws,       //27
        .data_out_num = dout,  //25
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    // now configure i2s with constructed pinout and config
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);

    // Sets the function that will handle data (i2sCallback)
    esp_a2d_sink_register_data_callback(i2sCallback);
}
void btAudio::i2sCallback(const uint8_t *data, uint32_t len)
{
    size_t i2s_bytes_write = 0;
    int16_t *data16 = (int16_t *)data;  //playData doesnt want const
    int16_t fy[2];
    float temp;

    int jump = 4;        //how many bytes at a time get sent to buffer
    int n = len / jump;  // number of byte chunks

    for (int i = 0; i < n; i++)
    {
        //process left channel
        fy[0] = (int16_t)((*data16) * _vol);
        data16++;

        // process right channel
        fy[1] = (int16_t)((*data16) * _vol);
        data16++;
        i2s_write(I2S_NUM_0, fy, jump, &i2s_bytes_write, 100);
    }
}

void btAudio::volume(float vol)
{
    _vol = constrain(vol, 0, 1);
}



void btAudio::saveDevices(const PairedDevices* devices)
{
    preferences.begin(PREF_NAMESPACE, false);
    preferences.putBytes(PREF_KEY, devices, sizeof(PairedDevices));
    preferences.end(); 
}

void btAudio::loadDevices(PairedDevices* devices)
{
    preferences.begin(PREF_NAMESPACE, true);
    // Check if there area no saved devices
    if (!preferences.isKey(PREF_KEY))
    {
        // Save a blank copy
        preferences.end();
        preferences.begin(PREF_NAMESPACE, false); // Open in r/w mode
        PairedDevices empty = {0};
        preferences.putBytes(PREF_KEY, empty, sizeof(PairedDevices));
    }
    preferences.getBytes(PREF_KEY, devices, sizeof(PairedDevices));
    preferences.end(); 
}

/*

typedef struct PairedDevices {
    esp_bd_addr_t addresses[MAX_PAIRED_DEVICES]; // 5 x 6 = 30 bytes
    char deviceNames[MAX_PAIRED_DEVICES][MAX_DEVICE_NAME_LENGTH]; // 5 x 32 = 160
    uint8_t count;
    uint8_t favourite;
} PairedDevices;

*/

void btAudio::addOrUpdateDevice(PairedDevices* devices, esp_bd_addr_t bda, const char* deviceName, int nameLen)
{
    if (!devices)
        return;

    // Check if we are updating or adding
    uint8_t deviceIndex = getDeviceIndex(devices, bda);

    // Index is 255 if device doesn't exist
    if (deviceIndex == 255)
    {
        // Try to put at end of list if it isn't full
        if (devices->count => MAX_PAIRED_DEVICES)
        {
            log_w("Tried to update/add device, but it wasn't in devices list and we are at the max paired devices");
            return;
        }

        // We are adding a new device to end of list, so increment count
        deviceIndex = devices->count;
        devices->count++;
    }

    // Copy address and name into the proper slot
    memcpy(devices->addresses[deviceIndex], bda, sizeof(esp_bd_addr_t));
    int len = nameLen > MAX_DEVICE_NAME_LENGTH ? MAX_DEVICE_NAME_LENGTH : nameLen;
    memcpy(devices->deviceNames[deviceIndex], deviceName, len);

    saveDevices(devices);
}

uint8_t btAudio::getDeviceIndex(const PairedDevices* devices, esp_bd_addr_t bda)
{
    if (!devices || devices->count == 0)
        return 255;

    for (uint8_t i = 0; i < devices->count; i++)
    {
        // Check if addresses match
        if (memcmp(devices->addresses[i], bda, sizeof(esp_bd_addr_t)) == 0)
            return i;
    }
    return 255;
}

void btAudio::swapDevices(PairedDevices* devices, uint8_t a, uint8_t b)
{
    esp_bd_addr_t swapBda;
    char swapName[MAX_DEVICE_NAME_LENGTH];

    // Swap addresses
    memcpy(swapBda, devices->addresses[a], sizeof(esp_bd_addr_t));
    memcpy(devices->addresses[a], devices->addresses[b], sizeof(esp_bd_addr_t));
    memcpy(devices->addresses[b], swapBda, sizeof(esp_bd_addr_t));

    // Swap names
    memcpy(swapName, devices->deviceNames[a], MAX_DEVICE_NAME_LENGTH);
    memcpy(devices->deviceNames[a], devices->deviceNames[b], MAX_DEVICE_NAME_LENGTH);
    memcpy(devices->deviceNames[b], swapName, MAX_DEVICE_NAME_LENGTH);

    // Swap favourite, if applicable
    if (devices->favourite == a)
        devices->favourite = b;
    else if (devices->favourite == b)
        devices->favourite = a;
}

void btAudio::moveDeviceUp(PairedDevices* devices, esp_bd_addr_t bda)
{
    uint8_t deviceIndex = getDeviceIndex(devices, bda);
    // Return if device not found in list
    if (deviceIndex == 255)
    {
        log_w("Tried to delete device, but couldn't find it in devices list");
        return;
    }

    // Check if device is already #1 (favourite) or #2 (first paired after favourite)
    if (deviceIndex <= 1)
        return;

    swapDevices(devices, deviceIndex, deviceIndex - 1);
    saveDevices(devices);
}

void btAudio::moveDeviceDown(PairedDevices* devices, esp_bd_addr_t bda)
{
    uint8_t deviceIndex = getDeviceIndex(devices, bda);
    // Return if device not found in list
    if (deviceIndex == 255)
    {
        log_w("Tried to delete device, but couldn't find it in devices list");
        return;
    }

    // Check if device is already last or is the favourite (fav is always at the top)
    if (deviceIndex == devices->count - 1 || deviceIndex == devices->favourite)
        return;

    swapDevices(devices, deviceIndex, deviceIndex + 1);
    saveDevices(devices);
}

void btAudio::deleteDevice(PairedDevices* devices, esp_bd_addr_t bda)
{
    uint8_t deviceIndex = getDeviceIndex(devices, bda);
    // Return if device not found in list
    if (deviceIndex == 255)
    {
        log_w("Tried to delete device, but couldn't find it in devices list");
        return;
    }

    // Move every device up 1 (down one index). Make sure we don't copy from out of bounds
    for (int i = deviceIndex; i < MAX_PAIRED_DEVICES - 2; i++)
    {
        memcpy(devices->addresses[i], devices->addresses[i+1], sizeof(esp_bd_addr_t));
        memcpy(devices->deviceNames[i], devices->deviceNames[i+1], MAX_DEVICE_NAME_LENGTH);
    }

    // Don't worry about zero-ing the last one, decreasing the count should avoid us fiddling with it
    devices->count--;
    saveDevices(devices);
}

void btAudio::favouriteDevice(PairedDevices* devices, esp_bd_addr_t bda)
{
    uint8_t deviceIndex = getDeviceIndex(devices, bda);
    // Return if device not found in list
    if (deviceIndex == 255)
    {
        log_w("Tried to favourite device, but couldn't find it in devices list");
        return;
    }

    // Not the most efficient, but keep swapping up until we reach the top (lowest index)
    while (deviceIndex > 0)
        swapDevices(devices, deviceIndex, deviceIndex--);

    devices->favourite = 0;
    saveDevices(devices);
}
