#ifndef PYDUINO_H
#define PYDUINO_H

#include <Arduino.h>
//#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <string.h>
#include "FunctionHeader.h"

// Constants
#define PY_DigitalWrite 1
#define PY_DigitalRead 2
#define PY_AnalogWrite 3
#define PY_AnalogRead 4
#define PY_Tone 5
#define PY_NoTone 6

#define DEFAULT_MESSAGE_RETURN 0
#define MESSAGE_RETURN 253
#define MESSAGE_START 254
#define MESSAGE_END 255

#define REC_BUF_SIZE 64
#define PORT_SIZE 19

//#define LCD_I2C_PORT 0x27

class Pyduino
{
  private:
    static byte recieveBuffer[REC_BUF_SIZE];
    static byte portStates[PORT_SIZE];
    //static LiquidCrystal_I2C lcd;

    // Internal message stuff
    static void HandleCommand(int len);
    static inline long NextArg() __attribute__((always_inline));
    static inline float NextArg_Float() __attribute__((always_inline));
    static inline String NextArg_String() __attribute__((always_inline));
    static inline void SendResponse(long response) __attribute__((always_inline));
    static inline void InitPort(long port, int state) __attribute__((always_inline));

    static int readPos;
    
  public:
    // User interface
    static void Init();
    static void Tick();
    static MessageCallback CustomMessageCallback;
    static long GetInt(byte* buf, int& readPos);
    static String GetString(byte* buf, int& strRead);
    
};

// Convertors from bytes to values
union Convertor {
   long intVal;
   byte bytes[4];
};

union Convertor_Float {
   long intVal;
   float floatVal;
};


#endif
