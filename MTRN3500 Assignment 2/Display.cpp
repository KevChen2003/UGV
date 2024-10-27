#include "Display.h"
#include <NetworkedModule.h>

Display::Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_Gps_ = SM_Gps;
}

error_state Display::processSharedMemory() {
	return error_state::SUCCESS;
}

void Display::shutdownModules() {

}

bool Display::getShutdownFlag() {
	return true;
}

void Display::threadFunction() {

}

error_state Display::connect(String^ hostName, int portNumber) {
	return error_state::SUCCESS;
}
error_state Display::communicate() {
	return error_state::SUCCESS;
}