#include "Arduino.h"

// brownout
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "./const.h"

//#define SERIAL_DEBUG

#if defined(SERIAL_DEBUG)
#define DBG(x) Serial.println(x)
#else
#define DBG(...)
#endif

#include "time.h"
#include "./connect.h"
#include "./ftp.h"
#include "./cam.h"

#define SEND_INTERVAL 5000
#define WIFI_CONNECT_INTERVAL 30000


const char* ntpServer = "pool.ntp.org";
bool timeInited = false;


void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // disable brownout detector

    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);

#ifdef SERIAL_DEBUG
    Serial.begin(115200);
    Serial.setDebugOutput(true);
#endif

    wifiInit();
    wifiConnect();
}

void initTime()
{
    configTime(0, 0, ntpServer);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }

    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
    tzset();
    timeInited = true;
}

void loop()
{
    unsigned long current_millis = millis();

    if (wifi_connected)
    {
        if (!timeInited)
            initTime();

        if (current_millis - send_timer >= SEND_INTERVAL)
        {
            send_timer = current_millis;

            if (!cam_init_ok)
            {
                cam_init_ok = cameraInit();
            }

            if (cam_init_ok)
            {
                takePicture();
            }
        }
    }
    else
    {
        if (current_millis - connect_timer >= WIFI_CONNECT_INTERVAL)
        {
            connect_timer = current_millis;
            wifiConnect();
        }
    }

    yield();
}
