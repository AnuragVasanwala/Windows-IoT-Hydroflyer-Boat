/*
	ESP8266-01 sketch for Hydroflyer (Boat) (RPi2 + WinIoT)
	https://www.hackster.io/AnuragVasanwala/windows-10-iot-core-hydroflyer-f83190

	NOTE: This sketch is tested and meant for Arduino Nano.

	Gateway sketch for Arduino is still in early development. Contribution is most welcomed.

	Objectives:
		+ Provide support for ADC, PWM, Serial to I2C Master on request
		+ Periodically send ESP8266-01 'ReplyBuffer_100' received from I2C Master
		+ Once 'ReplyBuffer_100' is sent to ESP, ESP will send back 'Arguments' parsed from 'Maneuver' GET request so read them.
		+ Provide support for GPS (Device: uBlox New6mV2 with 9600 baud) to I2C Master on demand

	: WARNING :
		+ 'Argument_Size' AND 'SensorBuffer_Size' MUST BE IDENTICAL ON ESP8266-01 SKETCH.
		+ SERIAL BAUD RATE MUST BE SAME ON BOTH SIDE (GATEWAY & ESP).
		+ 'SENDArgumentsATA_STRUCTURE' MUST BE IDENTICAL ON ESP8266 SIDE (WITH NAME OF 'RECEIVEArgumentsATA_STRUCTURE').
		+ 'RECEIVEArgumentsATA_STRUCTURE' MUST BE IDENTICAL ON ESP8266 SIDE (WITH NAME OF 'SENDArgumentsATA_STRUCTURE').
		+ YOU MUST HAVE 'EasyTransfer' LIBRARY IN YOUR ARDUINO FOLDER.
		  TO INSTALL 'EasyTransfer', REFER: https://github.com/madsci1016/Arduino-EasyTransfer
		  THIS REPOSITORY CONTAINS:
			+ EasyTransfer
			+ EasyTransferI2C
			+ EasyTransferVirtualWire
			+ SoftEasyTransfer

			BUT YOU ONLY HAVE TO COPY FIRST FOLDER 'EasyTransfer' TO YOUR ARDUINO's LIBRARY FOLDER
*/

/* Global Include */
#include <Wire.h>
#include <TinyGPS++.h>
#include <EasyTransfer.h>
#include <SoftwareSerial.h>

/* Our pretty Gateway's name or let's say address */
#define MyGateway1 0x40

/* Arguments sizes and Timeout constants */
#define Argument_Size		6          // Number of arguments to be fetch and store in global 'Arguments'
#define SensorBuffer_Size   100        // Max number of characters to be sent back to remote device

/* GPS Pin defination & Baud Rate*/
#define GPS_RX_PIN			8
#define GPS_TX_PIN			9
#define GPS_BAUD			9600       // Please confirm your GPS's baud. If it is 115200, them use AltSoftSerial instead of SoftwareSerial

/*
	[ DO NOT MODIFY ]
	Following are the list of Gateway Protocol Mode
	They must be identical on both side (Windows IoT Sketch and Arduino Sketch)
	Please refer Windows IoT Hydroflyser's Header File: Library\Gateway (I2C)\Gateway.h
*/
#define GATEWAY_CTRL_pinMode								10
#define GATEWAY_CTRLArgumentsigitalWrite							11
#define GATEWAY_CTRLArgumentsigitalRead							12
#define GATEWAY_CTRL_analogWrite							13
#define GATEWAY_CTRL_analogRead								14
#define GATEWAY_CTRL_getManeuverBytes						15
#define GATEWAY_CTRL_getRequestedURL						16
#define GATEWAY_CTRL_getGPS									17
#define GATEWAY_CTRL_sendSensorData_ToRemoteDevice			18
#define GATEWAY_CTRL_sendString_ToRemoteDevice				19
#define GATEWAY_CTRL_sendString_ToRemoteDevice_Continued	20

/* Byte manipulation macro */
#define DoubleToByte(buf,doubleData) \
  *((double *)buf) = doubleData;

#define IntegerToByte(buf,intData) \
  *((int *)buf) = intData;

/* Global Variables */
byte CTRL, Pin, BoolValue, Temp_X[SensorBuffer_Size], Value;
uint16_t Index;
int IntValue;

/* Global Counters */
byte C1 = 0;

/* GPS Device object & Variable */
TinyGPSPlus GPDArgumentsevice;
byte GPS_SendBuffer[16], Buffer_4[4], Buffer_2[2];

