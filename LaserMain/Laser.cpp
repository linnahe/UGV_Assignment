#using <System.dll>
#include <conio.h>//_kbhit()

#include <SMObject.h>
#include <smstructs.h>
#include "LaserHeader.h"

#define LASER_PORT 23000 // LMS151 port number
#define IP_ADDRESS "192.168.1.200"
#define PI 3.14159

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;
using namespace System::Net::Sockets;
using namespace System::Net;
using namespace System::Text;

// global ptrs
ProcessManagement* PMSMPtr;
SM_Laser* LSMPtr;

int main()
{
	Laser LaserMod;

	LaserMod.connect(IP_ADDRESS, LASER_PORT);
	LaserMod.setupSharedMemory();
	
	//Loop
	while (!_kbhit()) //put laser shutdown flag here to make it not shutdown
	{
		LaserMod.setHeartbeat(PMSMPtr->Heartbeat.Flags.Laser);
		
		if (LaserMod.getShutdownFlag()) {
			break;
		}

		while (!PMSMPtr->Shutdown.Flags.Laser)
		{
			LaserMod.getData();
			LaserMod.checkData();
			LaserMod.sendDataToSharedMemory();
		}
	}

	LaserMod.~Laser();


	return 0;
}

//convert laser data into integers and store them in a file or transfer them to shared memory
//use this to populate laser class
int Laser::connect(String^ hostName, int portNumber)
{
	String^ AskScan = gcnew String("sRN LMDscandata"); //AskScan handle put on the heap
	String^ StudID = gcnew String("5117757\n");
	// String to store received data for display
	String^ ResponseData;

	// Creat TcpClient object and connect to it
	Client = gcnew TcpClient(IP_ADDRESS, LASER_PORT); //create on heap
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

	SendData = System::Text::Encoding::ASCII->GetBytes(AskScan);
	return 1;
}

int Laser::setupSharedMemory()
{
	SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
	SMObject LaserObj(_TEXT("LaserObj"), sizeof(SM_Laser));
	PMObj.SMAccess();
	LaserObj.SMCreate();
	LaserObj.SMAccess();
	ProcessManagement* PMSMPtr = (ProcessManagement*)PMObj.pData;
	SM_Laser* LSMPtr = (SM_Laser*)LaserObj.pData;

	return 1;
}


int Laser::getData()
{
	Stream->WriteByte(0x02);
	Stream->Write(SendData, 0, SendData->Length);
	Stream->WriteByte(0x03);
	// Wait for the server to prepare the data, 1 ms would be sufficient, but used 10 ms
	System::Threading::Thread::Sleep(10);
	// Read the incoming data
	Stream->Read(ReadData, 0, ReadData->Length);
	// Convert incoming data from an array of unsigned char bytes to an ASCII string
	ResponseData = System::Text::Encoding::ASCII->GetString(ReadData);
	// Print the received string on the screen
	Console::WriteLine(ResponseData);
	return 1;
}

int Laser::checkData()
{
	// makes array of references to strings, completes bi-directional ethernet comms.
	array<wchar_t>^ Space = { ' ' };
	array<String^>^ StringArray = ResponseData->Split(Space);
	ResponseData = ""; // refreshes string
	if (StringArray[0] == "sRA" && StringArray[1] == "LMDscandata")
	{
		return 1;
	}
	else {
		return 0;
	}
}

int Laser::sendDataToSharedMemory()
{
	double StartAngle = System::Convert::ToInt32(StringArray[23], 16) * PI / 180; //rad to deg
	double Resolution = System::Convert::ToInt32(StringArray[24], 16) / 10000.0;

	Console::WriteLine("Start Angle     : {0,12:F3}", StartAngle);
	Console::WriteLine("Resolution      : {0,12:F3}", Resolution);

	int NumRanges = System::Convert::ToInt32(StringArray[25], 16);

	array<double>^ Range = gcnew array<double>(NumRanges);
	array<double>^ RangeX = gcnew array<double>(NumRanges);
	array<double>^ RangeY = gcnew array<double>(NumRanges);

	for (int i = 0; i < NumRanges; i++) {
		Range[i] = System::Convert::ToInt32(StringArray[26 + i], 16);
		RangeX[i] = Range[i] * sin((i * Resolution) * (PI / 180)); //convert from rad to deg
		RangeY[i] = -Range[i] * cos((i * Resolution) * (PI / 180)); //convert from rad to deg
		Console::WriteLine("polar: " + Range[i]);
		Console::WriteLine("x: " + RangeX[i] + " y: " + RangeY[i]);
		System::Threading::Thread::Sleep(100);
	}
	return 1;
}

bool Laser::getShutdownFlag()
{
	bool flag = PMSMPtr->Shutdown.Flags.Laser;
	return flag;
}

int Laser::setHeartbeat(bool heartbeat)
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
	if (PMSMPtr->Heartbeat.Flags.Laser == 0)
	{
		// true->put my flag up
		PMSMPtr->Heartbeat.Flags.Laser = 1;
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
	Console::WriteLine("Laser time stamp : {0,12:F3} {1,12:X2}", TimeStamp, Shutdown); //0 is the first parameter, 12 is the feed rate, then 3 is the decimal places
	Thread::Sleep(25);
	return 1;
}

Laser::~Laser()
{
	Stream->Close();
	Client->Close();
}