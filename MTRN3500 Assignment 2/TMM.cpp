#include "TMM.h"
#include "Laser.h"
#include "GNSS.h"
#include "Controller.h"
#include "Display.h"
#include "CrashAvoidance.h"
#include "VC.h"

error_state ThreadManagement::setupSharedMemory() {
	SM_TM_ = gcnew SM_ThreadManagement;
	SM_Laser_ = gcnew SM_Laser;
	SM_Gps_ = gcnew SM_GPS;
	return error_state::SUCCESS;
}

error_state ThreadManagement::processSharedMemory() {
	return error_state::SUCCESS;
}

void ThreadManagement::shutdownModules() {
	// shutdown all threads
	SM_TM_->shutdown = bit_ALL;
}

bool ThreadManagement::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_TM);
}

void ThreadManagement::threadFunction() {
	// make a list of thread properties
	ThreadPropertiesList = gcnew array<ThreadProperties^>{
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Laser(), &Laser::threadFunction), true, bit_LASER, "Laser thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(), &GNSS::threadFunction), false, bit_GPS, "GNSS Thread")
	};
}

error_state ThreadManagement::processHeartBeats() {
	return error_state::SUCCESS;
}

ThreadManagement::ThreadManagement() {}

ThreadManagement::~ThreadManagement() {}