/*
	GPS Device UART Link

	: WARNING :
	IF YOUR GPS DEVICE SUPPORTS 115200 BAUD, PLEASE USE 'AltSoftSerial' LIBRARY INSTEAD OF 'SoftwareSerial'
*/
SoftwareSerial GPS_UART(GPS_RX_PIN, GPS_TX_PIN);

/*
	EasyTransfer library makes it much easy to send and receive composite data over Serial.
	Create separate object for sending and receiving.
*/
EasyTransfer ETin, ETout;

/* 'RECEIVEArgumentsATA_STRUCTURE' MUST BE IDENTICAL ON GATEWAY SIDE (WITH NAME OF 'SENDArgumentsATA_STRUCTURE') */
struct RECEIVEArgumentsATA_STRUCTURE
{
	byte Arguments[Argument_Size];
};

/* 'SENDArgumentsATA_STRUCTURE' MUST BE IDENTICAL ON GATEWAY SIDE (WITH NAME OF 'RECEIVEArgumentsATA_STRUCTURE') */
struct SENDArgumentsATA_STRUCTURE
{
	char ReplyBuffer_100[SensorBuffer_Size];
};

SENDArgumentsATA_STRUCTURE mydata_out;
RECEIVEArgumentsATA_STRUCTURE mydata_in;

void setup()
{
	/* Initialize Gateway as I2C slave and register Request and Response handler */
	Wire.begin(MyGateway1);
	Wire.onRequest(onRequest);
	Wire.onReceive(onReceive);

	/* Initialize Hardware Serial to communicate with ESP8266-01 with 115200 baud. */
	Serial.begin(115200);

	/* Initialize Software Serial to communicate with GPS (uBlox Neo6mV2) with GPS_BAUD baud. */
	GPS_UART.begin(GPS_BAUD);

	/*
		Initialize ETin for incoming data while ETout for outgoing data
			+ 'OutgoingData' will holds data to be send to ESP on request
			+ Received data from ESP will be stored in 'IncomingData'
	*/
	ETin.begin(details(mydata_in), &Serial);
	ETout.begin(details(mydata_out), &Serial);

	/* Initial response to Remote Device (WP8.1/10) */
	mydata_out.ReplyBuffer_100[0] = 'B';
	mydata_out.ReplyBuffer_100[1] = 'O';
	mydata_out.ReplyBuffer_100[2] = 'O';
	mydata_out.ReplyBuffer_100[3] = 'T';
	mydata_out.ReplyBuffer_100[4] = 'I';
	mydata_out.ReplyBuffer_100[5] = 'N';
	mydata_out.ReplyBuffer_100[6] = 'G';
	mydata_out.ReplyBuffer_100[7] = ':';
	mydata_out.ReplyBuffer_100[8] = 'G';
	mydata_out.ReplyBuffer_100[9] = 'A';
	mydata_out.ReplyBuffer_100[10] = 'T';
	mydata_out.ReplyBuffer_100[11] = 'E';
	mydata_out.ReplyBuffer_100[12] = 'W';
	mydata_out.ReplyBuffer_100[13] = 'A';
	mydata_out.ReplyBuffer_100[14] = 'Y';
	mydata_out.ReplyBuffer_100[15] = '\0';

}
unsigned long LastUpdateTime;

void loop() {
	/* Periodically send response string to ESP (received from RPi2) and take back 'Arguments' from ESP */
	if (millis() - LastUpdateTime > 35)
	{
		/* Send string received from RPi2 */
		ETout.sendData();

		/* Wait for a while before receiving 'Arguments' from ESP */
		delay(25);

		/* Receive 'Arguments' from ESP */
		if (ETin.receiveData()) {}

		/* Update time to avoid frequent calls */
		LastUpdateTime = millis();
	}

	/* Read and parse GPS Data if ready */
	while (GPS_UART.available() > 0)
		if (GPDArgumentsevice.encode(GPS_UART.read()))
			FetchGPSData();
}

/* Executes when RPi2 requests number of bytes */
void onRequest()
{
	/* CTRL will be set by onReceived */
	switch (CTRL)
	{
	case GATEWAY_CTRLArgumentsigitalRead:
		Wire.write(BoolValue);
		break;
	case GATEWAY_CTRL_analogRead:
		IntegerToByte(Buffer_2, IntValue);
		Wire.write(Buffer_2, 2);
		break;
	case GATEWAY_CTRL_getManeuverBytes:
		Wire.write(mydata_in.Arguments, Argument_Size);
		break;
	case GATEWAY_CTRL_getRequestedURL:
		/* NOT IMPLEMENTED YET; CONTRIBUTION IS MOST WELCOMED */
		break;
	case GATEWAY_CTRL_getGPS:
		Wire.write(GPS_SendBuffer, 16);
		break;
	case GATEWAY_CTRL_sendSensorData_ToRemoteDevice:
	case GATEWAY_CTRL_sendString_ToRemoteDevice:
	case GATEWAY_CTRL_sendString_ToRemoteDevice_Continued:
		break;
	default:
		break;
	}
}

