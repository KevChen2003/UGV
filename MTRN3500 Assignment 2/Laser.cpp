#include "Laser.h"

Laser::Laser() {}

Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	// not sure if stopwatch required below as its intiialised in the thread function again later
	//Watch = gcnew Stopwatch;
}

error_state Laser::setupSharedMemory() {
	return error_state::SUCCESS;
}

bool Laser::getShutdownFlag() {
	return true;
}

error_state Laser::communicate() {
	return error_state::SUCCESS;
}

void Laser::threadFunction() {
	Console::WriteLine("Laser Thread is starting.");
	// initialise stopwatch
	Watch = gcnew Stopwatch;
	// wait at the barrier for other threads
	SM_TM_->ThreadBarrier->SignalAndWait();
	// start stopwatch
	Watch->Start();
	while (!getShutdownFlag()) {
		Console::WriteLine("Laser Thread is running.");
		processHeartBeats();
		// laser functionality 
		if (communicate() == error_state::SUCCESS && checkData() == error_state::SUCCESS) {
			// if communication is successful and data is successful, put the data in laser shared memory
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("Laser Thread is terminating.");
}

error_state Laser::processHeartBeats() {	
	if ((SM_TM_->heartbeat & bit_LASER) == 0) {
		// if the laser bit in the heartbeat byte is down, put the bit back up
		SM_TM_->heartbeat |= bit_LASER;
		// reset stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			// if the laser bit is up and the watch has exceeded the limit
			// shutdown all threads
			shutdownModules();
			return error_state::ERR_TMM_FAILURE;
		}
	}
	return error_state::SUCCESS;
}

error_state Laser::checkData() {
	return error_state::SUCCESS;
}

// Send/Recieve data from shared memory structures
error_state Laser::processSharedMemory() {
	return error_state::SUCCESS;
}

error_state Laser::connect(String^ hostName, int portNumber) {
	return error_state::SUCCESS;
}
error_state Laser::communicate() {
	return error_state::SUCCESS;
}

void Laser::shutdownModules() {
	// surely can find a way to make shutdownModules transferrable to all modules, but that probably means
	// having a UGVModule.cpp which i dont have so will keep repeating for now
	SM_TM_->shutdown = bit_ALL;
}

Laser::~Laser() {}