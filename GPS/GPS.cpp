#include "GPS.h"
#using <System.dll>
#include <Windows.h>
#include <tchar.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <iostream>
#include <conio.h>

#include <SMStructs.h>
#include <SMObject.h>

#define GPS_PORT 24000 // LMS151 port number
#define IP_ADDRESS "192.168.1.200"

using namespace System;
using namespace System::IO::Ports;
using namespace System::Net::Sockets;
using namespace Text;

struct GPSData {
	unsigned int Header; // 4 bytes
	unsigned char Discard1[40]; // 40 bytes
	double Northing; // 8 bytes
	double Easting; // 8 bytes
	double Height; // 8 bytes
	unsigned char Discard2[40]; // 40 bytes
	unsigned int Checksum; // 4 bytes
	// total 112 bytes
};

// these two CRC32 functions are code for calculating a checksum to verify that the GPS data has been received correctly by our code
// don't need to modify these but need to call them

unsigned long CRC32Value(int i)
{
	int j;
	unsigned long ulCRC;
	ulCRC = i;
	for (j = 8; j > 0; j--)
	{
		if (ulCRC & 1)
			ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
		else
			ulCRC >>= 1;
	}
	return ulCRC;
}

unsigned long CalculateBlockCRC32(unsigned long ulCount, /* Number of bytes in the data block */
	unsigned char* ucBuffer) /* Data block */
{
	unsigned long ulTemp1;
	unsigned long ulTemp2;
	unsigned long ulCRC = 0;
	while (ulCount-- != 0)
	{
		ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
		ulTemp2 = CRC32Value(((int)ulCRC ^ *ucBuffer++) & 0xff);
		ulCRC = ulTemp1 ^ ulTemp2;
	}
	return(ulCRC);
}

int main()
{
	while (1)
	{
		GPS GPSMod;
		// shared memory objects
		GPSMod.setupSharedMemory();

		// arrays of unsigned chars to send and receive data
		array<unsigned char>^ SendData; //unsigned char is a byte. SendData is a handle to the entire array
		array<unsigned char>^ RecvData;

		// Creat TcpClient object and connect to it
		GPSMod.connect(IP_ADDRESS, GPS_PORT);

		// data storage
		SendData = gcnew array<unsigned char>(1024); //1024 chars
		RecvData = gcnew array<unsigned char>(5000);


		// binary data in struct
		GPSData NovatelGPS;
		unsigned char* BytePtr;
		BytePtr = (unsigned char*)(&NovatelGPS);
		

		// Stream->Read(RecvData, 0, RecvData->Length);

		// header trapping
		unsigned int Header = 0;
		int i = 0;
		int Start; //Start of data
		do
		{
			Header = ((Header << 8) | RecvData[i++]);
		} while (Header != 0xaa44121c);
		Start = i - 4;

		for (int i = Start; i < Start + sizeof(GPSData); i++)
		{
			*(BytePtr++) = RecvData[i];
		}
		Console::WriteLine("{ 0:F3 } ", NovatelGPS.Easting); // ok

	}
	return 0;
}

int GPS::connect(String^, int)
{
	// create tcpclient object and connect
	Client = gcnew TcpClient(IP_ADDRESS, GPS_PORT);

	// configure connection
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;	//ms. how long the client waits for a character to be received
	Client->SendTimeout = 500;		//ms. how long the client waits for a character to be transmitted
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;
	
	Stream = Client->GetStream();
}

int GPS::setupSharedMemory()
{
	SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
	SMObject GPSObj(_TEXT("GPSObj"), sizeof(SM_GPS));
	PMObj.SMAccess();
	GPSObj.SMAccess();
	ProcessManagement* PMSMPtr = (ProcessManagement*)PMObj.pData;
	SM_GPS* GPSSMPtr = (SM_GPS*)GPSObj.pData;
}

int GPS::getData()
{
	// YOUR CODE HERE
	return 1;
}

int GPS::checkData()
{
	// YOUR CODE HERE
	return 1;
}

int GPS::sendDataToSharedMemory()
{
	// YOUR CODE HERE
	return 1;
}

bool GPS::getShutdownFlag()
{
	// YOUR CODE HERE
	return 1;
}

int GPS::setHeartbeat(bool heartbeat)
{
	//PMSMPtr->Heartbeat.Flags.GPS = 0;
	return 1;
}

//close GPS
GPS::~GPS()
{
	Stream->Close();
	Client->Close();
}