#include "Display.h"
#include <NetworkedModule.h>

Display::Display() { };

Display::Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_Gps_ = SM_Gps;
}

error_state Display::processSharedMemory() {
	return error_state::SUCCESS;
}

bool Display::getShutdownFlag() {
	return true;
}

error_state Display::connect(String^ hostName, int portNumber) {
	return error_state::SUCCESS;
}
error_state Display::communicate() {
	return error_state::SUCCESS;
}

Display::~Display() {};

void Display::shutdownModules() {
	SM_TM_->shutdown = bit_ALL;
}

void Display::threadFunction() {
	Console::WriteLine("Display Thread is starting.");
	// initialise stopwatch
	Watch = gcnew Stopwatch;
	// wait at the barrier for other threads
	SM_TM_->ThreadBarrier->SignalAndWait();
	// start stopwatch
	Watch->Start();
	while (!getShutdownFlag()) {
		Console::WriteLine("Display Thread is running.");
		processHeartBeats();
		// Display functionality 
		/*
		if (communicate() == error_state::SUCCESS) {
			// if communication is successful and data is successful, put the data in Display shared memory
			processSharedMemory();
		}
		*/
		Thread::Sleep(20);
	}
	Console::WriteLine("Display Thread is terminating.");
}

error_state Display::processHeartBeats() {
	if ((SM_TM_->heartbeat & bit_DISPLAY) == 0) {
		// if the Display bit in the heartbeat byte is down, put the bit back up
		SM_TM_->heartbeat |= bit_DISPLAY;
		// reset stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			// if the Display bit is up and the watch has exceeded the limit
			// shutdown all threads
			shutdownModules();
			return error_state::ERR_TMM_FAILURE;
		}
	}
	return error_state::SUCCESS;
}
