#include "Controller.h"

Controller::Controller() {}

Controller::Controller(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps, SM_VehicleControl^ SM_VC) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_Gps_ = SM_Gps;
	SM_VC_ = SM_VC;
}

Controller::~Controller() {
	// clean up native ptr
	delete ControllerInterface_;
}

error_state Controller::setupSharedMemory() {
	return error_state::SUCCESS;
}

error_state Controller::processSharedMemory() {
	// use the lock for reading and writing ONLY
	Monitor::Enter(SM_VC_->lockObject);
	try {
		SM_VC_->Speed = speed;
		SM_VC_->Steering = steer;
	}
	finally {
		Monitor::Exit(SM_VC_->lockObject);
	}
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
	bool keyboard = true;
	ControllerInterface_ = new ControllerInterface(1, keyboard);

	// wait at the barrier for other threads
	SM_TM_->ThreadBarrier->SignalAndWait();
	// start stopwatch
	Watch->Start();
	while (!getShutdownFlag()) {
		// Console::WriteLine("Controller Thread is running.");
		processHeartBeats();
		// Controller functionality 

		// remember to add a xbox button to indicate shutdown
		if (ControllerInterface_->IsConnected()) {
			controllerState state = ControllerInterface_->GetState();
			// ControllerInterface_->printControllerState(state);
			// speed = right trigger - left trigger
			// steer = right thumb x
			speed = state.rightTrigger - state.leftTrigger;
			steer = state.rightThumbX;
			// apparently keyboard steering is flipped
			if (keyboard) steer = -1 * steer;
			// Console::WriteLine("Speed: {0}, Steer: {1}", speed, steer);

		}
		else {
			speed = 0;
			steer = 0;
		}
		// next, send the values over to VC, who should send it to control the robot's movements
		processSharedMemory();
		Thread::Sleep(10);
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