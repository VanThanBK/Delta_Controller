/*
 Name:		Delta_Controller.ino
 Created:	5/22/2019 9:04:48 AM
 Author:	Than
*/

#include <Wire.h> 
#include "LiquidCrystal_I2C.h"
#include "LCDMenu.h"
#include "StableButton.h"
#include "EncoderSwitch.h"
#include "TaskScheduler.h"
#include "SerialCommand.h"
#include <ArduinoSTL.h>
#include <vector>
#include <EEPROM.h>
#include <string.h>

#define	H_PIN	31
#define	E_PIN	33
#define	X_PIN	35
#define	Y_PIN	37
#define	Z_PIN	39

#define	UP_PIN	41
#define	LE_PIN	43
#define	RI_PIN	45
#define	DO_PIN	47

#define	EN_PIN	49
#define	BA_PIN	51

#define	SW_PIN	5
#define	DT_PIN	4
#define	CLK_PIN	3

using namespace std;

SerialCommand SerialCMD_USB;
SerialCommand SerialCMD_COM[3];

uint8_t ButtonArray[] = { H_PIN, E_PIN, X_PIN, Y_PIN, Z_PIN, UP_PIN, LE_PIN, RI_PIN, DO_PIN, EN_PIN, BA_PIN, SW_PIN};

LiquidCrystal_I2C lcd(0x27, 20, 4);

typedef struct PointDelta
{
	float X_delta;
	float Y_delta;
	float Z_delta;
	bool  E_delta;
};

OriginMenu* ScanDevice;
Label* lbCom[3];
Label* lbDevice[3];
FunctionText* ftScan;
FunctionText* ftLoadDevice;
Label* lbNoticeScan;

OriginMenu* ManuallyMenu;
Label* lbX;
Label* lbY;
Label* lbZ;
SubMenu* smChooseE;
FunctionText* ftPen;
FunctionText* ftVaccum;
FunctionText* ftGrip;
VariableText* vtX;
VariableText* vtY;
VariableText* vtZ;
SubMenu* smResolution;
FunctionText* ft50mm;
FunctionText* ft10mm;
FunctionText* ft1mm;
Label* lbNoticeManually;

OriginMenu* AutoMenu;

FunctionText* ftSavePoint;
FunctionText* ftClear;
Label* lbNumberPoint;
FunctionText* ftPlay;

OriginMenu* ControllerMenu;
Label* lbControl;
Label* lbM03;
VariableText* vtM03;
Label* lbM204;
VariableText* vtM204;
Label* lbSpeedConveyor;
VariableText* vtSpeedConveyor;
FunctionText* ftForwardData;
FunctionText* ftSave;

void(*YesDeltaCom[3])();
void(*YesConveyor[3])();

float SpeedConveyor;

float SpeedDelta;
float AccelerationDelta;

float X_Delta;
float Y_Delta;
float Z_Delta;
bool  E_Delta;

typedef enum DEVICE
{
	NULL_DEVICE = 0,
	DELTA,
	CONVEYOR,
};

typedef enum END_EFFECT
{
	NULL_END_EFFECT = 0,
	PEN,
	VACCUM,
	GRIP,
};

typedef enum MODE
{
	FREE = 0,
	HOME,
	SET_VALUE,
	MOVE,
	LOAD,
	SCAN,
	AUTO,
};

typedef enum FUNCTION_BUTTON
{
	H = 0,
	E,
	X,
	Y,
	Z,
};

FUNCTION_BUTTON FunctionButton = H;

DEVICE Device[3];
END_EFFECT EndEffect = NULL_END_EFFECT;

MODE Mode = FREE;

vector<PointDelta> PointArray;
uint8_t IndexArray = 0;

