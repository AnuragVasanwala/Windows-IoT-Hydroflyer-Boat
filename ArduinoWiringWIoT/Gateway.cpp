#include <Wire.h>
#include "Gateway.h"

int Gateway::BytesToInt(byte MSB, byte LSB)
{
	byte Temp[2];
	Temp[0] = MSB;
	Temp[1] = LSB;
	return (*((int16_t *)Temp));
}

float Gateway::BytesToDouble(byte MSB, byte B2, byte B3, byte LSB)
{
	byte Temp[4];
	Temp[0] = MSB;
	Temp[1] = B2;
	Temp[2] = B3;
	Temp[3] = LSB;
	return (*((float *)Temp));
}

void Gateway::Initialize()
{
	/* Start I2C Master */
	Wire.begin();

	delay(10);
}

void Gateway::pinMode(int _GatewayAddress, UsablePins _Pin, UCHAR _PinMode)
{
	byte Temp_3[3];

	/* Prepare control bytes */
	Temp_3[0] = GATEWAY_CTRL_pinMode;	// First Byte: Control byte indicates what to do on slave side
	Temp_3[1] = (byte)_Pin;				// Second and so on bytes are then treated based upon control byte
	Temp_3[2] = _PinMode;			//

									/* Send byte array to slave gateway */
	Wire.beginTransmission(_GatewayAddress);
	Wire.write(Temp_3, 3);
	Wire.endTransmission();
	delay(5);
}

void Gateway::digitalWrite(int _GatewayAddress, UsablePins _Pin, byte _BoolValue)
{
	byte Temp_3[3];

	/* Prepare control bytes */
	Temp_3[0] = GATEWAY_CTRLArgumentsigitalWrite;	// First Byte: Control byte indicates what to do on slave side
	Temp_3[1] = (byte)_Pin;					// Second and so on bytes are then treated based upon control byte
	Temp_3[2] = (byte)_BoolValue;			//

											/* Send byte array to slave gateway */
	Wire.beginTransmission(_GatewayAddress);
	Wire.write(Temp_3, 3);
	Wire.endTransmission();
	delay(5);
}

byte Gateway::digitalRead(int _GatewayAddress, UsablePins _Pin)
{
	delay(5);
	byte Temp_2[2];

	/* Prepare control bytes */
	Temp_2[0] = GATEWAY_CTRLArgumentsigitalRead;	// First Byte: Control byte indicates what to do on slave side
	Temp_2[1] = (byte)_Pin;					// Second and so on bytes are then treated based upon control byte

											/* Send byte array to slave gateway */
	Wire.beginTransmission(_GatewayAddress);
	Wire.write(Temp_2, 2);
	Wire.endTransmission();

	return Wire.requestFrom(_GatewayAddress, 1);
}

void Gateway::analogWrite(int _GatewayAddress, UsablePins _Pin, byte _Value)
{
	delay(7);
	byte Temp_3[3];

	/* Prepare control bytes */
	Temp_3[0] = GATEWAY_CTRL_analogWrite;	// First Byte: Control byte indicates what to do on slave side
	Temp_3[1] = (byte)_Pin;					// Second and so on bytes are then treated based upon control byte
	Temp_3[2] = (byte)_Value;			//

										/* Send byte array to slave gateway */
	Wire.beginTransmission(_GatewayAddress);
	Wire.write(Temp_3, 3);
	Wire.endTransmission();
}

int Gateway::analogRead(int _GatewayAddress, UsablePins _Pin)
{
	delay(5);
	byte Temp_2[2];

	/* Prepare control bytes */
	Temp_2[0] = GATEWAY_CTRL_analogRead;	// First Byte: Control byte indicates what to do on slave side
	Temp_2[1] = (byte)_Pin;					// Second and so on bytes are then treated based upon control byte

											/* Send byte array to slave gateway */
	Wire.beginTransmission(_GatewayAddress);
	Wire.write(Temp_2, 2);
	Wire.endTransmission();

	Wire.requestFrom(_GatewayAddress, 2);

	Temp_2[0] = Wire.read();
	Temp_2[1] = Wire.read();

	return BytesToInt(Temp_2[0], Temp_2[1]);
}

Platform::String ^ Gateway::getRequestedURL(int _GatewayAddress)
{
	delay(5);
	return "";
}

byte Gateway::getManeuverBytes(int _GatewayAddress, byte *Buffer)
{
	delay(5);
	byte Temp_1[1];

	Temp_1[0] = GATEWAY_CTRL_getManeuverBytes;

	/* Clean-up Buffer */
	for (uint16_t C1 = 0; C1 < Argument_Size; C1++)
	{
		Buffer[C1] = 0;
	}

	/* Send byte array to slave gateway */
	Wire.beginTransmission(_GatewayAddress);
	Wire.write(Temp_1, 1);
	Wire.endTransmission();

	Wire.requestFrom(_GatewayAddress, Argument_Size);

	for (uint16_t C1 = 0; C1 < Argument_Size; C1++)
	{
		Buffer[C1] = Wire.read();
	}

	return Argument_Size;
}

