#include "Controller.h"
#include "TMM.h"
#include "Laser.h"
#include "GNSS.h"
#include "Display.h"
#include "CrashAvoidance.h"
#include "VC.h"

error_state ThreadManagement::setupSharedMemory() {
	SM_TM_ = gcnew SM_ThreadManagement;
	SM_Laser_ = gcnew SM_Laser;
	SM_Gps_ = gcnew SM_GPS;
	SM_VC_ = gcnew SM_VehicleControl;
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
	// assuming that crash avoidance thread is non critical
	ThreadPropertiesList = gcnew array<ThreadProperties^>{
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Laser(SM_TM_, SM_Laser_, SM_Gps_), &Laser::threadFunction), true, bit_LASER, "Laser Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(SM_TM_, SM_Laser_, SM_Gps_), &GNSS::threadFunction), false, bit_GPS, "GNSS Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew Controller(SM_TM_, SM_Laser_, SM_Gps_, SM_VC_), &Controller::threadFunction), true, bit_CONTROLLER, "Controller Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew VC(SM_TM_, SM_Laser_, SM_Gps_, SM_VC_), &VC::threadFunction), true, bit_VC, "VC Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew Display(SM_TM_, SM_Laser_, SM_Gps_), &Display::threadFunction), true, bit_DISPLAY, "Display Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew CrashAvoidance(SM_TM_, SM_Laser_, SM_Gps_), &CrashAvoidance::threadFunction), false, bit_CRASHAVOIDANCE, "Crash Avoidance Thread")
	};
	// list of threads
	ThreadList = gcnew array<Thread^>(ThreadPropertiesList->Length);

	// list of stopwatches
	StopwatchList = gcnew array<Stopwatch^>(ThreadPropertiesList->Length);

	// make thread barrier, number of threads = all threads in list + TMM thread
	SM_TM_->ThreadBarrier = gcnew Barrier(ThreadPropertiesList->Length + 1);

	for (int i = 0; i < ThreadPropertiesList->Length; i++) {
		// initialise all stopwatches
		StopwatchList[i] = gcnew Stopwatch;
		// initialise all the threads before starting
		ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
		// start all threads
		ThreadList[i]->Start();
	}

	// wait at TMT thread barrier
	SM_TM_->ThreadBarrier->SignalAndWait();

	// start all stopwatches
	for (int i = 0; i < ThreadList->Length; i++) {
		StopwatchList[i]->Start();
	}
	// start thread loop
	// !ConsoleKeyAvailable means that it will stop running once a key has been pressed
	// maybe insert routine shutdown here
	while ( /* !Console::KeyAvaialble && */ !getShutdownFlag()) {
		if (Console::KeyAvailable) {
			auto key = Console::ReadKey();
			if (key.KeyChar == 'q') {
				break;
			}
		}
		// Console::WriteLine("TMT Thread is running");
		processHeartBeats();
		Thread::Sleep(50);
	}

	// shutdown threads
	shutdownModules();
	// join all threads
	for (int i = 0; i < ThreadPropertiesList->Length; i++) {
		ThreadList[i]->Join();
	}
	Console::WriteLine("TMT Thread is terminating.");
}

error_state ThreadManagement::processHeartBeats() {
	// check heart beat flag of thread
		// if a response is received -> flag should be high:
			// put flag down and reset stopwatch	
	// else
		// check if stopwatch has exceeded time limit, if it hasn't then do nothing, if it has then:
			// if the process is critical, shutdown all threads, if not then restart the thread

	for (int i = 0; i < ThreadList->Length; i++) {
		// check heart beat flag of thread, if a response is received it should be high
		if (SM_TM_->heartbeat & ThreadPropertiesList[i]->BitID) {
			// if bit is high, put it down and reset stopwatch
			SM_TM_->heartbeat ^= ThreadPropertiesList[i]->BitID;
			StopwatchList[i]->Restart();
		}
		else {
			// if bit is down, check if stopwatch has exceeded time limit
			// else, don't do anything
			if (StopwatchList[i]->ElapsedMilliseconds > CRASH_LIMIT) {
				// stopwatch exceeded time limit
				if (ThreadPropertiesList[i]->Critical) {
					// critical thread, shutdown all threads
					Console::WriteLine(ThreadPropertiesList[i]->ThreadName + "failure. Shutting down all threads.");
					shutdownModules();
					return error_state::ERR_CRITICAL_PROCESS_FAILURE;
				}
				else {
					Console::WriteLine(ThreadPropertiesList[i]->ThreadName + "failure. Attempting to restart thread.");
					// not critical thread, try to restart thread
					// abort first
					ThreadList[i]->Abort();
					// new thread object
					ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);

					// only need to add 1 thread to thread barrier
					SM_TM_->ThreadBarrier = gcnew Barrier(1);

					// start the thread
					ThreadList[i]->Start();
				}
			}
		}
	}

	return error_state::SUCCESS;
}

ThreadManagement::ThreadManagement() {}

ThreadManagement::~ThreadManagement() {}