/*
	ESP8266-01 sketch for Hydroflyer (Boat) (RPi2 + WinIoT)
	https://www.hackster.io/AnuragVasanwala/windows-10-iot-core-hydroflyer-f83190

	NOTE: This sketch is tested and meant for ESP8266-01.

	Objectives:
	+ Create Access Point (Act as WiFi)
	+ Listen for 'Maneuver' GET requests
	+ Fetch arguments from GET request and store them in global 'Arguments'
	+ Respond back plain-text from global buffer 'ReplyBuffer_100' which was received from gateway
	+ On root or index GET request, send back project description html page
	+ Initialize serial port and listen for incoming data
	+ When data is received from gateway and send it back global 'Arguments'
	+ Gateway will send character array to ESP which will be store in global buffer 'ReplyBuffer_100'
	+ ESP will send back character array 'ReplyBuffer_100' to requested remote device via 'Maneuver' GET request
	+ Auto-reset global Arguments if no device connects for more than 'AutoReset_Timeout'

	: WARNING :
		+ 'Argument_Size' AND 'SensorBuffer_Size' MUST BE IDENTICAL ON GATEWAY SKETCH.
		+ SERIAL BAUD RATE MUST BE SAME ON BOTH SIDE (GATEWAY & ESP).
		+ 'SEND_DATA_STRUCTURE' MUST BE IDENTICAL ON GATEWAY SIDE (WITH NAME OF 'RECEIVE_DATA_STRUCTURE').
		+ 'RECEIVE_DATA_STRUCTURE' MUST BE IDENTICAL ON GATEWAY SIDE (WITH NAME OF 'SEND_DATA_STRUCTURE').
		+ YOU MUST HAVE 'EasyTransfer' LIBRARY IN YOUR ARDUINO FOLDER.
		  TO INSTALL 'EasyTransfer', REFER: https://github.com/madsci1016/Arduino-EasyTransfer
		  THIS REPOSITORY CONTAINS:
			  + EasyTransfer
			  + EasyTransferI2C
			  + EasyTransferVirtualWire
			  + SoftEasyTransfer

			  BUT YOU ONLY HAVE TO COPY FIRST FOLDER 'EasyTransfer' TO YOUR ARDUINO's LIBRARY FOLDER
*/

/* Global Includes */
#include <ESP8266WiFi.h>
#include <EasyTransfer.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h> 


/* Arguments sizes and Timeout constants */
#define AutoReset_Timeout    500        // If no device connects for this milliseconds, ESP will reset 'Arguments'
#define Argument_Size        6          // Number of arguments to be fetch and store in global 'Arguments'
#define SensorBuffer_Size    100        // Max number of characters to be sent back to remote device

/* Create MyServer object that listens on port number 80 (which is default http port) */
ESP8266WebServer MyServer(80);

/* Access Point's SSID and Password */
const char *SsID = "WIoT - Hydroflyer";    // SsID should not be more than 15 chars
const char *Password = "12345678";

/*
	EasyTransfer library makes it much easy to send and receive composite data over Serial.
	Create separate object for sending and receiving.
*/
EasyTransfer ETin, ETout;

/* 'SEND_DATA_STRUCTURE' MUST BE IDENTICAL ON GATEWAY SIDE (WITH NAME OF 'RECEIVE_DATA_STRUCTURE') */
struct SEND_DATA_STRUCTURE
{
	byte Arguments[Argument_Size];
};

/* 'RECEIVE_DATA_STRUCTURE' MUST BE IDENTICAL ON GATEWAY SIDE (WITH NAME OF 'SEND_DATA_STRUCTURE') */
struct RECEIVE_DATA_STRUCTURE
{
	char ReplyBuffer_100[SensorBuffer_Size];
};

SEND_DATA_STRUCTURE OutgoingData;
RECEIVE_DATA_STRUCTURE IncomingData;


/* Global Counters */
uint8_t C1;
unsigned long LastAccessTime;  // To reset 'Arguments' if no connection was made in last AutoReset_Timeout milliseconds.



/* Handles index or root requests */
void HandleRoot() {
	MyServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate", false);
	MyServer.sendHeader("Pragma", "no-cache", false);
	MyServer.sendHeader("Expires", "1000", false);
	MyServer.send(200, "text/html", "<html><head><title>Windows IoT - Hydroflyer</title></head><body>Hydrofler is online.<br/>Windows IoT is the heart of the boat.<br/>To maneuver boat, please refer: www.hackster.io/AnuragVasanwala/windows-10-iot-core-hydroflyer");
}

/* Handles maneuver requests */
void HandleManeuver() {
	/* Parse arguments and store it to Arguments byte array */
	for (C1 = 0; (C1 < Argument_Size) && (C1 < MyServer.args()); C1++)
	{
		/* Convert string argument value to equivalent byte */
		OutgoingData.Arguments[C1] = (byte)(MyServer.arg(C1).toInt());
	}

	/* Prepare html headers to instruct remote device to not to cache page */
	MyServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate", false);
	MyServer.sendHeader("Pragma", "no-cache", false);
	MyServer.sendHeader("Expires", "0", false);

	/* Send data to remote device */
	MyServer.send(200, "text/plain", IncomingData.ReplyBuffer_100);

	LastAccessTime = millis();
}

void setup()
{
	/* Wait for a one and half second to make ESP stable */
	delay(1500);

	/*
		Configure and start Serial Communication with gateway.
		SERIAL BAUD RATE MUST BE IDENTICAL GATEWAY, TOO.
	*/
	Serial.begin(115200);

	/*
		Setup ESP as AP with 'SsID' and 'Password'.
		Please make sure to do not give SSID more than 15
		and password length must also be less than 10 char
		in order to achieve quick response from ESP.
	*/
	WiFi.mode(WIFI_AP);
	WiFi.softAP(SsID, Password);

	/* Handle index or root GET request on function HandleRoot */
	MyServer.on("/", HandleRoot);

	/* Handle Maneuver GET request on function HandleManeuver */
	MyServer.on("/Maneuver", HandleManeuver);

	/* Start MyServer */
	MyServer.begin();

	/* Initialize ReplyBuffer to show initial message */
	IncomingData.ReplyBuffer_100[0] = 'B';
	IncomingData.ReplyBuffer_100[1] = 'O';
	IncomingData.ReplyBuffer_100[2] = 'O';
	IncomingData.ReplyBuffer_100[3] = 'T';
	IncomingData.ReplyBuffer_100[4] = 'I';
	IncomingData.ReplyBuffer_100[5] = 'N';
	IncomingData.ReplyBuffer_100[6] = 'G';
	IncomingData.ReplyBuffer_100[7] = '\0';

	/*
		Initialize ETin for incoming data while ETout for outgoing data
			+ Received data from gateway will be stored in 'IncomingData'
			+ 'OutgoingData' will holds data to be send to gateway on request
	*/
	ETin.begin(details(IncomingData), &Serial);
	ETout.begin(details(OutgoingData), &Serial);
}

void loop()
{
	/* Periodically check for request on Web Server */
	MyServer.handleClient();

	/* If gateway sends data to ESP, receive it and send back 'Arguments' */
	if (ETin.receiveData())
	{
		/* If no device has been connected for last 'AutoReset_Timeout' time, reset Arguments */
		if (millis() - LastAccessTime > AutoReset_Timeout)
		{
			for (C1 = 0; C1 < Argument_Size; C1++)
			{
				OutgoingData.Arguments[C1] = 0;
			}
		}

		/* Send back 'Arguments' to Gateway */
		ETout.sendData();
	}
}