#pragma once

#include <UGV_module.h>
#include <smstructs.h>


ref class Laser : public UGV_module //Laser class inherits from UGV_module
{

public: //prefer to have function declarations in this file, then definitions in a .cpp file
	int connect(String^ hostName, int portNumber) override;
	int setupSharedMemory() override;
	int getData() override;
	int checkData() override;
	int sendDataToSharedMemory() override;
	bool getShutdownFlag() override;
	int setHeartbeat(bool heartbeat) override;
	~Laser();


protected:
	// YOUR CODE HERE (ADDITIONAL MEMBER VARIABLES THAT YOU MAY WANT TO ADD)
	array<String^>^ SplitResponseData;
	array<String^>^ StringArray;
	String^ ResponseData;
	// arrays of unsigned chars to send and receive data
	array<unsigned char>^ SendData; //unsigned char is a byte. SendData is a handle to the entire array
	array<unsigned char>^ ReadData;
	double StartAngle;	//StringArray[23]
	double Resolution;	//StringArray[24]
	int NumRanges;		//StringArray[25]
};