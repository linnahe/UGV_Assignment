//lecture 2 20/09
#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include <SMObject.h>
#include <smstructs.h>

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

#include <UGV_module.h>

ref class Vehicle : public UGV_module //Vehicle class inherits from UGV_module
{

public: //prefer to have function declarations in this file, then definitions in a .cpp file

protected:
	// YOUR CODE HERE (ADDITIONAL MEMBER VARIABLES THAT YOU MAY WANT TO ADD)
	TcpClient^ Client;
	NetworkStream^ Stream;
};

int main()
{
	//declaration
	double TimeStamp; //divide by frequency(?)
	__int64 Frequency, Counter;
	int Shutdown = 0x00; //need in assignment

	QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);

	while (1)
	{
		//generate timestamp
		QueryPerformanceCounter((LARGE_INTEGER*)&Counter);
		TimeStamp = (double)Counter / (double)Frequency * 1000; //typecast. milliseconds
		Console::WriteLine("Vehicle time stamp : {0,12:F3} {1,12:X2}", TimeStamp, Shutdown); //0 is the first parameter, 12 is the feed rate, then 3 is the decimal places
		Thread::Sleep(25);
		//if (PMSMPtr->Shutdown.Status)
		//	break;
		if (_kbhit())
			break;

		SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
		PMObj.SMAccess();
		ProcessManagement* PMSMPtr = (ProcessManagement*)PMObj.pData;
		if (PMSMPtr->Shutdown.Status)
			exit(0);
	}
	Console::ReadKey();

	return 0;
}