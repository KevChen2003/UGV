#include "VC.h"
#include <NetworkedModule.h>

VC::VC() { };

VC::VC(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser; 
	SM_Gps_ = SM_Gps;
}

error_state VC::processSharedMemory() {
	return error_state::SUCCESS;
}

bool VC::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_VC);
}


error_state VC::connect(String^ hostName, int portNumber) {
	return error_state::SUCCESS;
}
error_state VC::communicate() {
	return error_state::SUCCESS;
}

VC::~VC() {};

void VC::shutdownModules() {
	SM_TM_->shutdown = bit_ALL;
}

void VC::threadFunction() {
	Console::WriteLine("VC Thread is starting.");
	// initialise stopwatch
	Watch = gcnew Stopwatch;
	// wait at the barrier for other threads
	SM_TM_->ThreadBarrier->SignalAndWait();
	// start stopwatch
	Watch->Start();
	while (!getShutdownFlag()) {
		// Console::WriteLine("VC Thread is running.");
		processHeartBeats();
		// VC functionality 
		/*
		if (communicate() == error_state::SUCCESS) {
			// if communication is successful and data is successful, put the data in Display shared memory
			processSharedMemory();
		}
		*/
		Thread::Sleep(20);
	}
	Console::WriteLine("VC Thread is terminating.");
}

error_state VC::processHeartBeats() {
	if ((SM_TM_->heartbeat & bit_VC) == 0) {
		// if the VC bit in the heartbeat byte is down, put the bit back up
		SM_TM_->heartbeat |= bit_VC;
		// reset stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			// if the VC bit is up and the watch has exceeded the limit
			// shutdown all threads
			shutdownModules();
			return error_state::ERR_TMM_FAILURE;
		}
	}
	return error_state::SUCCESS;
}
