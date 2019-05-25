#ifndef _SerialCommand_
#define _SerialCommand_

#include <string.h>
#include <Arduino.h>

struct Command
{
  String message = "";
  void(*function)() = NULL;
  float* value = NULL;
  String* contain;
};

class SerialCommand
{
public:
  HardwareSerial* _Serial;
  HardwareSerial* ForwardSerial = NULL;
  HardwareSerial* ForwardSerial1 = NULL;
  HardwareSerial* ForwardSerial2 = NULL;

  Command* cmdContainer;
  uint8_t cmdCounter = 0;
  bool IsForwardData;
  bool Enable;

  float* X_Delta = NULL;
  float* Y_Delta = NULL;
  float* Z_Delta = NULL;
  
  SerialCommand();
  SerialCommand(HardwareSerial*, uint16_t);
  ~SerialCommand();
  
  void AddCommand(String message, void(*function)());
  void AddCommand(String message, float* value);
  void AddCommand(float* xvalue, float* yvalue, float* zvalue);
  void AddCommand(String message, String* contain);
  void ClearCommand();
  void Execute();
  void ForwardData(HardwareSerial*, uint16_t);
  void ForwardData(HardwareSerial*, HardwareSerial*, HardwareSerial*,  uint16_t);
  void GetValueFromString();
  
private:
  boolean stringComplete;
  String inputString;
};

#endif
