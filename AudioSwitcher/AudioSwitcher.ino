/*

Audio Switcher

Outline:
- ESP8266 with a rotary encoder and 2 audio multiplexers (CD4052B)
- Rotary encoder selects audio source
- AUX multiplexer feeds into main multiplexer
- Display audio source on OLED display
- All outputs go through car's AUX port

ESP8266 Wiring:

D3 (GPIO0): Left side rotary encoder
D4 (GPIO2): Right side rotary encoder
GND: Middle rotary encoder

D1 (GPIO5): SCL (I2C)
D2 (GPIO4): SDA (I2C)
 
D5 (GPIO14): Main audio select A
D6 (GPIO12): Main audio select B
D7 (GPIO13): AUX audio select A
D8 (GPIO15): AUX audio select B

Audio sources:
 - 1: Bluetooth
 - 2: AUX port
 - 3: Lightning Cable
 - 4: USB-C Cable
 - 5: Carputer
 - 6-8: Reserved

Main audio select:
 - Bluetooth
 - AUX
 - Carputer

AUX audio select:
 - AUX port
 - Lightning
 - USB-C

*/

#include <CarComms.h>

// States for pins D6, D5, D8, D7
// D5 and D6 are main audio select, D7 and D8 are aux audio select
// Order swapped because A pins (D6 and D8) are the "low" order bits
const int audioPinStates[5][4] = {
    { LOW, LOW, LOW, LOW },    // Bluetooth - main 0
    { LOW, HIGH, LOW, LOW },   // AUX port - aux 0
    { LOW, HIGH, LOW, HIGH },  // Lightning - aux 1
    { LOW, HIGH, HIGH, LOW },  // USB-C - aux 2
    { HIGH, LOW, LOW, LOW }    // Carputer - main 1
};

#define AUDIO_MAIN_A D5
#define AUDIO_MAIN_B D6
#define AUDIO_AUX_A D7
#define AUDIO_AUX_B D8

int audioSource;

CarComms comms(handleCarData);

void setup()
{
    // Setup audio chip pins
    pinMode(AUDIO_MAIN_A, OUTPUT);
    pinMode(AUDIO_MAIN_B, OUTPUT);
    pinMode(AUDIO_AUX_A, OUTPUT);
    pinMode(AUDIO_AUX_B, OUTPUT);

    comms.begin();
    comms.receiveTypeMask = CarDataType::ID_AUDIO_SOURCE;
}

void loop()
{
}

void updateSelectedAudioPins()
{
    digitalWrite(AUDIO_MAIN_B, audioPinStates[audioSource][0]);
    digitalWrite(AUDIO_MAIN_A, audioPinStates[audioSource][1]);
    digitalWrite(AUDIO_AUX_B, audioPinStates[audioSource][2]);
    digitalWrite(AUDIO_AUX_A, audioPinStates[audioSource][3]);
}

void handleCarData(CarDataType type, const uint8_t* data, int len)
{
    AudioSourceMsg* msg = (AudioSourceMsg*)data;
    audioSource = msg->audioSource;
    updateSelectedAudioPins();
}
