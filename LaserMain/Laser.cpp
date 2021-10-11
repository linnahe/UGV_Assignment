//Compile in a C++ CLR empty project
//connect to MTRN-WAP1
#using <System.dll>
#include <conio.h>//_kbhit()

#include <SMObject.h>
#include <smstructs.h>
//#include "Laser.h"

#define LASER_PORT "23000"
#define PLATFORM_ADDRESS "192.168.1.200"

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

using namespace System::Net::Sockets;
using namespace System::Net;
using namespace System::Text;

int main()
{
	//shared memory object
	SMObject PMObj(_TEXT("PMObj"), sizeof(SM_Modules));
	PMObj.SMAccess();
	SM_Modules* PMSMPtr = (SM_Modules*)PMObj.pData;

	// LMS151 port number must be 23000
	int PortNumber = 23000;
	// Pointer to TcpClent type object on managed heap
	TcpClient^ Client; //handle to object
	// arrays of unsigned chars to send and receive data
	array<unsigned char>^ SendData; //unsigned char is a byte. SendData is a handle to the entire array
	array<unsigned char>^ ReadData;
	// String command to ask for Channel 1 analogue voltage from the PLC
	// These command are available on Galil RIO47122 command reference manual
	// available online
	String^ AskScan = gcnew String("sRN LMDscandata"); //AskScan handle put on the heap
	String^ StudID = gcnew String("5117757\n");
	// String to store received data for display
	String^ ResponseData;

	// Creat TcpClient object and connect to it
	Client = gcnew TcpClient("192.168.1.200", PortNumber); //create on heap
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
	Console::ReadKey();

	SendData = System::Text::Encoding::ASCII->GetBytes(AskScan);

	//Loop
	while (!PMSMPtr->PMSM.Shutdown.Flags.Laser) //put laser shutdown flag here to make it not shutdown
	{
		//class: data acquisition or get range part. need to do range calculation

		// Write command asking for data
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

		//lec5 slides
		// makes array of references to strings, completes bi-directional ethernet comms.
		array<wchar_t>^ Space = { ' ' };
		array<String^>^ StringArray = ResponseData->Split(Space);

		double StartAngle = System::Convert::ToInt32(StringArray[23], 16);
		double Resolution = System::Convert::ToInt32(StringArray[24], 16) / 10000.0;
		int NumRanges = System::Convert::ToInt32(StringArray[25], 16);

		array<double>^ Range = gcnew array<double>(NumRanges);
		array<double>^ RangeX = gcnew array<double>(NumRanges);
		array<double>^ RangeY = gcnew array<double>(NumRanges);

		for (int i = 0; i < NumRanges; i++) {
			Range[i] = System::Convert::ToInt32(StringArray[26 + i], 16);
			RangeX[i] = Range[i] * sin(i * Resolution);
			RangeY[i] = Range[i] * cos(i * Resolution);
		}

		if (_kbhit()) {
			Console::ReadKey();
			break;
		}
	}

	//can put these in the laser class destructor
	Stream->Close();
	Client->Close();

	Console::ReadKey();
	Console::ReadKey();


	return 0;
}

//convert laser data into integers and store them in a file or transfer them to shared memory
//use this to populate laser class