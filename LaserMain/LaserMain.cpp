//lecture 2 20/09
#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include <SMObject.h>
#include <smstructs.h>

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

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
		TimeStamp = (double)Counter / (double)Frequency*1000; //typecast. milliseconds
		Console::WriteLine("Laser time stamp : {0,12:F3} {1,12:X2}", TimeStamp, Shutdown); //0 is the first parameter, 12 is the feed rate, then 3 is the decimal places
		Thread::Sleep(25);
		if (PMData->Shutdown.Status) //if shutdown is non-zero then it will break and quit
			break;
		if (_kbhit())
			break;
	}
	Console::ReadKey();

	return 0;
}