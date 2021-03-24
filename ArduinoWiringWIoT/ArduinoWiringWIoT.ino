/*
	Windows Phone 8.1/10 Application for Windows 10 IoT Core - Hydroflyer
	https://www.hackster.io/AnuragVasanwala/windows-10-iot-core-hydroflyer-f83190

	THIS SKETCH IS DEVELOPED FOR MANEUVER MODE 2, ONLY.
	TO KNOW MORE ABOUT MODE 2, PLEASE REFER PROJECT ARTICLE.

	Objectives:
		+ Periodical Task
			+ Retrieve Arguments from gateway that came from WP application
				+ After parsing Arguments, control Engine & Rudder via gateway
			+ Retrieve GPS Data from gateway
			+ Retrieve compass heading
			+ Build string to return to WP application that must be in following format:
				"<GPS Valid Byte>|<Latitude>|<Longitude>|<Compass Heading>|<Float Switch (RAW)>|<Leak Detector (RAW)"
*/
#include "CommonCode.h"

void setup()
{
	/* Initialize Gateway */
	Gateway::Initialize();

	/*
	Initialize Engine and Rudder pin as OUTPUT pin.
	Initialize LeakDetector pin as INPUT_PULLUP.
	THIS COMMANDS WILL BE EXECUTED ON 'MyGateway1'.
	[Remote execution of gateway functions]
	*/
	Gateway::pinMode(MyGateway1, Engine_Pin, OUTPUT);
	Gateway::pinMode(MyGateway1, Rudder_Pin1, OUTPUT);
	Gateway::pinMode(MyGateway1, Rudder_Pin2, OUTPUT);
	Gateway::pinMode(MyGateway1, LeakDetection_Pin, INPUT_PULLUP);

	/*
	FloatSwitch_Pin will be act as analog float switch.
	Most of boat comes with digital float switch.

	A FLOAT SWITCH IS USED AS SAFETY SWITCH TO AVOID
	ACCIDENTAL OPERATION OF ENGINE AND RUDDER(IF ANY)
	WHEN BOAT IS NOT ON WATER.
	*/
	Gateway::pinMode(MyGateway1, FloatSwitch_Pin, INPUT_PULLUP);

	/* Initialize RPi2's GPIO_4 as OUTPUT */
	pinMode(ResetGateway_Pin, OUTPUT);
	digitalWrite(GPIO_4, HIGH);

	/* Initialize HMC5883L Compass */
	DigitalCompass.initialize();
}


void ExtractCompassData()
{
	/* Read RAW data from Compass */
	DigitalCompass.getHeading(&mx, &my, &mz);

	/* Convert RAW to Radian */
	Heading = (float)atan2(my, mx);
	if (Heading < 0)
		Heading += 2 * M_PI;

	/* Convert Radian to Degree */
	HeadingInDegrees = (int)(Heading * 180 / M_PI);
}

void Maneuver()
{
	/*
	Process ManeuverBytes and Signal Gateway to generate appropriate PWM signals for the motors
	[0] = Indicates Accelerometer rotation in X
	[1] = Indicates Accelerometer rotation in Y
	[2] = Indicates Accelerometer rotation in Z
	[3] = Reserved [You can use it for your own purpose]
	[4] = Reserved [You can use it for your own purpose]
	[5] = Reserved [You can use it for your own purpose]

	Here, Windows Phone 8.1/10 App will filter out Accelerometer reading into 90 - 110.

	I have considered following readings to maneuver boat.

	Axis	| Reading			| Description
	========|===================|=============
	X		| <= 98				| Stop Engine
	X		| > 98				| Start Engine
	Y		| < 98				| Left Turn (Rudder1)
	Y		| 98 < 100 > 102	| Center
	Y		| > 102				| Right Turn (Rudder2)

	How arguments are collected from WP8.1/10?
	WP8.1/10 --> ESP8266-01 --> Gateway --> RPi2

	*/

	/* Control Engine Pin */
	if (Arguments[0] <= 98)
	{
		Gateway::digitalWrite(MyGateway1, Engine_Pin, LOW);
	}
	else if (Arguments[0] > 98)
	{
		Gateway::digitalWrite(MyGateway1, Engine_Pin, HIGH);
	}

	/* Control Rudder Pins */
	if (Arguments[1] < 98)
	{
		/* Left turn */
		Gateway::digitalWrite(MyGateway1, Rudder_Pin1, LOW);
		Gateway::digitalWrite(MyGateway1, Rudder_Pin2, HIGH);
	}
	else if (Arguments[1] > 102)
	{
		/* Right turn */
		Gateway::digitalWrite(MyGateway1, Rudder_Pin1, HIGH);
		Gateway::digitalWrite(MyGateway1, Rudder_Pin2, LOW);
	}
	else
	{
		/* Center */
		Gateway::digitalWrite(MyGateway1, Rudder_Pin1, LOW);
		Gateway::digitalWrite(MyGateway1, Rudder_Pin2, LOW);
	}
}

