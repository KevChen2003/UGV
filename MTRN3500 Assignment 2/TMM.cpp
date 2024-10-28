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
	// assuming that crash avoidance thread is critical
	ThreadPropertiesList = gcnew array<ThreadProperties^>{
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Laser(), &Laser::threadFunction), true, bit_LASER, "Laser Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(), &GNSS::threadFunction), false, bit_GPS, "GNSS Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew Controller(), &Controller::threadFunction), true, bit_CONTROLLER, "Controller Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew VC(), &VC::threadFunction), true, bit_VC, "VC Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew Display(), &Display::threadFunction), true, bit_DISPLAY, "Display Thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew CrashAvoidance(), &CrashAvoidance::threadFunction), true, bit_CRASHAVOIDANCE, "Crash Avoidance Thread")
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
	while (!Console::KeyAvailable && getShutdownFlag()) {
		Console::WriteLine("TMT Thread is running");
		processHeartBeats();
		Thread::Sleep(50);
	}

	// shutdown threads
	shutdownModules();
	// join all threads
	for (int i = 0; i < ThreadPropertiesList->Length; i++) {
		ThreadList[i]->Join();
	}
}

error_state ThreadManagement::processHeartBeats() {
	return error_state::SUCCESS;
}

ThreadManagement::ThreadManagement() {}

ThreadManagement::~ThreadManagement() {}