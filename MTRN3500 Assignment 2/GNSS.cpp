#include "GNSS.h"

GNSS::GNSS() {}

GNSS::GNSS(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Gps_ = SM_Gps;
	SM_Laser_ = SM_Laser;
}

error_state GNSS::setupSharedMemory() {
	return error_state::SUCCESS;
}

bool GNSS::getShutdownFlag() {
	return true;
}

void GNSS::threadFunction() {
	Console::WriteLine("GNSS Thread is starting.");
	// initialise stopwatch
	Watch = gcnew Stopwatch;
	// wait at the barrier for other threads
	SM_TM_->ThreadBarrier->SignalAndWait();
	// start stopwatch
	Watch->Start();
	while (!getShutdownFlag()) {
		Console::WriteLine("GNSS Thread is running.");
		processHeartBeats();
		// GNSS functionality 
		if (communicate() == error_state::SUCCESS && checkData() == error_state::SUCCESS) {
			// if communication is successful and data is successful, put the data in laser shared memory
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("GNSS Thread is terminating.");
}

error_state GNSS::processHeartBeats() {
	if ((SM_TM_->heartbeat & bit_GPS) == 0) {
		// if the GNSS bit in the heartbeat byte is down, put the bit back up
		SM_TM_->heartbeat |= bit_GPS;
		// reset stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			// if the GNSS bit is up and the watch has exceeded the limit
			// shutdown all threads
			shutdownModules();
			return error_state::ERR_TMM_FAILURE;
		}
	}
	return error_state::SUCCESS;
}


error_state GNSS::checkData() {
	return error_state::SUCCESS;
}

error_state GNSS::processSharedMemory() {
	return error_state::SUCCESS;
}

GNSS::~GNSS() {}

error_state GNSS::connect(String^ hostName, int portNumber) {
	return error_state::SUCCESS;
}
error_state GNSS::communicate() {
	return error_state::SUCCESS;
}

void GNSS::shutdownModules() {
	SM_TM_->shutdown = bit_ALL;
}
