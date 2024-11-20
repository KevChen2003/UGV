#include "CrashAvoidance.h"
#include <cmath>

CrashAvoidance::CrashAvoidance() {}

CrashAvoidance::CrashAvoidance(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps, SM_CrashAvoidance^ SM_CA) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_Gps_ = SM_Gps;
	SM_CA_ = SM_CA;
}

CrashAvoidance::~CrashAvoidance() {}
error_state CrashAvoidance::setupSharedMemory() {
	return error_state::SUCCESS;
}

error_state CrashAvoidance::processSharedMemory() {
	// grab x and y values from SM laser
	// Enter the monitor to ensure thread-safe access
	Monitor::Enter(SM_Laser_->lockObject);
	try {
		// grab the x and y values
		RangeX = SM_Laser_->x;
		RangeY = SM_Laser_->y;
	}
	finally {
		// Exit the monitor
		Monitor::Exit(SM_Laser_->lockObject);
	}
	return error_state::SUCCESS;
}

error_state CrashAvoidance::SetFlags(bool CanGoForwards, bool CanSteerLeft, bool CanSteerRight) {
	// set movement flags
	// Enter the monitor to ensure thread-safe access
	Monitor::Enter(SM_CA_->lockObject);
	try {
		// set flags
		SM_CA_->CanGoForwards = CanGoForwards;
		SM_CA_->CanSteerLeft = CanSteerLeft;
		SM_CA_->CanSteerRight = CanSteerRight;
	}
	finally {
		// Exit the monitor
		Monitor::Exit(SM_CA_->lockObject);
	}
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

	String^ Message = "";
	String^ LastMessage = "";
	while (!getShutdownFlag()) {
		// Console::WriteLine("CrashAvoidance Thread is running.");
		processHeartBeats();
		// CrashAvoidance functionality 

		// from edstem forum: UGV has length 1310mm and width 560mm
		// if laser detects y value between -280 and +280 (within vehicle width), and the distance of the point < 1m. prevent vehicle from going forwards
		// if the laser detects outside, but still within 1m and y > 280, then prevent vehicle from steering left
		// if laser detects outside, but still within 1m and y < -280, then prevent vehicle from steering right
		
		// for this, you can change values in SM_VC, but what happens if VC gets access to it before this thread can,
		// then the check wouldn't do anything
		
		processSharedMemory();

		// set movement flags every iteration, true if no objects within crash distance
		bool CanGoForwards = true;
		bool CanSteerLeft = true;
		bool CanSteerRight = true;
		
		// THIS IS ALL ASSUMING LASER IS IN MM
		// THE LASER SIMULATED READINGS ARE ALL WITHIN 1000, SO IF YOU USE MM THEN ALL READINGS WILL SHOW
		// OBSTACLES 
		Message = "No Obstacles in Sight";
		for (int i = 0; i < RangeX->Length; i++) {
			// loop through values and update status

			if (RangeX[i] == 0 && RangeY[i] == 0) continue; // skip through (0,0) points, as they're points outside of laser reach
			
			// sometimes comment these out on sim to allow the robot to actually move, otherwise it will think that there's obstacles and won't move
			
			// left and right might be flipped, negative Y might be left and positive Y might be right on the real robot
			/*
			// using mm but inaccurate on actual robot
			if (CheckDistance(RangeX[i], RangeY[i]) && RangeY[i] >= -280 && RangeY[i] <= 280) {
				// object in front and within 1m, prevent from moving forwards
				//Console::WriteLine("Inhibiting Forwards.");
				CanGoForwards = false;
			}
			else if (CheckDistance(RangeX[i], RangeY[i]) && RangeY[i] > 280) {
				// object within 1m on vehicle's left , prevent it from steering left
				//Console::WriteLine("Inhibiting Left.");
				CanSteerLeft = false;
			}
			else if (CheckDistance(RangeX[i], RangeY[i]) && RangeY[i] < -280) {
				// obkect within 1m on vehicle's right, prevent steering right
				//Console::WriteLine("Inhibiting Right.");
				CanSteerRight = false;
			}
			*/
			// REAL ONE
			
			// using 0.5 degree resolution
			if (CheckDistance(RangeX[i], RangeY[i]) && i >= 90 && i <= 270) {
				// object in front (between 45 and 135 degrees) and within 1m, prevent from moving forwards
				//Console::WriteLine("Inhibiting Forwards.");
				Message = "Obstacle Detected in Front.";
				CanGoForwards = false;
			}
			else if (CheckDistance(RangeX[i], RangeY[i]) && i > 270) {
				// object within 1m on vehicle's left (between 135 and 180 degrees), prevent it from steering left
				//Console::WriteLine("Inhibiting Left.");
				Message = "Obstacle Detected on the Left.";
				CanSteerLeft = false;
			}
			else if (CheckDistance(RangeX[i], RangeY[i]) && i < 90) {
				// obkect within 1m on vehicle's right (between 0 and 45 degrees), prevent steering right
				//Console::WriteLine("Inhibiting Right.");
				Message = "Obstacle Detected on the Right.";
				CanSteerRight = false;
			}
			
			if (Message != LastMessage) Console::WriteLine(Message); // prevents printing of duplicate error messages in a row
			LastMessage = Message;
		}
		

		SetFlags(CanGoForwards, CanSteerLeft, CanSteerRight);

		Thread::Sleep(10);
	}
	Console::WriteLine("CrashAvoidance Thread is terminating.");
}

bool CrashAvoidance::CheckDistance(double x, double y) {
	// check distance of point (x, y) from (0,0), return TRUE if distance < 1m, FALSE otherwise
	// DISTANCE CAN BE M OR MM, SO SET TO < 1M OR <1000MM DEPENDING ON WHAT UNITS YOU WANT TO USE

	double dist = std::sqrt(std::pow(x - 0, 2) + std::pow(y - 0, 2));

	// if (dist < 1000.0) Console::WriteLine("Calculated dist: {0}", dist);

	return dist < 1000.0;
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
			// printing error here before modules are shut down
			printError(error_state::ERR_TMM_FAILURE);
			shutdownModules();
			return error_state::ERR_TMM_FAILURE;
		}
	}
	return error_state::SUCCESS;
}