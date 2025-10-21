/*

BluetoothAudio code is huge; it can't run a display but it can do ESP-NOW
This board will communicate via ESP-NOW and drive the display/settings

THIS SKETCH USED FOR DEBUGGING WEIRD ESP-NOW DISCONNECTION

*/

#include <CarComms.h>

CarComms comms(handleCarData);

#ifdef DEBUG_ESP_PORT
#define log_i(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define log_i(...)
#endif


void setup()
{
    delay(1000);

    Serial.begin(115200);

    comms.begin();
    comms.receiveTypeMask = CarDataType::ID_BT_TRACK_UPDATE | CarDataType::ID_BT_INFO;
    log_i("Init comms\n");
}

void loop()
{
    if (Serial.available())
    {
        //read until a terminator. after this point there should only be numbers
        String command = Serial.readString();
        command.trim();
        if (command == "disconnect")
        {
            disconnect();
            log_i("Disconnecting\n");
        }
        else if (command == "pause")
        {
            pause();
            log_i("Pausing\n");
        }
        else if (command == "play")
        {
            play();
            log_i("Playing\n");
        }
    }
}

void handleCarData(CarDataType type, const uint8_t* data, int len)
{
    log_i("Got car data. Type: %d\n", type);
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
    // Turn connection "off" so phone doesn't reconnect immediately
    /*
    disconnectTime = millis();
    waitingToSetConnectable = true;
    setConnectable(false);
    */
    BTTrackUpdateMsg msg;
    msg.type = BTTrackUpdateType::BT_UPDATE_DEVICE_DISCONNECT;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void skipForward()
{
    BTTrackUpdateMsg msg;
    memset(&msg, 0, sizeof(BTTrackUpdateMsg));  // 0-initialize so other skip fields are empty
    msg.type = BTTrackUpdateType::BT_UPDATE_SKIP;
    msg.skipUpdate.forward = true;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void skipBackward()
{
    BTTrackUpdateMsg msg;
    memset(&msg, 0, sizeof(BTTrackUpdateMsg));  // 0-initialize so other skip fields are empty
    msg.type = BTTrackUpdateType::BT_UPDATE_SKIP;
    msg.skipUpdate.reverse = true;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void pause()
{
    BTTrackUpdateMsg msg;
    memset(&msg, 0, sizeof(BTTrackUpdateMsg));  // 0-initialize so other skip fields are empty
    msg.type = BTTrackUpdateType::BT_UPDATE_SKIP;
    msg.skipUpdate.pause = true;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
}

void play()
{
    BTTrackUpdateMsg msg;
    memset(&msg, 0, sizeof(BTTrackUpdateMsg));  // 0-initialize so other skip fields are empty
    msg.type = BTTrackUpdateType::BT_UPDATE_SKIP;
    msg.skipUpdate.play = true;
    comms.send(CarDataType::ID_BT_TRACK_UPDATE, &msg, sizeof(BTTrackUpdateMsg));
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