void CaS_SensorData()
{
	/* Initialize temp vars and buffers */
	String SendData = "";
	char Buffer_12[12];
	char Buffer_5[5];
	char Buffer_4[4];

	/* Extract Compass Data */
	ExtractCompassData();

	/*
	Get GPS data from gateway to process it or
	to send it back in string to WP8.1/10
	*/
	Gateway::GPSData GPSArgumentsata = Gateway::getGPS(MyGateway1);

	/*
		Build string to return to WP application that must be in following format :
		"<GPS Valid Byte>|<Latitude>|<Longitude>|<Compass Heading>|<Float Switch (RAW)>|<Leak Detector (RAW)"
	*/

	/*
	Convert Valid byte into equivalent string
	+ If Valid Byte is 12 it means uBlox Neo6mV2
	is trying to acquire satellites.
	+ If Valid Byte is 31 it means uBlox Neo6mV2
	has successfully locked satellites and have
	valid GPS data.
	*/
	sprintf(Buffer_4, "%d", GPSArgumentsata.Valid);
	Buffer_4[3] = '\0';
	SendData += Buffer_4;
	SendData += "|";

	/*
	Convert Latitude into String of format (12.1234567)

	Why 3 decimal and 7 precision points are taken?
	Please check world map and max-min latitude
	and longitude. 7 precision points are taken
	to increase accuracy to show boat on map.
	*/
	sprintf(Buffer_12, "%3.7f", GPSArgumentsata.Latitude);
	Buffer_12[11] = '\0';	// Indicates last char is null/terminator char
	SendData += Buffer_12;	// Con-cat formatted latitude with string
	SendData += "|";		// Con-cat splitter symbol so that WP can extract each data using '|' symbol

							/* Convert Longitude into String of format (12.1234567) */
	sprintf(Buffer_12, "%3.7f", GPSArgumentsata.Longitude);
	Buffer_12[11] = '\0';
	SendData += Buffer_12;
	SendData += "|";

	/* Convert HeadingInDegree into equivalent string */
	sprintf(Buffer_4, "%d", HeadingInDegrees);
	Buffer_4[3] = '\0';
	SendData += Buffer_4;
	SendData += "|";

	/* Convert raw data of FloatSwitch_Pin */
	int FloatSwitch_RawData = Gateway::analogRead(MyGateway1, FloatSwitch_Pin);
	sprintf(Buffer_5, "%d", FloatSwitch_RawData);
	Buffer_5[4] = '\0';
	SendData += Buffer_5;
	SendData += "|";

	/* Convert raw data of LeakDetector */
	int LeakDetector_RawData = Gateway::analogRead(MyGateway1, LeakDetection_Pin);
	sprintf(Buffer_5, "%d", LeakDetector_RawData);
	Buffer_5[4] = '\0';
	SendData += Buffer_5;

	/* Finally con-cat null/terminator character at the end of the string */
	SendData += '\0';

	/*
	Send collected data back to Windows Phone 8.1/10 App

	How this data will be sent back?
	RPi2 -> Gateway -> ESP8266-01 -> WP8.1/10
	*/
	Gateway::sendString_ToRemoteDevice(MyGateway1, SendData);
}

