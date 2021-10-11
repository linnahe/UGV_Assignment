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

#define NUM_UNITS 5 //number of modules set up

bool IsProcessRunning(const char* processName);
void StartProcesses();

//defining start up sequence
TCHAR Units[10][20] = //
{
	TEXT("GPS.exe"),
	TEXT("Display.exe"),
	TEXT("Camera.exe"),
	TEXT("Laser.exe"),
	TEXT("Vehicle.exe")
};

int main()
{
	// instantiate SMObject
	SMObject PMObj(_TEXT("PMObj"), sizeof(SM_Modules));
	/*
	SMObject LaserObj(_TEXT("LaserObj"), sizeof(SM_Laser));
	SMObject GPSObj(_TEXT("GPSObj"), sizeof(SM_GPS));
	SMObject VehicleObj(_TEXT("VehicleObj"), sizeof(SM_VehicleControl))
	*/

	// create and access shared memory
	PMObj.SMCreate(); // check SMCreateError flag for error trapping
	PMObj.SMAccess(); //check SMAccessError flag for error trapping

	// ptr to SM struct
	SM_Modules* PMSMPtr = (SM_Modules*)PMObj.pData;

	// set flags at start of program
	PMSMPtr->PMSM.Shutdown.Flags.ProcessManagement = 0;
	PMSMPtr->PMSM.Heartbeat.Status = 0x00; 
	PMSMPtr->PMSM.Shutdown.Status = 0x00;
	

	//PM specific tasks here
	// e.g. SMHBPtr->PMTimeStamp = (double)Stopwatch::GetTimestamp();

	//start all 5 modules
	StartProcesses();

	while (!_kbhit()) { //while no keyboard hit
		// detect laser heartbeat
		if (PMSMPtr->PMSM.Heartbeat.Flags.Laser == 1) {
			PMSMPtr->PMSM.Heartbeat.Flags.Laser = 0;
		}
		else {
			//wait time limit required
			PMSMPtr->PMSM.Shutdown.Status = 0xFF; // shutdown; critical process
		}

		// detect GPS heartbeat
		if (PMSMPtr->PMSM.Heartbeat.Flags.GPS == 1) {
			PMSMPtr->PMSM.Heartbeat.Flags.GPS = 0;
		}
		else {
			PMSMPtr->PMSM.Shutdown.Flags.Display = 1;
			PMSMPtr->PMSM.Shutdown.Status = 0xFF;
			StartProcesses(); // restart non-critical process
		}

		// detect camera heartbeat
		if (PMSMPtr->PMSM.Heartbeat.Flags.Camera == 1) {
			PMSMPtr->PMSM.Heartbeat.Flags.Camera = 0;
		}
		else {
			PMSMPtr->PMSM.Shutdown.Status = 0xFF; // shutdown; critical process
		}

		// detect vehicle heartbeat
		if (PMSMPtr->PMSM.Heartbeat.Flags.VehicleControl == 1) {
			PMSMPtr->PMSM.Heartbeat.Flags.VehicleControl = 0;
		}
		else {
			PMSMPtr->PMSM.Shutdown.Status = 0xFF; // shutdown; critical process
		}

		// detect display heartbeat
		if (PMSMPtr->PMSM.Heartbeat.Flags.Display == 1) {
			PMSMPtr->PMSM.Heartbeat.Flags.Display = 0;
		}
		else {
			PMSMPtr->PMSM.Shutdown.Status = 0xFF; // shutdown; critical process
		}

		// exit loop if PM shutdown
		//if (PMSMPtr->PMSM.Shutdown.Flags.ProcessManagement == 1) {
		//	break;
		//}

	}

	// shutdown PM after exiting while loop
	PMSMPtr->PMSM.Shutdown.Status = 0xFF;

	Console::WriteLine("Process management terminated normally.");
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

