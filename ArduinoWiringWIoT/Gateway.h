#pragma once

#define Argument_Size									6
#define SensorBuffer_Size								256

#define GATEWAY_CTRL_pinMode							10
#define GATEWAY_CTRLArgumentsigitalWrite						11
#define GATEWAY_CTRLArgumentsigitalRead						12
#define GATEWAY_CTRL_analogWrite						13
#define GATEWAY_CTRL_analogRead							14
#define GATEWAY_CTRL_getManeuverBytes					15
#define GATEWAY_CTRL_getRequestedURL					16
#define GATEWAY_CTRL_getGPS								17
#define GATEWAY_CTRL_sendSensorData_ToRemoteDevice		18
#define GATEWAY_CTRL_sendString_ToRemoteDevice			19
#define GATEWAY_CTRL_sendString_ToRemoteDevice_Continued	20

static class Gateway sealed
{
private:
	static int BytesToInt(byte, byte);
	static float BytesToDouble(byte, byte, byte, byte);
public:
	enum UsablePins
	{
		UsablePinArguments2 = 2,
		UsablePinArguments3 = 3,
		UsablePinArguments4 = 4,
		UsablePinArguments5 = 5,
		UsablePinArguments6 = 6,
		UsablePinArguments7 = 7,
		UsablePinArguments10 = 10,
		UsablePinArguments11 = 11,
		UsablePinArguments12 = 12,
		UsablePinArguments13 = 13,
		UsablePin_A0 = 14,
		UsablePin_A1 = 15,
		UsablePin_A2 = 16,
		UsablePin_A3 = 17,
		//UsablePin_A4 = 18,
		//UsablePin_A5 = 19,
		UsablePin_A6 = 19,
		UsablePin_A7 = 19
	};

	enum PinModes
	{
		PinMode_INPUT = DIRECTION_IN,
		PinMode_OUTPUT = DIRECTION_OUT,
		PinMode_INPUT_PULLUP = INPUT_PULLUP
	};

	struct GPSData
	{
		byte Valid;
		float Latitude, Longitude;
		uint16_t Year;
		byte Month, Day, Hour, Minute, Second;
	};

	static void Initialize();

	static void pinMode(int _GatewayAddress, UsablePins _Pin, UCHAR _PinMode);

	static void digitalWrite(int _GatewayAddress, UsablePins _Pin, byte _BoolValue);
	static byte digitalRead(int _GatewayAddress, UsablePins);

	static void analogWrite(int _GatewayAddress, UsablePins, byte);
	static int analogRead(int _GatewayAddress, UsablePins);

	static Platform::String ^getRequestedURL(int _GatewayAddress);
	static byte getManeuverBytes(int _GatewayAddress, byte *);
	static GPSData getGPS(int _GatewayAddress);

	static void sendSensorData_ToRemoteDevice(int _GatewayAddress, GPSData, uint16_t, byte, int, int);
	static void sendString_ToRemoteDevice(int _GatewayAddress, String);

};