Gateway::GPSData Gateway::getGPS(int _GatewayAddress)
{
	delay(5);
	byte Temp_16[16];
	GPSData Result;

	Temp_16[0] = GATEWAY_CTRL_getGPS;

	/* Send byte array to slave gateway */
	Wire.beginTransmission(_GatewayAddress);
	Wire.write(Temp_16, 1);
	Wire.endTransmission();

	/* Request and read byte array for GPS Data */
	Wire.requestFrom(_GatewayAddress, 16);
	for (byte C1 = 0; C1 < 16; C1++)
	{
		Temp_16[C1] = Wire.read();
	}

	/*
	Gateway will send GPS data in 16 different bytes
	BYTE 0:
	BIT : 0 - Valid Bit for Location
	BIT : 1 - Valid Bit for Altitude
	BIT : 2 - Valid Bit for Date
	BIT : 3 - Valid Bit for Time
	BIT : 4 - Valid Bit for Speed
	BIT : 5 - RESERVED
	BIT : 6 - RESERVED
	BIT : 7 - RESERVED

	BYTE 1,2,3,4 - Latitude
	BYTE 5,6,7,8 - Longitude
	BYTE 9       - Day
	BYTE 10      - Month
	BYTE 11,12   - Year
	BYTE 13      - Hour
	BYTE 14      - Minute
	BYTE 15      - Second
	*/

	Result.Valid = Temp_16[0];
	Result.Latitude = Gateway::BytesToDouble(Temp_16[1], Temp_16[2], Temp_16[3], Temp_16[4]);
	Result.Longitude = Gateway::BytesToDouble(Temp_16[5], Temp_16[6], Temp_16[7], Temp_16[8]);
	Result.Day = Temp_16[9];
	Result.Month = Temp_16[10];
	Result.Year = Gateway::BytesToInt(Temp_16[11], Temp_16[12]);
	Result.Hour = Temp_16[13];
	Result.Minute = Temp_16[14];
	Result.Second = Temp_16[15];

	return Result;
}

void Gateway::sendSensorData_ToRemoteDevice(int _GatewayAddress, GPSData _GPSData, uint16_t _Compass, byte _LeakAlert, int _Battery1_Volts = -1, int _Battery2_Volts = -1)
{
	delay(5);
	String Result = "";
	Result += _GPSData.Valid;
	Result += "|";
	Result += _GPSData.Latitude;
	Result += "|";
	Result += _GPSData.Longitude;
	Result += "|";
	Result += _GPSData.Year;
	Result += "|";
	Result += _GPSData.Month;
	Result += "|";
	Result += _GPSData.Day;
	Result += "|";
	Result += _GPSData.Hour;
	Result += "|";
	Result += _GPSData.Minute;
	Result += "|";
	Result += _GPSData.Second;
	Result += "|";
	Result += _Compass;
	Result += "|";
	Result += _LeakAlert;
	Result += "|";
	Result += _Battery1_Volts;
	Result += "|";
	Result += _Battery2_Volts;
	Gateway::sendString_ToRemoteDevice(_GatewayAddress, Result);
}

void Gateway::sendString_ToRemoteDevice(int _GatewayAddress, String BufferToBeSend)
{
	delay(5);
	uint16	C1;
	int Length = BufferToBeSend.length();

	if (Length > 31)
	{
		byte Temp_32[32];
		byte TotalIteration = 0;
		byte CurrentIteration = 1;

		while (Length > 0)
		{
			TotalIteration += 1;
			Length -= 31;
		}

		/* Prepare control bytes */
		Temp_32[0] = GATEWAY_CTRL_sendString_ToRemoteDevice;	// First Byte: Control byte indicates what to do on slave side

		for (C1 = 0; C1 < 31; C1++)
		{
			Temp_32[C1 + 1] = BufferToBeSend.charAt(C1);
		}

		if (C1 < 31)
		{
			Temp_32[C1] = '\0';
		}

		/* Send byte array to slave gateway */
		Wire.beginTransmission(_GatewayAddress);
		Wire.write(Temp_32, 32);
		Wire.endTransmission();

		delay(5);

		while (CurrentIteration < TotalIteration)
		{
			/* Prepare control bytes */
			Temp_32[0] = GATEWAY_CTRL_sendString_ToRemoteDevice_Continued;	// First Byte: Control byte indicates what to do on slave side

			for (C1 = 0; C1 < 31; C1++)
			{
				Temp_32[C1 + 1] = BufferToBeSend.charAt((CurrentIteration * 31) + C1);
			}
			if (C1 < 31)
			{
				Temp_32[C1] = '\0';
			}

			/* Send byte array to slave gateway */
			Wire.beginTransmission(_GatewayAddress);
			Wire.write(Temp_32, 32);
			Wire.endTransmission();

			CurrentIteration++;
			delay(5);
		}
	}
	else
	{
		byte Temp_32[32];

		/* Prepare control bytes */
		Temp_32[0] = GATEWAY_CTRL_sendString_ToRemoteDevice;	// First Byte: Control byte indicates what to do on slave side

		for (C1 = 0; C1 < 31; C1++)
		{
			Temp_32[C1 + 1] = BufferToBeSend.charAt(C1);
		}

		if (C1 < 31)
		{
			Temp_32[C1] = '\0';
		}

		/* Send byte array to slave gateway */
		Wire.beginTransmission(_GatewayAddress);
		Wire.write(Temp_32, 32);
		Wire.endTransmission();
		delay(5);
	}

}
