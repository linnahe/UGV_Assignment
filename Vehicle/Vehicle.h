#pragma once

#include <UGV_module.h>
#include <smstructs.h>

#using <System.dll>
using namespace System;
using namespace System::Net::Sockets;

ref class Vehicle : public UGV_module // class inherits from UGV_module
{

public:
	int connect(String^ hostName, int portNumber) override;
	int setupSharedMemory() override;
	void moveVehicle(double steer, double speed, int flag) override;
	~Vehicle();

};