void setup()
{
	InitFunction();
	Serial.begin(9600);
	Serial1.begin(9600);
	Serial2.begin(9600);
	Serial3.begin(9600);

	SerialCMD_USB = SerialCommand(&Serial, 9600);
	SerialCMD_COM[0] = SerialCommand(&Serial1, 9600);
	SerialCMD_COM[1] = SerialCommand(&Serial2, 9600);
	SerialCMD_COM[2] = SerialCommand(&Serial3, 9600);

	SerialCMD_USB.ForwardData(&Serial1, &Serial2, &Serial3, 9600);
	SerialCMD_USB.IsForwardData = true;

	SerialCMD_COM[0].ForwardData(&Serial, 9600);
	SerialCMD_COM[1].ForwardData(&Serial, 9600);
	SerialCMD_COM[2].ForwardData(&Serial, 9600);

	SerialCMD_USB.AddCommand("Confirm", ConfirmUsb);

	for (uint8_t i = 0; i < 3; i++)
	{
		SerialCMD_COM[i].AddCommand("YesDelta", YesDeltaCom[i]);
		SerialCMD_COM[i].AddCommand("YesConveyor", YesConveyor[i]);
		SerialCMD_COM[i].AddCommand("Ok", Ok);
		SerialCMD_COM[i].AddCommand("M03", &SpeedDelta);
		SerialCMD_COM[i].AddCommand("M204", &AccelerationDelta);
		SerialCMD_COM[i].AddCommand("Speed", &SpeedConveyor);
		SerialCMD_COM[i].AddCommand(&X_Delta, &Y_Delta, &Z_Delta);
	}

	InitUI();

	StableButton.Init(ButtonArray, 12);
	EncoderSwitch.Init(DT_PIN);

	TaskScheduler.Init();
	TaskScheduler.Add(CheckTimeOutScan, 450);
	TaskScheduler.Add(CheckTimeOutLoad, 500);
	TaskScheduler.Add(CheckTimeOutHomeLoad, 400);
	TaskScheduler.Add(EncoderSendGcode, 1000);
	TaskScheduler.Add(EncoderLoad, 50);
	TaskScheduler.Add(RunPointArray, 100);

	TaskScheduler.Stop(CheckTimeOutScan);
	TaskScheduler.Stop(CheckTimeOutLoad);
	TaskScheduler.Stop(CheckTimeOutHomeLoad);
	TaskScheduler.Stop(EncoderSendGcode);
	TaskScheduler.Stop(EncoderLoad);
	TaskScheduler.Stop(RunPointArray);
	TaskScheduler.Run();

	attachInterrupt(digitalPinToInterrupt(CLK_PIN), Isr_encoder, RISING);
}

void loop()
{
	ExecuteControlButton();
	LCDMenu.ExecuteEffect();
	LCDMenu.UpdateScreen();

	SerialCMD_USB.Execute();
	SerialCMD_COM[0].Execute();
	SerialCMD_COM[1].Execute();
	SerialCMD_COM[2].Execute();	
}

void Isr_encoder()
{
	EncoderSwitch.IRT_ENCODER_PIN();
}

void InitFunction()
{
	YesDeltaCom[0] = YesDeltaCom1;
	YesDeltaCom[1] = YesDeltaCom2;
	YesDeltaCom[2] = YesDeltaCom3;

	YesConveyor[0] = YesConveyor1;
	YesConveyor[1] = YesConveyor2;
	YesConveyor[2] = YesConveyor3;
}

