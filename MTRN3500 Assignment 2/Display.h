#pragma once

#include <NetworkedModule.h>

ref class Display : public NetworkedModule {
public:

	Display();

	Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps);

	error_state processSharedMemory() override;

	void shutdownModules();

	bool getShutdownFlag() override;

	void threadFunction() override;

	virtual error_state connect(String^ hostName, int portNumber) override;
	virtual error_state communicate() override;

private: 
	// add additional data members or helper functions here
	SM_ThreadManagement^ SM_TM_;
	SM_Laser^ SM_Laser_;
	SM_GPS^ SM_Gps_;
};