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
GPS::~GPS()
{
	// YOUR CODE HERE
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

int main()
{
	while (1)
	{
		SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
		PMObj.SMAccess();
		ProcessManagement* PMSMPtr = (ProcessManagement*)PMObj.pData;
		if (PMSMPtr->Shutdown.Status)
			exit(0);
	}
	return 0;
}