void InitUI()
{
	ScanDevice = new OriginMenu();
	{
		lbCom[2] = new Label(ScanDevice, "COM3", 14, 0);
		lbCom[1] = new Label(ScanDevice, "COM2", 7, 0);
		lbCom[0] = new Label(ScanDevice, "COM1", 0, 0);
		lbDevice[2] = new Label(ScanDevice, "----", 14, 1);
		lbDevice[1] = new Label(ScanDevice, "----", 7, 1);
		lbDevice[0] = new Label(ScanDevice, "----", 0, 1);

		ftScan = new FunctionText(ScanDevice, "Scan", 0, 2);
		ftScan->Function = ScanningDevice;

		ftLoadDevice = new FunctionText(ScanDevice, "Load", 16, 2);
		ftLoadDevice->Function = LoadDevice;

		lbNoticeScan = new Label(ScanDevice, "No Device", 0, 3);
	}

	ManuallyMenu = new OriginMenu();
	{
		lbX = new Label(ManuallyMenu, "X:", 0, 0);
		vtX = new VariableText(ManuallyMenu, 0, 2, 0);
		vtX->SetExternalValue(&X_Delta);
		vtX->HandleWhenValueChange = SendMoveXGCode;

		lbY = new Label(ManuallyMenu, "Y:", 10, 0);
		vtY = new VariableText(ManuallyMenu, 0, 12, 0);
		vtY->SetExternalValue(&Y_Delta);
		vtY->HandleWhenValueChange = SendMoveYGCode;

		lbZ = new Label(ManuallyMenu, "Z:", 0, 1);
		vtZ = new VariableText(ManuallyMenu, 0, 2, 1);
		vtZ->SetExternalValue(&Z_Delta);
		vtZ->HandleWhenValueChange = SendMoveZGCode;

		smChooseE = new SubMenu(ManuallyMenu, "E:----", 10, 1);
		{
			Label* lbChooseE = new Label(smChooseE->Container, "End_Effector:", 4, 0);
			ftPen = new FunctionText(smChooseE->Container, "Pen", 0, 1);
			ftPen->Function = FunctionPen;
			ftVaccum = new FunctionText(smChooseE->Container, "Vaccum", 0, 2);
			ftVaccum->Function = FunctionVaccum;
			ftGrip = new FunctionText(smChooseE->Container, "Grip", 0, 3);
			ftGrip->Function = FunctionGrip;
		}

		smResolution = new SubMenu(ManuallyMenu, "Resolution", 0, 2);
		{
			Label* lbResolution = new Label(smResolution->Container, "Resolution:", 4, 0);
			ft50mm = new FunctionText(smResolution->Container, "Move 50 mm", 0, 1);
			ft50mm->Function = Function50mm;
			ft10mm = new FunctionText(smResolution->Container, "Move 10 mm", 0, 2);
			ft10mm->Function = Function10mm;
			ft1mm = new FunctionText(smResolution->Container, "Move  1 mm", 0, 3);
			ft1mm->Function = Function1mm;
		}

		lbNoticeManually = new Label(ManuallyMenu, "", 0, 3);
	}

	AutoMenu = new OriginMenu();
	{
		ftSavePoint = new FunctionText(AutoMenu, "SavePoint", 0, 1);
		ftSavePoint->Function = SavePointAutoMenu;

		lbNumberPoint = new Label(AutoMenu, "Point:0", 11, 1);

		ftPlay = new FunctionText(AutoMenu, "Run", 0, 2);
		ftPlay->Function = PlayAutoMenu;

		ftClear = new FunctionText(AutoMenu, "Clear", 14, 2);
		ftClear->Function = ClearAutoMenu;
	}

	ControllerMenu = new OriginMenu();
	{
		lbControl = new Label(ControllerMenu, "Control", 6, 0);

		lbM03 = new Label(ControllerMenu, "M03:", 0, 1);
		vtM03 = new VariableText(ControllerMenu, 0, 4, 1);
		vtM03->SetExternalValue(&SpeedDelta);
		vtM03->HandleWhenValueChange = SendSetM03GCode;

		lbM204 = new Label(ControllerMenu, "M204:", 10, 1);
		vtM204 = new VariableText(ControllerMenu, 0, 15, 1);
		vtM204->SetExternalValue(&AccelerationDelta);
		vtM204->HandleWhenValueChange = SendSetM204GCode;

		lbSpeedConveyor = new Label(ControllerMenu, "S_CO:", 0, 2);
		vtSpeedConveyor = new VariableText(ControllerMenu, 0, 5, 2);
		vtSpeedConveyor->SetExternalValue(&SpeedConveyor);
		vtSpeedConveyor->HandleWhenValueChange = SendSetSpeedGCode;

		ftForwardData = new FunctionText(ControllerMenu, "Control", 0, 3);
		ftForwardData->Function = FunctionForwardData;

		ftSave = new FunctionText(ControllerMenu, "Save", 16, 3);
		ftSave->Function = SendSaveScode;
	}

	LCDMenu.Init(&lcd, "  Delta Controller");
	LCDMenu.AddMenu(ScanDevice);
	LCDMenu.AddMenu(ManuallyMenu);
	LCDMenu.AddMenu(AutoMenu);
	LCDMenu.AddMenu(ControllerMenu);

	LCDMenu.SetCurrentMenu(ScanDevice);
	LCDMenu.UpdateScreen();
}

