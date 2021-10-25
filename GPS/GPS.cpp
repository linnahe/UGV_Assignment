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

// global ptrs
ProcessManagement* PMSMPtr;
SM_GPS* GPSSMPtr;

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


int main()
{
	GPS GPSMod;

	// Creat TcpClient object and connect to it
	GPSMod.connect(IP_ADDRESS, GPS_PORT);
	// shared memory objects
	GPSMod.setupSharedMemory();
	// get GPS data 
	GPSMod.getData();

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
	// arrays of unsigned chars to send and receive data
	array<unsigned char>^ SendData; //unsigned char is a byte. SendData is a handle to the entire array
	array<unsigned char>^ RecvData;


	// data storage
	SendData = gcnew array<unsigned char>(1024);
	RecvData = gcnew array<unsigned char>(5000);

	Stream->Read(RecvData, 0, RecvData->Length);

	// header trapping
	unsigned int Header = 0;
	int i = 0;
	int Start;
	unsigned char Data;

	do
	{
		Data = ReadData[i++];
		Header = ((Header << 8) | Data);
	} while (Header != 0xaa44121c);

	// back to header (4 bytes)
	Start = i - 4;

	// binary data in struct
	GPSData NovatelGPS;
	unsigned char* BytePtr;
	BytePtr = (unsigned char*)(&NovatelGPS);

	for (int i = Start; i < Start + sizeof(GPSData); i++)
	{
		*(BytePtr++) = RecvData[i];
	}
	Console::WriteLine("{ 0:F3 } ", NovatelGPS.Easting); // ok

	unsigned int CalculatedCRC = CalculateBlockCRC32(108, BytePtr);
	if (CalculatedCRC == NovatelGPS.Checksum) {
		GPSSMPtr->northing = NovatelGPS.Northing;
		GPSSMPtr->easting = NovatelGPS.Easting;
		GPSSMPtr->height = NovatelGPS.Height;
	}
}

int GPS::checkData()
{
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
	PMSMPtr->Heartbeat.Flags.GPS = 0;
}

//close GPS
GPS::~GPS()
{
	Stream->Close();
	Client->Close();
}

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