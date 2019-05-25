// 
// 
// 

#include "EncoderSwitch.h"

void EncoderSwitchClass::Init(uint8_t dtpin)
{
	Dt_pin = dtpin;
	Resolution = 1;
	pinMode(Dt_pin, INPUT_PULLUP);
	Enable = false;
}

void EncoderSwitchClass::IRT_ENCODER_PIN()
{
	if (Enable == false)
	{
		Pulse = 0;
		return;
	}
	if (digitalRead(Dt_pin) == HIGH)
	{
		Pulse--;
	}
	else
	{
		Pulse++;
	}
}

void EncoderSwitchClass::SetResolution(uint8_t resolution)
{
	Resolution = resolution;
}

int EncoderSwitchClass::GetValue()
{
	int buffer = Pulse / Resolution;
	if (buffer != 0)
	{
		Pulse = 0;
	}	
	return buffer;
}

int EncoderSwitchClass::GetPulse()
{
	int buffer = Pulse / Resolution;
	return buffer;
}

EncoderSwitchClass EncoderSwitch;