void ExecuteControlButton()
{
	if (StableButton.IsPressed(H_PIN))
	{
		SendHomeGCode();
		FunctionButton = H;
	}

	if (StableButton.IsPressed(E_PIN))
	{
		SendChangeEGCode();
		FunctionButton = E;
	}

	if (StableButton.IsPressed(X_PIN))
	{
		if (FunctionButton == X)
		{
			FunctionButton = H;
			lbNoticeManually->SetText("");
			EncoderSwitch.Enable = false;
			TaskScheduler.Stop(EncoderSendGcode);
			TaskScheduler.Stop(EncoderLoad);
		}
		else
		{
			FunctionButton = X;
			EncoderSwitch.Enable = true;
			TaskScheduler.Resum(EncoderSendGcode);
			TaskScheduler.Resum(EncoderLoad);
		}
	}

	if (StableButton.IsPressed(Y_PIN))
	{
		if (FunctionButton == Y)
		{
			FunctionButton = H;
			lbNoticeManually->SetText("");
			EncoderSwitch.Enable = false;
			TaskScheduler.Stop(EncoderSendGcode);
			TaskScheduler.Stop(EncoderLoad);
		}
		else
		{
			FunctionButton = Y;
			EncoderSwitch.Enable = true;
			TaskScheduler.Resum(EncoderSendGcode);
			TaskScheduler.Resum(EncoderLoad);
		}
	}

	if (StableButton.IsPressed(Z_PIN))
	{
		if (FunctionButton == Z)
		{
			FunctionButton = H;
			lbNoticeManually->SetText("");
			EncoderSwitch.Enable = false;
			TaskScheduler.Stop(EncoderSendGcode);
			TaskScheduler.Stop(EncoderLoad);
		}
		else
		{
			FunctionButton = Z;
			EncoderSwitch.Enable = true;
			TaskScheduler.Resum(EncoderSendGcode);
			TaskScheduler.Resum(EncoderLoad);
		}
	}

	if (StableButton.IsPressed(UP_PIN))
	{
		LCDMenu.MoveCursorUp();
	}

	if (StableButton.IsPressed(LE_PIN))
	{
		LCDMenu.MoveCursorLeft();
	}

	if (StableButton.IsPressed(RI_PIN))
	{
		LCDMenu.MoveCursorRight();
	}

	if (StableButton.IsPressed(DO_PIN))
	{
		LCDMenu.MoveCursorDown();
	}

	if (StableButton.IsPressed(EN_PIN))
	{
		LCDMenu.Enter();
	}

	if (StableButton.IsPressed(BA_PIN))
	{
		LCDMenu.Return();
	}

	if (StableButton.IsPressed(SW_PIN))
	{
		
	}
}

void SendHomeGCode()
{
	if (Mode == HOME)
	{
		return;
	}
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			Mode = HOME;
			lbNoticeScan->SetText("-Homing...");
			lbNoticeManually->SetText("-Homing...");
			SerialCMD_COM[i]._Serial->println("G28");
		}		
	}
}

void SendChangeEGCode()
{
	if (Mode == MOVE)
	{
		return;
	}
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			if (EndEffect == VACCUM)
			{
				Mode = MOVE;
				lbNoticeScan->SetText("-Moving...");
				lbNoticeManually->SetText("-Moving...");
				SendVaccumGcode(E_Delta);
			}
		}
	}
}

void SendVaccumGcode(bool value)
{

}

