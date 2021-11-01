#pragma once

#include <UGV_module.h>
#include <smstructs.h>

unsigned long CRC32Value(int i);
unsigned long CalculateBlockCRC32(unsigned long ulCount, unsigned char* ucBuffer);


ref class Vehicle : public UGV_module // class inherits from UGV_module
{
public: //prefer to have function declarations in this file, then definitions in a .cpp file
	int connect(String^ hostName, int portNumber) override;
	int setupSharedMemory() override;
	int getData() override;
	int checkData() override;
	int sendDataToSharedMemory() override;
	bool getShutdownFlag() override;
	int setHeartbeat(bool heartbeat) override;
	~Vehicle();

protected:
	// YOUR CODE HERE (ADDITIONAL MEMBER VARIABLES THAT YOU MAY WANT TO ADD)
	array<unsigned char>^ SendData;		// declaration
	String^ StudID;
};