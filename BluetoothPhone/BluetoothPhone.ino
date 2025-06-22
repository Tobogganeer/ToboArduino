/*
 * This sketch is based in part on a keypad demo from:
 * http://www.circuitbasics.com/how-to-set-up-a-keypad-on-an-arduino/
 * 
 * Keypad library:
 * https://playground.arduino.cc/uploads/Code/keypad/index.zip
 * 
 * Tone library:
 * http://downloads.arduino.cc/libraries/github.com/bhagman/Tone-1.0.0.zip
 * 
 * This input/output demo generates DTMF tones corresponding to keypad keys pressed.
 * LM - September 2019
 */



/*

Pin layout for phone keypad:

1+5 = *
1+7 = #
1+8 = 0
1+9 = flash
2+5 = 7
2+6 = store
2+7 = 9
2+8 = 8
3+5 = 4
3+6 = mem
3+7 = 6
3+8 = 5
4+5 = 1
4+6 = redial
4+7 = 3
4+8 = 2


Pin 1: 
Pin 2: 
Pin 3: 
Pin 4: 
Pin 5: 
Pin 6: 
Pin 7: 



=== Top buttons ===
Row 1:
Row 2:
Row 3:
Row 4:
Row 5:
=== Bottom Buttons ===

Col 1:
Col 2:
Col 3:
Col 4:
=== Only for top row ===

*/



#include <Keypad.h>

// Uncomment next line to enable serial console debugging.
#define DEBUG 9600


const byte ROWS = 5;
const byte COLS = 4;

char keyMatrix[ROWS][COLS] = {
    { 'R', 'M', 'S', 'F' },
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 19, 18, 5, 17, 16 };
byte colPins[COLS] = { 4, 0, 2, 15 };


//byte rowPins[ROWS] = { 16, 4, 0, 2, 15 };
//byte colPins[COLS] = { 19, 18, 5, 17 };


// Tone frequencies
const int dtmfRow[ROWS] = { 697, 770, 852, 941 };
const int dtmfCol[COLS] = { 1209, 1336, 1477, 1633 };

const int ROW_TONE_DIO = 22;
const int COL_TONE_DIO = 23;

//ToneESP32 rowPlayer(ROW_TONE_DIO, 0);
//ToneESP32 colPlayer(COL_TONE_DIO, 0);
#define PWM_RESOLUTION 12

Keypad myKeypad = Keypad(makeKeymap(keyMatrix), rowPins, colPins, ROWS, COLS);

const long TONE_DURATION = 250;   // Milliseconds
const long TEST_DURATION = 4000;  // For tweaking waveform

void setup()
{
#ifdef DEBUG
    Serial.begin(DEBUG);
#endif
    ledcAttach(ROW_TONE_DIO, 0, PWM_RESOLUTION);
    ledcAttach(COL_TONE_DIO, 0, PWM_RESOLUTION);
}

void loop()
{
    char keyPressed = myKeypad.getKey();

    if (keyPressed)
    {
#ifdef DEBUG
        Serial.print(keyPressed);
        if (keyPressed == '#')
            Serial.println();
#endif
        // Generate corresponding DTMF tone
        dtmfON(keyPressed);
        delay(TONE_DURATION);
        dtmfOFF();
    }
}

void dtmfOFF()
{
    ledcWriteTone(ROW_TONE_DIO, 0);
    ledcWriteTone(COL_TONE_DIO, 0);
    return;
}

void dtmfON(char c)
{
#ifdef DEBUG
    Serial.println();
    Serial.println("In dtmfON() - c = '" + String(c) + "'");
#endif
    int row = -1;
    int col = -1;
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            if (c == keyMatrix[i][j])
            {
                row = i;
                col = j;
                break;
            }
        }
        if (row >= 0)
            break;
    }
    if (row < 0)  // Unencodeable character
        return;
        // Row and column tones
#ifdef DEBUG
    Serial.print(" Row tone: " + String(dtmfRow[row]));
    Serial.println("Hz,  Col tone: " + String(dtmfCol[col]) + "Hz");
#endif
    ledcWriteTone(ROW_TONE_DIO, dtmfRow[row]);
    ledcWriteTone(COL_TONE_DIO, dtmfCol[col]);
}