void SendMoveXGCode()
{
	if (Mode == MOVE)
	{
		return;
	}
	String gcode = String("G01 X") + String(X_Delta);
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			Mode = MOVE;
			lbNoticeManually->SetText("-Moving...");
			SerialCMD_COM[i]._Serial->println(gcode);
			break;
		}
	}
}

void SendMoveYGCode()
{
	if (Mode == MOVE)
	{
		return;
	}
	String gcode = String("G01 Y") + String(Y_Delta);
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			Mode = MOVE;
			lbNoticeManually->SetText("-Moving...");
			SerialCMD_COM[i]._Serial->println(gcode);
			break;
		}
	}
}

void SendMoveZGCode()
{
	if (Mode == MOVE)
	{
		return;
	}
	String gcode = String("G01 Z") + String(Z_Delta);
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			Mode = MOVE;
			lbNoticeManually->SetText("-Moving...");
			SerialCMD_COM[i]._Serial->println(gcode);
			break;
		}
	}
}

void GetDataFromEeprom()
{
	//EEPROM.get(SEASON_ADDRESS, ProcessingSeason);
}

void SaveDataStructToEeprom()
{
	//EEPROM.put(SEASON_ADDRESS, ProcessingSeason);
}

void ConfirmUsb()
{
	Serial.println("YesController");
}

void YesDeltaCom1()
{
	Device[0] = DELTA;
}

void YesDeltaCom2()
{
	Device[1] = DELTA;
}

void YesDeltaCom3()
{
	Device[2] = DELTA;
}

void YesConveyor1()
{
	Device[0] = CONVEYOR;
}

void YesConveyor2()
{
	Device[1] = CONVEYOR;
}

void YesConveyor3()
{
	Device[2] = CONVEYOR;
}

void Ok()
{
	switch (Mode)
	{
	case FREE:
		break;
	case HOME:
		for (uint8_t i = 0; i < 3; i++)
		{
			if (Device[i] == DELTA)
			{
				SerialCMD_COM[i]._Serial->println("Position");
				break;
			}
		}
		lbNoticeScan->SetText("-Home");
		lbNoticeManually->SetText("-Home");
		TaskScheduler.Resum(CheckTimeOutHomeLoad);
		Mode = FREE;
		break;
	case SET_VALUE:
		lbNoticeScan->SetText("-Set done");
		lbNoticeManually->SetText("-Set done");
		Mode = FREE;
		break;
	case MOVE:
		lbNoticeScan->SetText("-Finished");
		if (FunctionButton == H || FunctionButton == E)
		{
			lbNoticeManually->SetText("-Finished");
		}		
		Mode = FREE;
		break;
	case LOAD:
		break;

	case AUTO:
		Mode = FREE;
		break;
	default:
		break;
	}
}

void ScanningDevice()
{
	if (Mode == SCAN)
	{
		return;
	}
	Mode = SCAN;
	lbNoticeScan->SetText("-Scanning...");
	for (uint8_t i = 0; i < 3; i++)
	{
		SerialCMD_COM[i].Enable = true;
		Device[i] = NULL_DEVICE;
		SerialCMD_COM[i]._Serial->println("Confirm");		
	}
	TaskScheduler.Resum(CheckTimeOutScan);
}

void LoadDevice()
{
	if (Mode == LOAD)
	{
		return;
	}
	Mode = LOAD;
	lbNoticeScan->SetText("-Loadding...");
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			LoadDelta(i);
		}
		else if (Device[i] == CONVEYOR)
		{
			LoadConveyor(i);
		}
	}
	TaskScheduler.Resum(CheckTimeOutLoad);
}

void LoadDelta(uint8_t com)
{
	SerialCMD_COM[com]._Serial->println("pullM03");
	SerialCMD_COM[com]._Serial->println("pullM204");
	SerialCMD_COM[com]._Serial->println("Position");
}

void LoadConveyor(uint8_t com)
{
	SerialCMD_COM[com]._Serial->println("Getspee");
}