void VerifyTimeout()
{
	/*
	Arguments collected from Gateway must be in the range of 90 - 110.

	If there is no device connected with ESP8266-01, Gateway will reset
	Arguments and thus we will receive Arguments less than 90.

	In such situation, we don't want to wander our boat like a ghost boat :D. So reset Engine and Rudder.
	In some situation, Arduino Nano seems to be stuck, so I've rested Arduino Nano. It will reset Engine and Rudder, too.
	*/
	if (Arguments[0] < 2)
	{
		/* If there is no connection with Arduino Nano for about 700ms, reset it. */
		if (millis() - LastCommunicationTime > 700)
		{
			/* Reset Arduino Nano. To reset Nano, write LOW to its reset pin and then HIGH */
			digitalWrite(ResetGateway_Pin, LOW);
			delay(1);
			digitalWrite(ResetGateway_Pin, HIGH);

			/* Now, wait for minimum 3-seconds to give Nano sufficient time to boot-up */
			delay(3000);

			/*
			Nano has just booted up.
			So we need to setup pinMode again.
			*/
			Gateway::pinMode(MyGateway1, Engine_Pin, OUTPUT);
			Gateway::pinMode(MyGateway1, Rudder_Pin1, OUTPUT);
			Gateway::pinMode(MyGateway1, Rudder_Pin2, OUTPUT);
			Gateway::pinMode(MyGateway1, LeakDetection_Pin, INPUT_PULLUP);

			/* Record current time as LastCommunicationTime to avoid frequent resets */
			LastCommunicationTime = millis();
		}
	}
	else
	{
		/* Record current time as LastCommunicationTime to avoid unwanted resets */
		LastCommunicationTime = millis();
	}
}

void loop()
{
	/* Enjoy the try-catch feature of Windows IoT in Arduino Sketch */
	try
	{
		/* STEP 1 : Read ManeuverBytes from Gateway */
		Gateway::getManeuverBytes(MyGateway1, Arguments);

		/* STEP 2 : Maneuver */
		/*
		Process ManeuverBytes and Signal Gateway to generate appropriate PWM signals for the motors
		[0] = Indicates Accelerometer rotation in X
		[1] = Indicates Accelerometer rotation in Y
		[2] = Indicates Accelerometer rotation in Z
		[3] = Reserved [You can use it for your own purpose]
		[4] = Reserved [You can use it for your own purpose]
		[5] = WP8.1/10 HeartBeat; Must be new every 700ms or reset Nano

		Here, Windows Phone 8.1/10 App will filter out Accelerometer reading into 90 - 110.

		I have considered following readings to maneuver boat.

		Axis	| Reading			| Description
		========|===================|=============
		X		| <= 98				| Stop Engine
		X		| > 98				| Start Engine
		Y		| < 98				| Left Turn (Rudder1)
		Y		| 98 < 100 > 102	| Center
		Y		| > 102				| Right Turn (Rudder2)

		How arguments are collected from WP8.1/10?
		WP8.1/10 --> ESP8266-01 --> Gateway --> RPi2

		*/
		Maneuver();

		/* STEP 3 : CaS = Collect and Send Sensor Data back to WP8.1/10 via Gateway */
		CaS_SensorData();

		/* STEP 4 : Check for timeout or Arduino Nano's status */
		/*
		Arguments collected from Gateway must be in the range of 90 - 110.

		If there is no device connected with ESP8266-01, Gateway will reset
		Arguments and thus we will receive Arguments less than 90.

		In such situation, we don't want to wander our boat like a ghost boat :D. So reset Engine and Rudder.
		In some situation, Arduino Nano seems to be stuck, so I've rested Arduino Nano. It will reset Engine and Rudder, too.
		*/
		VerifyTimeout();

		/* Increment HeartBeatCounter to let know WP8.1/10 that entire system is alive and not stuck */
		HeartBeatCounter++;

		/* Reset ErrorCounter */
		ErrorCounter = 0;
	}
	catch (Platform::Exception^ e)
	{
		/* Ignore any error and resume next loop */
		ErrorCounter++;
		if (ErrorCounter > MAX_ERROR_TO_RESET)
		{
			/* Reset Arduino Nano. To reset Nano, write LOW to its reset pin and then HIGH */
			digitalWrite(ResetGateway_Pin, LOW);
			delay(1);
			digitalWrite(ResetGateway_Pin, HIGH);

			/* Now, wait for minimum 3-seconds to give Nano sufficient time to boot-up */
			delay(3000);

			/*
			Nano has just booted up.
			So we need to setup pinMode again.
			*/
			Gateway::pinMode(MyGateway1, Engine_Pin, OUTPUT);
			Gateway::pinMode(MyGateway1, Rudder_Pin1, OUTPUT);
			Gateway::pinMode(MyGateway1, Rudder_Pin2, OUTPUT);
			Gateway::pinMode(MyGateway1, LeakDetection_Pin, INPUT_PULLUP);

			/* Record current time as LastCommunicationTime to avoid frequent resets */
			LastCommunicationTime = millis();
		}
	}

	/* Give pretty gateway to take some breath */
	delay(20);
}