#include "VC.h"
#include <NetworkedModule.h>

VC::VC() { };

VC::VC(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
}

error_state VC::processSharedMemory() {
	return error_state::SUCCESS;
}

bool VC::getShutdownFlag() {
	return true;
}

void VC::threadFunction() {

}

error_state VC::connect(String^ hostName, int portNumber) {
	return error_state::SUCCESS;
}
error_state VC::communicate() {
	return error_state::SUCCESS;
}

VC::~VC() {};

error_state VC::processHeartBeats() {
	return error_state::SUCCESS;
}