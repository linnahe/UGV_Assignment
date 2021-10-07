//lecture 2 20/09
#using <System.dll>
#include <conio.h>
#include <SMObject.h>
#include <smstructs.h>

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;


int main()
{
	//tele-operation
	//template class array (of e.g. int, double)
	//declarations + initialisations
	SMObject PMObj(TEXT("ProcessManagement"), sizeof(ProcessManagement)); //created an object

	array<String^>^ ModuleList = gcnew array<String^>{"Laser", "Display", "Vehicle", "GPS", "Camera"}; //modulelist is a set of handles
	array<int>^ Critical = gcnew array<int>(ModuleList->Length) { 1, 1, 1, 0, 0 }; //critical is a handle, not a set of handles
	array<Process^>^ ProcessList = gcnew array<Process^>(ModuleList->Length); //arrays of pointers to process. not initialised

	//SM Creation and seeking access
	PMObj.SMCreate(); //create file
	PMObj.SMAccess(); //give read and write access


	ProcessManagement* PMData = (ProcessManagement*)PMObj.pData; //pointer 

	for (int i = 0; i < ModuleList->Length; i++) //starting the processes
	{
		if (Process::GetProcessesByName(ModuleList[i])->Length == 0) //how many modules/processes are runnning. output is an array. proces is from diagnostics module
		{
			ProcessList[i] = gcnew Process;
			ProcessList[i]->StartInfo->FileName = ModuleList[i]; //putting the file name to be executed
			//set working directory to executables folder
			// ProcessList[i]->StartInfo->WorkingDirectory = "D:\\_jlj" instead of line 35
			ProcessList[i]->Start(); //start the process, executes filename
			Console::WriteLine("The process " + ModuleList[i] + ".exe started");
		}
	}

	//main loop
	while (!_kbhit()) //while no key is pressed
	{
		// checking for heartbeats
			//iterate through all the processes
				//is heartbeat bit of process [i] up?
					//True->put the bit of process[i] down
					//False->Increment the counter (heartbeat lost counter) add something on line 22 for Counters
						//is the counter passed the limit for process[i]
							//True->is process[i] critical
								//true->shutdown all
								//false->has process[i] exited the operating system (HasExited())
									//true-> start();
									//false-> kill(); wait then start();
							//False-> increment counter for process[i]
		Thread::Sleep(1000); //try change this to 25ms or whatever
	}

	// PMData->Shutdown.Status = 0xFF; //as soon as it shutdowns, changes shared memory variable
	//clearing and shutdown
	Console::ReadKey(); //doesnt exit terminal but stops. so have to delete this line

	return 0;
}