#pragma once

#include <NetworkedModule.h>

ref class VC : public NetworkedModule {
public:

	VC();

	VC(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps);

	error_state processSharedMemory() override;

	bool getShutdownFlag() override;

	void threadFunction() override;

	error_state processHeartBeats();

	virtual error_state connect(String^ hostName, int portNumber) override;
	virtual error_state communicate() override;

	~VC();

private:
	// add additional data members or helper functions here
	SM_ThreadManagement^ SM_TM_;
	SM_Laser^ SM_Laser_;
	SM_GPS^ SM_Gps_;
};