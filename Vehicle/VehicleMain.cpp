#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include <SMObject.h>
#include <smstructs.h>
#include "Vehicle.h"
#include <UGV_module.h>

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;


#define VC_PORT 25000 // LMS151 port number
#define IP_ADDRESS "192.168.1.200"

// global ptrs
ProcessManagement* PMSMPtr = NULL;
SM_VehicleControl* VCSMPtr = NULL;

int main()
{
	Vehicle VCMod;
	VCMod.setupSharedMemory();
	VCMod.connect(IP_ADDRESS, VC_PORT);

	while (1) //put vehicle shutdown flag here to make it not shutdown
	{
		VCMod.setHeartbeat(PMSMPtr->Heartbeat.Flags.VehicleControl);

		if (VCMod.getShutdownFlag()) {
			break;
		}

		while (!PMSMPtr->Shutdown.Flags.VehicleControl)
		{
			VCMod.getData();
			VCMod.sendDataToSharedMemory();
		}
	}
	VCMod.~Vehicle();
	Console::ReadKey();
	return 0;

}

int Vehicle::connect(String^ hostName, int portNumber)
{
	String^ AskScan = gcnew String("sRN LMDscandata"); //AskScan handle put on the heap
	StudID = gcnew String("5117757\n");
	// String to store received data for display
	String^ ResponseData;

	// Creat TcpClient object and connect to it
	Client = gcnew TcpClient(IP_ADDRESS, VC_PORT); //create on heap
	// Configure connection
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;//ms. how long the client waits for a character to be received
	Client->SendTimeout = 500;//ms. how long the client waits for a character to be transmitted
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;

	// unsigned char arrays of 16 bytes each are created on managed heap. ASCII characters
	SendData = gcnew array<unsigned char>(16); //16 chars
	ReadData = gcnew array<unsigned char>(2500); //read up to 2500 chars

	// Get the network stream object associated with client so we can use it to read and write
	NetworkStream^ Stream = Client->GetStream(); //CLR object, Stream handle initialised, putting on heap

	/*
	// Convert string command to an array of unsigned char
	SendData = System::Text::Encoding::ASCII->GetBytes(StudID); //AskScan string is a readable ASCII, convert it into binary bytes, then put into SendData
	// authenticate user
	Stream->Write(SendData, 0, SendData->Length);
	// Wait for the server to prepare the data, 1 ms would be sufficient, but used 10 ms
	System::Threading::Thread::Sleep(10);
	// Read the incoming data
	Stream->Read(ReadData, 0, ReadData->Length); //ReadData is binary here, need to convert to ASCII then print
	ResponseData = System::Text::Encoding::ASCII->GetString(ReadData);
	// Print the received string on the screen
	Console::WriteLine(ResponseData);
	//Console::ReadKey(); 
	SendData = System::Text::Encoding::ASCII->GetBytes(AskScan); */
	return 1;
}

int Vehicle::setupSharedMemory()
{
	ProcessManagementData = new SMObject(_TEXT("PMObj"), sizeof(ProcessManagement));
	SensorData = new SMObject(_TEXT("LaserObj"), sizeof(SM_VehicleControl));
	ProcessManagementData->SMCreate();
	ProcessManagementData->SMAccess();
	SensorData->SMCreate();
	SensorData->SMAccess();
	PMSMPtr = (ProcessManagement*)ProcessManagementData->pData;
	VCSMPtr = (SM_VehicleControl*)SensorData->pData;

	return 1;
}

int Vehicle::getData()
{
	// retrieve data from display
	// SendData = gcnew array<unsigned char>(16);
	SendData = System::Text::Encoding::ASCII->GetBytes(StudID);
	Stream->Write(SendData, 0, SendData->Length);
	// SendData = System::Text::Encoding::ASCII->GetBytes(ReadData);
	Thread::Sleep(100);

	return 1;
}

int Vehicle::checkData()
{
	return 1;
}

int Vehicle::sendDataToSharedMemory()
{
	int flag = 0; // toggles to 1
	SendData = gcnew array<unsigned char>(1024); //initialisation
	System::String^ Message = gcnew System::String("# ");
	Message = Message + VCSMPtr->Steering.ToString("F3") + " " + VCSMPtr->Speed.ToString("F3") + flag + " #"; // sends data to weeder
	SendData = Encoding::ASCII->GetBytes(Message);

	//print vc data
	Console::WriteLine(Message);
	flag = !flag; //toggles

	return 1;
}

bool Vehicle::getShutdownFlag()
{
	bool flag = PMSMPtr->Shutdown.Status;
	return flag;
}

int Vehicle::setHeartbeat(bool heartbeat)
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
	if (PMSMPtr->Heartbeat.Flags.VehicleControl == 0)
	{
		// true->put my flag up
		PMSMPtr->Heartbeat.Flags.VehicleControl = 1;
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
	Console::WriteLine("Vehicle Control time stamp : {0,12:F3} {1,12:X2}", TimeStamp, Shutdown); //0 is the first parameter, 12 is the feed rate, then 3 is the decimal places
	Thread::Sleep(25);
	return 1;
}


Vehicle::~Vehicle()
{
	Stream->Close();
	Client->Close();
}