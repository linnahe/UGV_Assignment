#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include <SMObject.h>
#include <smstructs.h>
//#include "Laser.h"

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

int main()
{
	// instantiate shared memory object
	SMObject PMObj(TEXT("PMObj"), sizeof(ProcessManagement));

	// access shared memory
	PMObj.SMAccess();

	array<double>^ TSValues = gcnew array<double>(100);
	int TSCounter = 0;
	double TimeStamp, TimeGap; //divide by frequency(?)
	__int64 Frequency, Counter;
	__int64 OldCounter;
	int Shutdown = 0x00; //need in assignment

	QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&OldCounter);

	// initialise module flag



	while (1)
	{
		//generate timestamp
		QueryPerformanceCounter((LARGE_INTEGER*)&Counter);
		TimeGap = (double)(Counter-OldCounter) / (double)Frequency*1000; //typecast. milliseconds
		if (TSCounter <100)
			TSValues[TSCounter++] = TimeGap;
			//did PM put my flag down?
			//if (PMData->Heartbeats.Flags.Laser==0)
				//True-> put my flag up
				// PMData->Heartbeats.Flags.Laser==1;
				//False->if the PM time stamp older by agreed time period
					// if(TimeStamp - PMData->PMTimeStamp > 250) // if PMData->PMTimeStamp is processed between PM publishes its first time stamp, the reading will be wrong here
						//true->serious critical process failure -> Shutdown.Status = 0xFF;
					//false->keep going (dont need this line)
		Console::WriteLine("Laser time stamp : {0,12:F3} {1,12:X2}", TimeStamp, Shutdown); //0 is the first parameter, 12 is the feed rate, then 3 is the decimal places
		Thread::Sleep(25);
		if (ProcessManagement->Shutdown.Status) //if shutdown is non-zero then it will break and quit
			break;
		if (_kbhit()) {
			Console::ReadKey();
			break;
		}
	}
	for (int i = 0; i < 100; i++)
		Console::WriteLine("{0,12:F3", TSValues[i]);

	return 0;
}

//plot TSValues in Matlab