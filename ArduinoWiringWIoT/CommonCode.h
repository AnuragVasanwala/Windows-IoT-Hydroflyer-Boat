#pragma once
/* Global Includes */
#include "Gateway.h"
#include "I2Cdev.h"
#include "HMC5883L.h"

/*
Following '#define ARDUINO 200' denotes that this sketch is developed in Arduino Version 200 :D that is not actually exists.

Why it is required?
This project uses HMC5883L compass device.
I2Cdev and HMC5883L library compiles code based upon Arduino version.
If we don't denote it, it will recognize as version 1 and which leads to multiple problems.
I'm not sure about this but with this mod, my program works successful without any exception for long run.

We are making Windows IoT's pre-processor fool when it extracts pre-processor directives to avoid compilation of old version code.
*/
#define ARDUINO 200

/* Our pretty Gateway's name or let's say address */
#define MyGateway1 0x40

/* Gateway : Pin Name -> Pin Number */
#define Engine_Pin Gateway::UsablePinArguments11
#define Rudder_Pin1 Gateway::UsablePinArguments5
#define Rudder_Pin2 Gateway::UsablePinArguments6
#define FloatSwitch_Pin Gateway::UsablePin_A1
#define LeakDetection_Pin Gateway::UsablePin_A0

/* RPi2 : Pin Name -> Pin Number */
#define ResetGateway_Pin GPIO_4

/* Global loop Variables */
byte Arguments[6];
int ResetCounter = 0;					// Holds number of time RPi2 had reseted Gateway.
int HeartBeatCounter = 0;				// Holds HeartBeatCounter. Will update at the end of loop execution.
unsigned long LastCommunicationTime;	// Holds the last known communication time in-order to know about pretty gateway's status.

/* Error Counter and Constant */
byte ErrorCounter = 0;					// Counts errors and resets Nano if consequent error count is > 'MAX_ERROR_TO_RESET'
#define MAX_ERROR_TO_RESET	5			// Maximum number of consequent errors to reset Nano

/* Global Compass Object & Variable */
HMC5883L DigitalCompass;				// Digital Compass's object
int16_t mx, my, mz;						// Axis data in RAW
float Heading;							// Heading in Radian
int HeadingInDegrees;