#using <System.dll>
#include <Windows.h>
#include <tchar.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <iostream>
#include <conio.h>

#include <SMStructs.h>
#include <SMObject.h>

using namespace System;
using namespace System::Diagnostics;
using namespace System::Net::Sockets;
using namespace System::Net;
using namespace System::Text;
using namespace System::Threading;

#define NUM_UNITS 5 //number of modules set up

bool IsProcessRunning(const char* processName);
void StartProcesses();

//defining start up sequence
TCHAR Units[10][20] = //
{
	TEXT("Laser2.exe"),
	TEXT("Display.exe"),
	TEXT("Vehicle.exe"),
	TEXT("GPS.exe"),
	TEXT("Camera.exe")
};

/*
value struct UGVProcesses
{
	String^ ModuleName;
	int Critical;
	int CrashCount;
	int CrashCountLimit;
	Process^ ProcessName;
};
*/

int main()
{
	//declaration
	double TimeStamp; //divide by frequency(?)
	__int64 Frequency, Counter;
	int Shutdown = 0x00; //need in assignment

	// generate timestamp
	QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&Counter);

	/*array<UGVProcesses>^ ProcessList = gcnew array<UGVProcesses>
	{
		{"Laser", 1, 0, 10, gcnew Process},
		{ "Display", 1, 0, 10, gcnew Process },
		{ "Vehicle", 1, 0, 10, gcnew Process },
		{ "GPS", 0, 0, 10, gcnew Process },
		{ "Camera", 0, 0, 10, gcnew Process }
	};
	*/

	// create and access shared memory
	SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
	PMObj.SMCreate();
	PMObj.SMAccess();
	/*
	if (PMObj.SMCreate() == true) // check SMCreateError flag for error trapping
	{
		Console::WriteLine("Failed to create shared memory");
	}
	if (PMObj.SMAccess() == true) //check SMAccessError flag for error trapping
	{
		Console::WriteLine("Failed to access shared memory");
	} */
	ProcessManagement* PMSMPtr = (ProcessManagement*)PMObj.pData; // ptr to SM struct

	// laser shared memory
	SMObject LaserObj(_TEXT("LaserObj"), sizeof(SM_Laser));
	LaserObj.SMCreate();
	LaserObj.SMAccess();
	SM_Laser* LSMPtr = (SM_Laser*)LaserObj.pData;

	// gps shared memory
	SMObject GPSObj(_TEXT("GPSObj"), sizeof(SM_GPS));
	GPSObj.SMCreate();
	GPSObj.SMAccess();
	SM_GPS* GPSSMPtr = (SM_GPS*)GPSObj.pData;

	// vehicle shared memory
	SMObject VehicleObj(_TEXT("VehicleObj"), sizeof(SM_VehicleControl));
	VehicleObj.SMCreate();
	VehicleObj.SMAccess();
	SM_VehicleControl* VCSMPtr = (SM_VehicleControl*)VehicleObj.pData;


	// set flags at start of program
	PMSMPtr->Shutdown.Status = 0x00;
	PMSMPtr->Shutdown.Flags.ProcessManagement = false;
	PMSMPtr->Heartbeat.Status = 0x00;		// set heartbeat for all modules

	//start all 5 modules
	StartProcesses();
	
	// while all modules are active, PM states it is active
	/*
	while (!(PMSMPtr->Heartbeat.Flags.Laser && PMSMPtr->Heartbeat.Flags.GPS && PMSMPtr->Heartbeat.Flags.VehicleControl && PMSMPtr->Heartbeat.Flags.Display && PMSMPtr->Heartbeat.Flags.Camera))
	{
		PMSMPtr->PMHeartbeat.Status = 0xFF;
	} */

	//time before starting processes
	PMSMPtr->PMTimeStamp = (double)Stopwatch::GetTimestamp();


	while (!_kbhit())
	{
		TimeStamp = (double)Counter / (double)Frequency * 1000; //typecast. milliseconds
		Console::WriteLine("Process Management time stamp : {0,12:F3} {1,12:X2}", TimeStamp, Shutdown); //0 is the first parameter, 12 is the feed rate, then 3 is the decimal places
		Thread::Sleep(100);

		while (!PMSMPtr->Shutdown.Flags.ProcessManagement) { //while process management is active
			// tell modules that pm is active
			PMSMPtr->PMHeartbeat.Status = 0xFF;

			Thread::Sleep(100);

			// detect laser heartbeat
			if (PMSMPtr->Heartbeat.Flags.Laser == 1) {
				PMSMPtr->Heartbeat.Flags.Laser = 0;
			}
			else {
				Thread::Sleep(100);
				PMSMPtr->Shutdown.Status = 0xFF; // shutdown; critical process
			}

			// detect GPS heartbeat
			if (PMSMPtr->Heartbeat.Flags.GPS == 1) {
				PMSMPtr->Heartbeat.Flags.GPS = 0;
			}
			else {
				PMSMPtr->Shutdown.Flags.GPS = 1;
				// PMSMPtr->Shutdown.Status = 0xFF;
				StartProcesses(); // restart non-critical process
			}

			// detect camera heartbeat
			if (PMSMPtr->Heartbeat.Flags.Camera == 1) {
				PMSMPtr->Heartbeat.Flags.Camera = 0;
			}
			else {
				PMSMPtr->Shutdown.Flags.Camera = 1;
				PMSMPtr->Shutdown.Status = 0xFF; // shutdown; critical process
			}

			// detect vehicle heartbeat
			if (PMSMPtr->Heartbeat.Flags.VehicleControl == 1) {
				PMSMPtr->Heartbeat.Flags.VehicleControl = 0;
			}
			else {
				PMSMPtr->Shutdown.Flags.VehicleControl = 1;
				PMSMPtr->Shutdown.Status = 0xFF; // shutdown; critical process
			}

			// detect display heartbeat
			if (PMSMPtr->Heartbeat.Flags.Display == 1) {
				PMSMPtr->Heartbeat.Flags.Display = 0;
			}
			else {
				PMSMPtr->Shutdown.Flags.Display = 1;
				PMSMPtr->Shutdown.Status = 0xFF; // shutdown; critical process
			}
		}

	}

	// shutdown PM after exiting while loop
	// PMSMPtr->Shutdown.Status = 0xFF;

	Console::WriteLine("Process management terminated normally.");
	Sleep(100);
	return 0;
}


//Is process running function
bool IsProcessRunning(const char* processName)
{
	bool exists = false;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry))
		while (Process32Next(snapshot, &entry))
			if (!_stricmp((const char *)entry.szExeFile, processName))
				exists = true;

	CloseHandle(snapshot);
	return exists;
}


void StartProcesses()
{
	STARTUPINFO s[10];
	PROCESS_INFORMATION p[10];

	for (int i = 0; i < NUM_UNITS; i++)
	{
		if (!IsProcessRunning((const char *)Units[i])) //check if each process is running
		{
			ZeroMemory(&s[i], sizeof(s[i]));
			s[i].cb = sizeof(s[i]);
			ZeroMemory(&p[i], sizeof(p[i]));

			if (!CreateProcess(NULL, Units[i], NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &s[i], &p[i])) //if process not running, then it will create that process
			{
				printf("%s failed (%d).\n", Units[i], GetLastError());
				_getch();
			}
			std::cout << "Started: " << Units[i] << std::endl;
			Sleep(100);
		}
	}
}