void CheckTimeOutScan()
{
	TaskScheduler.Stop(CheckTimeOutScan);

	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == NULL_DEVICE)
		{
			lbDevice[i]->SetText("----");
			SerialCMD_COM[i].Enable = false;
		}
		else if (Device[i] == DELTA)
		{
			lbDevice[i]->SetText("DELTA");
		}
		else if (Device[i] == CONVEYOR)
		{
			lbDevice[i]->SetText("CONVE");
		}
	}
	Mode = FREE;
	lbNoticeScan->SetText("-Scan done");
}

void CheckTimeOutLoad()
{
	TaskScheduler.Stop(CheckTimeOutLoad);
	UpdateLabelVelocity();
	UpdateLabelPosition();
	Mode = FREE;
	lbNoticeScan->SetText("-Load done");
}

void CheckTimeOutHomeLoad()
{
	TaskScheduler.Stop(CheckTimeOutHomeLoad);
	UpdateLabelPosition();
}

void EncoderSendGcode()
{
	int encoderBuffer = EncoderSwitch.GetValue();
	if (encoderBuffer == 0)
	{
		return;
	}

	switch (FunctionButton)
	{
	case X:
		X_Delta = X_Delta + encoderBuffer;
		SendMoveX();
		break;
	case Y:
		Y_Delta = Y_Delta + encoderBuffer;
		SendMoveY();
		break;
	case Z:
		Z_Delta = Z_Delta + encoderBuffer;
		SendMoveZ();
		break;
	default:
		break;
	}
	UpdateLabelPosition();
}

void SendMoveX()
{
	String gcode = String("G01 X") + String(X_Delta);
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			Mode = MOVE;
			SerialCMD_COM[i]._Serial->println(gcode);
			break;
		}
	}
}

void SendMoveY()
{
	String gcode = String("G01 Y") + String(Y_Delta);
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			Mode = MOVE;
			SerialCMD_COM[i]._Serial->println(gcode);
			break;
		}
	}
}

void SendMoveZ()
{
	String gcode = String("G01 Z") + String(Z_Delta);
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			Mode = MOVE;
			SerialCMD_COM[i]._Serial->println(gcode);
			break;
		}
	}
}

void EncoderLoad()
{
	int encoderBuffer = EncoderSwitch.GetPulse();

	switch (FunctionButton)
	{
	case X:
		lbNoticeManually->SetText("-X desire: " + String(X_Delta + encoderBuffer));
		break;
	case Y:
		lbNoticeManually->SetText("-Y desire: " + String(Y_Delta + encoderBuffer));
		break;
	case Z:
		lbNoticeManually->SetText("-Z desire: " + String(Z_Delta + encoderBuffer));
		break;
	default:
		break;
	}
}

void RunPointArray()
{
	if (Mode == AUTO)
	{
		return;
	}
	PointDelta pointBuffer;
	pointBuffer = PointArray.operator[](IndexArray);
	X_Delta = pointBuffer.X_delta;
	Y_Delta = pointBuffer.Y_delta;
	Z_Delta = pointBuffer.Z_delta;
	E_Delta = pointBuffer.E_delta;
	SendMoveXYZEGCode();
	Mode = AUTO;
	UpdateLabelPosition();
	IndexArray++;
	if (IndexArray == PointArray.size())
	{
		IndexArray = 0;
	}
}

void SendMoveXYZEGCode()
{
	String gcode = String("G01 X") + String(X_Delta) + String(" Y") + String(Y_Delta) + String(" Z") + String(Z_Delta);
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			SerialCMD_COM[i]._Serial->println(gcode);
			break;
		}
	}
}

void FunctionPen()
{
	EndEffect = PEN;
	smChooseE->SetText("E:Pen");
	LCDMenu.Return();
}

void FunctionVaccum()
{
	EndEffect = VACCUM;
	smChooseE->SetText("E:Vaccum");
	LCDMenu.Return();
}

void FunctionGrip()
{
	EndEffect = GRIP;
	smChooseE->SetText("E:Grip");
	LCDMenu.Return();
}

