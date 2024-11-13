#include "Controller.h"

Controller::Controller() {}

Controller::Controller(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_Gps_ = SM_Gps;
}

Controller::~Controller() {
	// clean up native ptr
	delete ControllerInterface_;
}

error_state Controller::setupSharedMemory() {
	return error_state::SUCCESS;
}

error_state Controller::processSharedMemory() {
	return error_state::SUCCESS;
}

bool Controller::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_CONTROLLER);
}

void Controller::shutdownModules() {
	SM_TM_->shutdown = bit_ALL;
}

void Controller::threadFunction() {
	Console::WriteLine("Controller Thread is starting.");
	// initialise stopwatch
	Watch = gcnew Stopwatch;
	// initialise controller interface
	// 1st param = xbox player num
	// 2nd param = 0 if xbox control, 1 if keyboard
	ControllerInterface_ = new ControllerInterface(1, 0);

	// wait at the barrier for other threads
	SM_TM_->ThreadBarrier->SignalAndWait();
	// start stopwatch
	Watch->Start();
	while (!getShutdownFlag()) {
		// Console::WriteLine("Controller Thread is running.");
		processHeartBeats();
		// Controller functionality 
		if (ControllerInterface_->IsConnected()) {
			ControllerInterface_->printControllerState(ControllerInterface_->GetState());
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("Controller Thread is terminating.");
}

error_state Controller::processHeartBeats() {
	if ((SM_TM_->heartbeat & bit_CONTROLLER) == 0) {
		// if the Controller bit in the heartbeat byte is down, put the bit back up
		SM_TM_->heartbeat |= bit_CONTROLLER;
		// reset stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			// if the Controller bit is up and the watch has exceeded the limit
			// shutdown all threads
			shutdownModules();
			return error_state::ERR_TMM_FAILURE;
		}
	}
	return error_state::SUCCESS;
}