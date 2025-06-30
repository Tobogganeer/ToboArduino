// This is just to separate out the files - obviously won't work unless included by BluetoothController.ino

#define ICON_SONG 0
#define ICON_ARTIST 1
#define ICON_ALBUM 2
#define ICON_FAVOURITE 3
#define ICON_TIME 4

uint8_t songIcon[8] = {
    0b00000,
    0b01100,
    0b01011,
    0b01001,
    0b01001,
    0b11011,
    0b11011,
    0b00000,
};

uint8_t artistIcon[8] = {
    0b01110,
    0b11111,
    0b11111,
    0b01110,
    0b00000,
    0b01110,
    0b11111,
    0b00000,
};

uint8_t albumIcon[8] = {
    0b00000,
    0b01110,
    0b11111,
    0b11011,
    0b11111,
    0b01110,
    0b00000,
    0b00000,
};

uint8_t favouriteIcon[8] = {
    0b00000,
    0b11011,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000,
};

uint8_t timeIcon[8] = {
    0b00000,
    0b01110,
    0b10101,
    0b10111,
    0b10001,
    0b01110,
    0b00000,
    0b00000,
};

void initIcons()
{
    lcd.createChar(ICON_SONG, songIcon);
    lcd.createChar(ICON_ARTIST, artistIcon);
    lcd.createChar(ICON_ALBUM, albumIcon);
    lcd.createChar(ICON_FAVOURITE, favouriteIcon);
    lcd.createChar(ICON_TIME, timeIcon);
}