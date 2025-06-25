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

class btAudio {
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
	
  private:
    const char *_devName;
    static int32_t  _sampleRate;
	
	// static function causes a static infection of variables
	static void i2sCallback(const uint8_t *data, uint32_t len);
	static void a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
	static void avrc_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
	static void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
	
	// bluetooth address of connected device
	static uint8_t _address[6];
	static float _vol;
};



#endif
