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
#include "GPS.h"

#define GPS_PORT 24000 // LMS151 port number
#define IP_ADDRESS "192.168.1.200"

using namespace System;
using namespace System::IO::Ports;
using namespace System::Net::Sockets;
using namespace Text;

/*
int GPS::connect(String^ hostName, int portNumber)
{
	// YOUR CODE HERE
	return 1;
}
int GPS::setupSharedMemory()
{
	// YOUR CODE HERE
	return 1;
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
	// YOUR CODE HERE
	return 1;
}

*/

//close GPS
GPS::~GPS()
{
	Stream->Close();
	Client->Close();
}


struct GPS {
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
		// shared memory objects
		SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
		SMObject GPSObj(_TEXT("GPSObj"), sizeof(GPS));
		PMObj.SMAccess();
		GPSObj.SMAccess();
		ProcessManagement* PMSMPtr = (ProcessManagement*)PMObj.pData;
		GPS* GPSSMPtr = (GPS*)GPSObj.pData;

		// declare handle to TcpClient object
		TcpClient^ Client;
		Client = gcnew TcpClient(IP_ADDRESS, GPS_PORT);

		// configure client
		Client->NoDelay = true;
		Client->ReceiveTimeout = 500;
		Client->SendTimeout = 500;
		Client->ReceiveBufferSize = 1024;
		Client->SendBufferSize = 1024;

		// declarations
		SerialPort^ Port = nullptr;
		String^ PortName = nullptr;
		array<unsigned char>^ SendData = nullptr;
		array<unsigned char>^ RecvData = nullptr;
		unsigned int Checksum;
		double Northing;
		double Easting;
		double Height;

		// instantiations
		Port = gcnew SerialPort;
		// PortName = gcnew String(“COM1”);
		SendData = gcnew array<unsigned char>(16);
		RecvData = gcnew array<unsigned char>();

		// configurations
		Port->PortName = PortName;
		Port->BaudRate = 115200;
		Port->StopBits = StopBits::One;
		Port->DataBits = 8;
		Port->Parity = Parity::None;
		Port->Handshake = Handshake::None;

		// Set the read/write timeouts & buffer size
		Port->ReadTimeout = 500;
		Port->WriteTimeout = 500;
		//Port->ReadBufferSize = ;
		Port->WriteBufferSize = 1024;

		// actions
		Port->Open();
		Port->Read(RecvData, 0, sizeof(GPS));


		// data storage
		SendData = gcnew array<unsigned char>(1024);
		Message = gcnew String("# ");
		Message = Message + steer.ToString("F3") + " " + speed.ToString("F3") + " 1 #"; // flag can be 0 or 1
		SendData = Encoding::ASCII->GetBytes(Message);

		// binary data in struct
		GPS GPSData;
		unsigned char* BytePtr;
		BytePtr = (unsigned char*)(&GPSData);
		for (int i = 0; i < sizeof(GPS); i++)
			SendData[i] = *(BytePtr + i);

		// TCP send to server
		NetworkStream^ Stream = Client->GetStream();
		Stream->Write(SendData, 0, SendData->Length);

		// TCP receiver from server
		array<unsigned char>^ RecvData;
		RecvData = gcnew array<unsigned char>(5000);
		NetworkStream^ Stream = Client->GetStream();
		Stream->Read(RecvData, 0, RecvData->Length);

		int NumData = 0;
		while (NumData != sizeof(GPS))
			NumData += Stream->Read(RecvData, NumData, sizeof(GPS) - NumData);

		// header trapping
		unsigned int Header = 0;
		int i = 0;
		int Start; //Start of data
		do
		{
			Header = ((Header << 8) | RecvData[i++]);
		} while (Header != 0xaa44121c);
		Start = i - 4;

		// loading binary data into struct
		GPS NovatelGPS;
		unsigned char* BytePtr = (unsigned char*)&NovatelGPS;
		for (int i = Start; i < Start + sizeof(GPS); i++)
		{
			*(BytePtr++) = RecvData[i];
		}
		Console::WriteLine("{ 0:F3 } ", NovatelGPS.Easting); // ok

		
		if (PMSMPtr->Shutdown.Status)
			exit(0);
	}
	return 0;
}