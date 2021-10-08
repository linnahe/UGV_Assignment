#using <System.dll>
#include <Windows.h>
#include <tchar.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <iostream>
#include <conio.h>

#include "SMStructs.h"
#include "SMObject.h"

using namespace System;
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
	TEXT("VehicleControl.exe")
};

int main()
{
	/*
	// instantiate SMObject
	SMObject PMObj(_TEXT("PMObj"), sizeof(SM_Heartbeats));

	// create shared memory
	PMObj.SMCreate(); // check SMCreateError flag for error trapping
	PMObj.SMAccess(); //check SMAccessError flag for error trapping

	// ptr to SM struct
	SM_Heartbeats* SMHBPtr = (SM_Heartbeats*)PMObj.pData; */

	//start all 5 modules
	StartProcesses();
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