void onReceive(int NOB)
{
	CTRL = Wire.read();

	switch (CTRL)
	{
	case GATEWAY_CTRL_pinMode:
		Pin = Wire.read();
		Value = Wire.read();
		pinMode(Pin, Value);
		break;
	case GATEWAY_CTRLArgumentsigitalWrite:
		Pin = Wire.read();
		Value = Wire.read();
		digitalWrite(Pin, Value);
		break;
	case GATEWAY_CTRLArgumentsigitalRead:
		Pin = Wire.read();
		BoolValue = digitalRead(Pin);
		break;
	case GATEWAY_CTRL_analogWrite:
		analogWrite(Wire.read(), Wire.read());
		break;
	case GATEWAY_CTRL_analogRead:
		IntValue = analogRead(Wire.read());
		break;
	case GATEWAY_CTRL_getManeuverBytes:
	case GATEWAY_CTRL_getRequestedURL:
	case GATEWAY_CTRL_getGPS:
		break;
	case GATEWAY_CTRL_sendSensorData_ToRemoteDevice:
	case GATEWAY_CTRL_sendString_ToRemoteDevice:
		Index = 0;
		for (C1 = 0; C1 < 31; C1++)
		{
			mydata_out.ReplyBuffer_100[Index++] = Wire.read();
		}
		mydata_out.ReplyBuffer_100[Index] = '\0';
		break;
	case GATEWAY_CTRL_sendString_ToRemoteDevice_Continued:
		for (C1 = 0; C1 < 31; C1++)
		{
			mydata_out.ReplyBuffer_100[Index++] = Wire.read();
		}
		if (Index < 99)
		{
			mydata_out.ReplyBuffer_100[Index] = '\0';
		}
		break;
	default:
		break;
	}
	while (Wire.available())
	{
		Wire.read();
	}
}

void FetchGPSData()
{
	//Validation Byte
	bitWrite(GPS_SendBuffer[0], 0, GPDArgumentsevice.location.isValid());
	bitWrite(GPS_SendBuffer[0], 1, GPDArgumentsevice.altitude.isValid());
	bitWrite(GPS_SendBuffer[0], 2, GPDArgumentsevice.date.isValid());
	bitWrite(GPS_SendBuffer[0], 3, GPDArgumentsevice.time.isValid());
	bitWrite(GPS_SendBuffer[0], 4, GPDArgumentsevice.speed.isValid());
	//Future Use
	bitClear(GPS_SendBuffer[0], 5);
	bitClear(GPS_SendBuffer[0], 6);
	bitClear(GPS_SendBuffer[0], 7);

	//Location : Latitude
	DoubleToByte(Buffer_4, GPDArgumentsevice.location.lat());
	GPS_SendBuffer[1] = Buffer_4[0];
	GPS_SendBuffer[2] = Buffer_4[1];
	GPS_SendBuffer[3] = Buffer_4[2];
	GPS_SendBuffer[4] = Buffer_4[3];

	//Location : Longitude
	DoubleToByte(Buffer_4, GPDArgumentsevice.location.lng());
	GPS_SendBuffer[5] = Buffer_4[0];
	GPS_SendBuffer[6] = Buffer_4[1];
	GPS_SendBuffer[7] = Buffer_4[2];
	GPS_SendBuffer[8] = Buffer_4[3];

	//Date : Day
	GPS_SendBuffer[9] = (byte)GPDArgumentsevice.date.day();
	//Date : Month
	GPS_SendBuffer[10] = (byte)GPDArgumentsevice.date.month();
	//Date : Year
	IntegerToByte(Buffer_2, GPDArgumentsevice.date.year());
	GPS_SendBuffer[11] = Buffer_2[0];
	GPS_SendBuffer[12] = Buffer_2[1];

	//Time : Hour
	GPS_SendBuffer[13] = (byte)GPDArgumentsevice.time.hour();
	//Time : Minute
	GPS_SendBuffer[14] = (byte)GPDArgumentsevice.time.minute();
	//Time : Second
	GPS_SendBuffer[15] = (byte)GPDArgumentsevice.time.second();
}