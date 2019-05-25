// EncoderSwitch.h

#ifndef _ENCODERSWITCH_h
#define _ENCODERSWITCH_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class EncoderSwitchClass
{
 protected:


 public:
	void Init(uint8_t dtpin);
	void IRT_ENCODER_PIN();
	void SetResolution(uint8_t resolution);
	bool Enable;
	int	GetValue();
	int	GetPulse();
 private:
	 uint8_t Dt_pin;
	 int16_t Pulse;
	 uint8_t Resolution;
};

extern EncoderSwitchClass EncoderSwitch;

#endif