void Function50mm()
{
	vtX->Resolution = 50;
	vtY->Resolution = 50;
	vtZ->Resolution = 50;
	vtM03->Resolution = 50;
	vtM204->Resolution = 50;
	vtSpeedConveyor->Resolution = 50;
	LCDMenu.Return();
}

void Function10mm()
{
	vtX->Resolution = 10;
	vtY->Resolution = 10;
	vtZ->Resolution = 10;
	vtM03->Resolution = 10;
	vtM204->Resolution = 10;
	vtSpeedConveyor->Resolution = 10;
	LCDMenu.Return();
}

void Function1mm()
{
	vtX->Resolution = 1;
	vtY->Resolution = 1;
	vtZ->Resolution = 1;
	vtM03->Resolution = 1;
	vtM204->Resolution = 1;
	vtSpeedConveyor->Resolution = 1;
	LCDMenu.Return();
}

void SavePointAutoMenu()
{
	PointDelta point;
	point.E_delta = E_Delta;
	point.X_delta = X_Delta;
	point.Y_delta = Y_Delta;
	point.Z_delta = Z_Delta;
	PointArray.push_back(point);
	String text = String("Point:") + String(PointArray.size());
	lbNumberPoint->SetText(text);
}

void PlayAutoMenu()
{
	if (PointArray.size() == 0)
	{
		return;
	}
	if (ftPlay->Text == "Run")
	{
		ftPlay->SetText("Stop");
		TaskScheduler.Resum(RunPointArray);
	}
	else
	{
		ftPlay->SetText("Run");
		TaskScheduler.Stop(RunPointArray);
	}
}

void ClearAutoMenu()
{
	IndexArray = 0;
	PointArray.clear();
	String text = String("Point:") + String(PointArray.size());
	lbNumberPoint->SetText(text);
}

void FunctionForwardData()
{
		if (ftForwardData->Text == "Control")
		{
			if (Device[0] == DELTA || Device[0] == CONVEYOR) SerialCMD_COM[0].IsForwardData = true;
			if (Device[1] == DELTA || Device[1] == CONVEYOR) SerialCMD_COM[1].IsForwardData = true;
			if (Device[2] == DELTA || Device[2] == CONVEYOR) SerialCMD_COM[2].IsForwardData = true;		
			ftForwardData->SetText("Forward");
		}
		else
		{
			SerialCMD_COM[0].IsForwardData = false;
			SerialCMD_COM[1].IsForwardData = false;
			SerialCMD_COM[2].IsForwardData = false;
			ftForwardData->SetText("Control");
		}
}

void SendSetM03GCode()
{
	String gcode = String("M03 S") + String(vtM03->GetValue());
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			Mode = SET_VALUE;
			SerialCMD_COM[i]._Serial->println(gcode);
			break;
		}
	}
}

void SendSetM204GCode()
{
	String gcode = String("M204 A") + String(vtM204->GetValue());
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			Mode = SET_VALUE;
			SerialCMD_COM[i]._Serial->println(gcode);
			break;
		}
	}
}

void SendSetSpeedGCode()
{
	String gcode = String("Velocit:") + String(vtSpeedConveyor->GetValue());
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == CONVEYOR)
		{
			Mode = SET_VALUE;
			SerialCMD_COM[i]._Serial->println(gcode);
			break;
		}
	}
}

void SendSaveScode()
{
	for (uint8_t i = 0; i < 3; i++)
	{
		if (Device[i] == DELTA)
		{
			Mode = SET_VALUE;
			SerialCMD_COM[i]._Serial->println("M500");
			break;
		}
	}
}

void UpdateLabelPosition()
{
	vtX->SetValue(X_Delta);
	vtY->SetValue(Y_Delta);
	vtZ->SetValue(Z_Delta);
}

void UpdateLabelVelocity()
{
	vtM03->SetValue(SpeedDelta);
	vtM204->SetValue(AccelerationDelta);
	vtSpeedConveyor->SetValue(SpeedConveyor);
}
