#include "CrashAvoidance.h"

CrashAvoidance::CrashAvoidance() {}

CrashAvoidance::CrashAvoidance(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_Gps_ = SM_Gps;
}

CrashAvoidance::~CrashAvoidance() {}
error_state CrashAvoidance::setupSharedMemory() {
	return error_state::SUCCESS;
}

error_state CrashAvoidance::processSharedMemory() {
	return error_state::SUCCESS;
}


bool CrashAvoidance::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_CRASHAVOIDANCE);
}

void CrashAvoidance::shutdownModules() {
	SM_TM_->shutdown = bit_ALL;
}

void CrashAvoidance::threadFunction() {
	Console::WriteLine("CrashAvoidance Thread is starting.");
	// initialise stopwatch
	Watch = gcnew Stopwatch;
	// wait at the barrier for other threads
	SM_TM_->ThreadBarrier->SignalAndWait();
	// start stopwatch
	Watch->Start();
	while (!getShutdownFlag()) {
		Console::WriteLine("CrashAvoidance Thread is running.");
		processHeartBeats();
		// CrashAvoidance functionality 
		Thread::Sleep(20);
	}
	Console::WriteLine("CrashAvoidance Thread is terminating.");
}

error_state CrashAvoidance::processHeartBeats() {
	if ((SM_TM_->heartbeat & bit_CRASHAVOIDANCE) == 0) {
		// if the CrashAvoidance bit in the heartbeat byte is down, put the bit back up
		SM_TM_->heartbeat |= bit_CRASHAVOIDANCE;
		// reset stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			// if the CrashAvoidance bit is up and the watch has exceeded the limit
			// shutdown all threads
			shutdownModules();
			return error_state::ERR_TMM_FAILURE;
		}
	}
	return error_state::SUCCESS;
}