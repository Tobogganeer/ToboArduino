#include "Pyduino.h"

// Init values
byte Pyduino::recieveBuffer[REC_BUF_SIZE] = {0};
byte Pyduino::portStates[PORT_SIZE] = {255};
int Pyduino::readPos = 0;
MessageCallback Pyduino::CustomMessageCallback = NULL;
//LiquidCrystal_I2C Pyduino::lcd(LCD_I2C_PORT, 16, 2);

void Pyduino::HandleCommand(int len)
{
  int type = recieveBuffer[readPos++]; // Just read one byte

  switch (type)
  {
    case PY_DigitalWrite:
    {
      long state = NextArg();
      long port = NextArg();
      InitPort(port, OUTPUT);
      digitalWrite(port, state);
      SendResponse(DEFAULT_MESSAGE_RETURN);
      break;
    }
    case PY_DigitalRead:
    {
      long port = NextArg();
      InitPort(port, INPUT);
      //SendResponse(1);
      //SendResponse(digitalRead(port) == HIGH ? 1 : 0);
      SendResponse(digitalRead(port));
      break;
    }
    case PY_AnalogWrite:
    {
      long value = NextArg();
      long port = NextArg();
      InitPort(port, OUTPUT);
      analogWrite(port, value);
      SendResponse(DEFAULT_MESSAGE_RETURN);
      break;
    }
    case PY_AnalogRead:
    {
      long port = NextArg();
      InitPort(port, INPUT);
      SendResponse(analogRead(port));
      break;
    }
    case PY_Tone:
    {
      long port = NextArg();
      long freq = NextArg();
      float dur = NextArg_Float();
      InitPort(port, OUTPUT);
      if (dur == 0)
        tone(port, freq);
      else
        tone(port, freq, dur);
      SendResponse(DEFAULT_MESSAGE_RETURN);
      break;
    }
    case PY_NoTone:
    {
      long port = NextArg();
      InitPort(port, OUTPUT);
      noTone(port);
      SendResponse(DEFAULT_MESSAGE_RETURN);
      break;
    }
    default:
    {
      if (Pyduino::CustomMessageCallback != NULL)
        SendResponse(Pyduino::CustomMessageCallback(&recieveBuffer[readPos], len, type));
      else
        SendResponse(DEFAULT_MESSAGE_RETURN);
      break;
    }
  }
}

long Pyduino::NextArg()
{
  return GetInt(recieveBuffer, readPos);
  /*
  union Convertor conv;
  conv.bytes[0] = recieveBuffer[readPos++];
  conv.bytes[1] = recieveBuffer[readPos++];
  conv.bytes[2] = recieveBuffer[readPos++];
  conv.bytes[3] = recieveBuffer[readPos++];
  return conv.intVal;
  */
}

float Pyduino::NextArg_Float()
{
  int intVal = NextArg();
  union Convertor_Float conv;
  conv.intVal = intVal;
  return conv.floatVal;
}

String Pyduino::NextArg_String()
{
  return GetString(recieveBuffer, readPos);
}

void Pyduino::SendResponse(long response)
{
  union Convertor conv;
  conv.intVal = response;
  byte responseAr[5] = {MESSAGE_RETURN, conv.bytes[0], conv.bytes[1], conv.bytes[2], conv.bytes[3]};
  Serial.flush();
  if (Serial.write(responseAr, 5) != 5)
    Serial.write(responseAr, 5);
}

void Pyduino::InitPort(long port, int state)
{
  // If a port is not in the desired state, set it to that state
  if (portStates[port] != state)
  {
    portStates[port] = state;
    if (state <= INPUT_PULLUP)
    {
      pinMode(port, state);
    }
  }
}
    
void Pyduino::Init()
{
  // Start the serial port
  Serial.begin(115200);
  Serial.setTimeout(1);
}

void Pyduino::Tick()
{
  // If incoming data, store it
  if (Serial.available())
  {
    // Zero buf
    for (int i = 0; i < REC_BUF_SIZE; i++)
    {
      recieveBuffer[i] = 0;
    }
    
    int bytes = Serial.readBytes(recieveBuffer, Serial.available());

    // Check for command header, and handle the command
    for (int i = 0; i < REC_BUF_SIZE; i++)
    {
      if (recieveBuffer[i] == MESSAGE_START)
      {
        readPos = i + 1;
        HandleCommand(bytes - i);
        return;
      }
    }
  }
}

long Pyduino::GetInt(byte* buf, int& readPos)
{
  // Shove bytes into union and return the value
  union Convertor conv;
  conv.bytes[0] = buf[readPos++];
  conv.bytes[1] = buf[readPos++];
  conv.bytes[2] = buf[readPos++];
  conv.bytes[3] = buf[readPos++];
  return conv.intVal;
}

String Pyduino::GetString(byte* buf, int& strRead)
{
  // Store 
  byte count = buf[strRead++];

  // Buffer overflow here! -----------------------
  char str[count + 1];
  //char str[count];

  for (int i = 0; i < count; i++)
  {
    str[i] = buf[strRead++];
  }

  str[count] = 0x00;
  //str[count + 1] = 0x00;
  
  return String(str);
}
