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
using namespace System::Diagnostics;
using namespace System::Threading;
using namespace System::IO::Ports;
using namespace System::Net::Sockets;
using namespace Text;

// global ptrs
ProcessManagement* PMSMPtr = NULL;
SM_GPS* GPSSMPtr = NULL;

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

GPSData NovatelGPS;
unsigned char* BytePtr;

int main()
{
	GPS GPSMod;

	// Creat TcpClient object and connect to it
	GPSMod.connect(IP_ADDRESS, GPS_PORT);
	// shared memory objects
	GPSMod.setupSharedMemory();

	while (!_kbhit())
	{
		GPSMod.setHeartbeat(PMSMPtr->Heartbeat.Flags.GPS);
		// get GPS data 
		while (!PMSMPtr->Shutdown.Flags.GPS)
		{
			GPSMod.getData();
			if (GPSMod.checkData())
			{
				GPSMod.sendDataToSharedMemory();
			}
		}
	
		if (GPSMod.getShutdownFlag())
		{
			break;
		}
	}

	//destructor
	GPSMod.~GPS();

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

	return 1;
}

int GPS::setupSharedMemory()
{
	SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
	SMObject GPSObj(_TEXT("GPSObj"), sizeof(SM_GPS));
	PMObj.SMAccess();
	GPSObj.SMAccess();
	ProcessManagement* PMSMPtr = (ProcessManagement*)PMObj.pData;
	SM_GPS* GPSSMPtr = (SM_GPS*)GPSObj.pData;

	return 1;
}

int GPS::getData()
{
	// arrays of unsigned chars to send and receive data
	array<unsigned char>^ SendData; //unsigned char is a byte. SendData is a handle to the entire array
	array<unsigned char>^ RecvData;


	// data storage
	SendData = gcnew array<unsigned char>(1024);
	RecvData = gcnew array<unsigned char>(5000);

	// Stream->Read(RecvData, 0, RecvData->Length);

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

	BytePtr = (unsigned char*)(&NovatelGPS);

	for (int i = Start; i < Start + sizeof(GPSData); i++)
	{
		*(BytePtr++) = RecvData[i];
	}
	Console::WriteLine("{ 0:F3 } ", NovatelGPS.Easting); // ok

	return 1;
}

int GPS::checkData()
{
	unsigned int CalculatedCRC = CalculateBlockCRC32(108, BytePtr);
	if (CalculatedCRC == NovatelGPS.Checksum) {
		GPSSMPtr->northing = NovatelGPS.Northing;
		GPSSMPtr->easting = NovatelGPS.Easting;
		GPSSMPtr->height = NovatelGPS.Height;
		return 1;
	}
	else {
		return 0;
	}
}

int GPS::sendDataToSharedMemory()
{
	GPSSMPtr->northing = NovatelGPS.Northing;
	GPSSMPtr->easting = NovatelGPS.Easting;
	GPSSMPtr->height = NovatelGPS.Height;
	Console::WriteLine("Northing : {0,12:F3}", NovatelGPS.Northing);
	Console::WriteLine("Easting : {0,12:F3}", NovatelGPS.Easting);
	Console::WriteLine("Height : {0,12:F3}", NovatelGPS.Height);
	Console::WriteLine("***"); //new line
	return 1;
}

bool GPS::getShutdownFlag()
{
	bool flag = PMSMPtr->Shutdown.Flags.GPS;
	return flag;
}

int GPS::setHeartbeat(bool heartbeat)
{
	array<double>^ TSValues = gcnew array<double>(100);
	int TSCounter = 0;
	double TimeStamp; //divide by frequency(?)
	__int64 Frequency, Counter;
	__int64 OldCounter;
	int Shutdown = 0x00; //need in assignment

	//generate timestamp
	QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&Counter);
	TimeStamp = (double)(Counter) / (double)Frequency * 1000; //typecast. milliseconds
	if (TSCounter < 100)
		TSValues[TSCounter++] = TimeStamp;
	//did PM put my flag down?
	if (PMSMPtr->Heartbeat.Flags.GPS == 0)
	{
		// true->put my flag up
		PMSMPtr->Heartbeat.Flags.GPS = 1;
	}
	else {
		//False->if the PM time stamp older by agreed time period
		if (TimeStamp - PMSMPtr->PMTimeStamp > 2000) // if PMData->PMTimeStamp is processed between PM publishes its first time stamp, the reading will be wrong here
		{
			//true->serious critical process failure
			PMSMPtr->Shutdown.Status = 0xFF;
			Console::WriteLine("Process Management Failed");
		} //false->keep going (dont need this line)
	}
	Console::WriteLine("GPS time stamp : {0,12:F3} {1,12:X2}", TimeStamp, Shutdown); //0 is the first parameter, 12 is the feed rate, then 3 is the decimal places
	Thread::Sleep(25);
	return 1;
}

//close GPS
GPS::~GPS()
{
	Stream->Close();
	Client->Close();
}

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