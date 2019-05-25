#include "SerialCommand.h"

SerialCommand::SerialCommand()
{
	_Serial = &Serial;
	_Serial->begin(9600);
	//inputString.reserve(100);

	boolean stringComplete = false;
	String inputString = "";

	cmdCounter = 0;
	IsForwardData = false;
	Enable = true;
	//cmdContainer = new Command[10];
	cmdContainer = NULL;
}

SerialCommand::SerialCommand(HardwareSerial* serial, uint16_t baudrate)
{
	_Serial = serial;
	_Serial->begin(baudrate);
	//inputString.reserve(100);

	boolean stringComplete = false;
	String inputString = "";

	IsForwardData = false;
	Enable = true;
	cmdCounter = 0;
	//cmdContainer = new Command[cmdCounter];
	cmdContainer = NULL;
}

SerialCommand::~SerialCommand()
{
  //free(cmdContainer);
  delete[] cmdContainer;
}

void SerialCommand::ForwardData(HardwareSerial* forwardSerial, uint16_t baudrate)
{
	ForwardSerial = forwardSerial;
	ForwardSerial->begin(baudrate);
}

void SerialCommand::ForwardData(HardwareSerial* forwardSerial, HardwareSerial* forwardSerial1 , HardwareSerial* forwardSerial2, uint16_t baudrate)
{
	ForwardSerial = forwardSerial;
	ForwardSerial1 = forwardSerial1;
	ForwardSerial2 = forwardSerial2;

	ForwardSerial->begin(baudrate);
	ForwardSerial1->begin(baudrate);
	ForwardSerial2->begin(baudrate);
}

void SerialCommand::GetValueFromString()
{
	float position[3];
	uint8_t indexPosition = 0;
	String splitWord = "";
	inputString += ",";
	for (uint16_t i = 0; i < inputString.length(); i++)
	{
		if (inputString[i] == ',')
		{
			if (splitWord == "")
				continue;
			position[indexPosition] = splitWord.substring(1).toFloat();
			splitWord = "";
			indexPosition++;
			continue;
		}
		splitWord += String(inputString[i]);
	}
	if (indexPosition == 3)
	{
		*X_Delta = position[0];
		*Y_Delta = position[1];
		*Z_Delta = roundf(-position[2]);
	}
}

void SerialCommand::AddCommand(String message, void(*function)())
{
  cmdCounter++;
  //cmdContainer = (Command *) realloc(cmdContainer, cmdCounter * sizeof(Command));
  
   Command* newContainer = new Command[cmdCounter];
   for( uint8_t i = 0; i < cmdCounter - 1; i++ )
   {
     newContainer[i] = cmdContainer[i];
   } 
   if( cmdContainer != NULL ) 
   {
     delete[] cmdContainer;
   }   
   cmdContainer = newContainer;
   
  
  cmdContainer[cmdCounter - 1].message = message;
  cmdContainer[cmdCounter - 1].function = function;
  cmdContainer[cmdCounter - 1].value = NULL;
  cmdContainer[cmdCounter - 1].contain = NULL;
}

void SerialCommand::AddCommand(String message, float* value)
{  
  cmdCounter++;
  //cmdContainer = (Command *) realloc(cmdContainer, cmdCounter * sizeof(Command));

   Command* newContainer = new Command[cmdCounter];
   for( uint8_t i = 0; i < cmdCounter - 1; i++ )
   {
     newContainer[i] = cmdContainer[i];
   } 
   if( cmdContainer != NULL )
   {
     delete[] cmdContainer;
   }   
   cmdContainer = newContainer;
  
  cmdContainer[cmdCounter - 1].message = message;
  cmdContainer[cmdCounter - 1].value = value;
  cmdContainer[cmdCounter - 1].function = NULL;
  cmdContainer[cmdCounter - 1].contain = NULL;
}

void SerialCommand::AddCommand(float* xvalue, float* yvalue, float* zvalue)
{
	X_Delta = xvalue;
	Y_Delta = yvalue;
	Z_Delta = zvalue;
}

void SerialCommand::AddCommand(String message, String* contain)
{
	cmdCounter++;
	//cmdContainer = (Command *) realloc(cmdContainer, cmdCounter * sizeof(Command));

	Command* newContainer = new Command[cmdCounter];
	for (uint8_t i = 0; i < cmdCounter - 1; i++)
	{
		newContainer[i] = cmdContainer[i];
	}
	if (cmdContainer != NULL)
	{
		delete[] cmdContainer;
	}
	cmdContainer = newContainer;

	cmdContainer[cmdCounter - 1].message = message;
	cmdContainer[cmdCounter - 1].value = NULL;
	cmdContainer[cmdCounter - 1].function = NULL;
	cmdContainer[cmdCounter - 1].contain = contain;
}

void SerialCommand::ClearCommand()
{
	delete[] cmdContainer;
	cmdCounter = 0;
}

void SerialCommand::Execute()
{
	if (Enable == false)
	{
		return;
	}
  // receive every character
  while (_Serial->available()) 
  {
    char inChar = (char)_Serial->read();

	if (IsForwardData)
	{
		if (ForwardSerial != NULL)
		{
			ForwardSerial->print(inChar);
		}

		if (ForwardSerial1 != NULL)
		{
			ForwardSerial1->print(inChar);
		}

		if (ForwardSerial2 != NULL)
		{
			ForwardSerial2->print(inChar);
		}
	}
    
	if (inChar == '\n')
    {
      stringComplete = true;
      break;
    }

	if (inChar != '\r')
	{
		inputString += inChar;
	} 
  }
  
  if (!stringComplete)
    return;

  //Serial.println(inputString);
   
  // when complete receiving
  for( uint8_t index = 0; index < cmdCounter; index++ )
  {
	if (cmdContainer[index].message == inputString.substring(0, cmdContainer[index].message.length()))
    { 
      if( cmdContainer[index].value != NULL )
      {
        *cmdContainer[index].value = inputString.substring(cmdContainer[index].message.length() + 1).toFloat();        
      }
	  else if (cmdContainer[index].contain != NULL)
	  {
		  //*cmdContainer[index].contain = inputString.substring(cmdContainer[index].message.length() + 1);

		  *cmdContainer[index].contain = "";

		  for (uint16_t i = cmdContainer[index].message.length() + 1; i < inputString.length(); i++)
		  {
			  *cmdContainer[index].contain += inputString[i];
		  }

		  /*uint16_t num = inputString.length() - cmdContainer[index].message.length();
		  
		  inputString.toCharArray(cmdContainer[index].contain, num + 1, 5);*/
		  //Serial.println(*cmdContainer[index].contain);
		  //Serial.println("Done");
	  }
      else
      {
		  if (cmdContainer[index].function != NULL)
			cmdContainer[index].function();
      }
    }
  }

  if (X_Delta != NULL)
  {
	  GetValueFromString();
  }

  inputString = "";
  stringComplete = false;
}
