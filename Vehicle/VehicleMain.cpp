#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include <SMObject.h>
#include <smstructs.h>
#include "Vehicle.h"

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

#include <UGV_module.h>

#define VC_PORT 25000 // LMS151 port number
#define IP_ADDRESS "192.168.1.200"

// global ptrs
ProcessManagement* PMSMPtr;
SM_VehicleControl* VCSMPtr;

int main()
{
	////declaration
	//double TimeStamp; //divide by frequency(?)
	//__int64 Frequency, Counter;
	//int Shutdown = 0x00; //need in assignment

	//QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);

	//	//generate timestamp
	//	QueryPerformanceCounter((LARGE_INTEGER*)&Counter);
	//	TimeStamp = (double)Counter / (double)Frequency * 1000; //typecast. milliseconds
	//	Console::WriteLine("Vehicle time stamp : {0,12:F3} {1,12:X2}", TimeStamp, Shutdown); //0 is the first parameter, 12 is the feed rate, then 3 is the decimal places
	//	Thread::Sleep(25);
	//	//if (PMSMPtr->Shutdown.Status)
	//	//	break;
	//	if (_kbhit())
	//		break;
	Vehicle VCMod;
	VCMod.setupSharedMemory();
	VCMod.connect(IP_ADDRESS, VC_PORT);

}

int Vehicle::connect(String^ hostName, int portNumber)
{
	// create tcpclient object and connect
	Client = gcnew TcpClient(IP_ADDRESS, VC_PORT);
	String^ StudID = gcnew String("5117757\n");

	// configure connection
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;	//ms. how long the client waits for a character to be received
	Client->SendTimeout = 500;		//ms. how long the client waits for a character to be transmitted
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;
	Stream = Client->GetStream();
}

int Vehicle::setupSharedMemory()
{
	SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
	SMObject VObj(_TEXT("VObj"), sizeof(SM_VehicleControl));
	PMObj.SMAccess();
	VObj.SMAccess();
	ProcessManagement* PMSMPtr = (ProcessManagement*)PMObj.pData;
	SM_VehicleControl* VCSMPtr = (SM_VehicleControl*)VObj.pData;
}

int Vehicle::getData()
{
	SendData = gcnew array<unsigned char>(1024); //initialisation
	System::String^ Message = gcnew System::String("# ");
	Message = Message + VCSMPtr->Steering.ToString("F3") + " " + VCSMPtr->Speed.ToString("F3") + " 1 #";
	SendData = Encoding::ASCII->GetBytes(Message);
}

int Vehicle::checkData()
{
	return 1;
}

int Vehicle::sendDataToSharedMemory()
{
	return 1;
}

bool Vehicle::getShutdownFlag()
{
	return 1;
}

int Vehicle::setHeartbeat(bool heartbeat)
{
	return 1;
}


Vehicle::~Vehicle()
{
	Stream->Close();
	Client->Close();
}