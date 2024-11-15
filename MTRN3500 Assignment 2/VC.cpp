#include <msclr/marshal_cppstd.h>
#include "VC.h"
#include <NetworkedModule.h>

VC::VC() { };

VC::VC(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps, SM_VehicleControl^ SM_VC) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser; 
	SM_Gps_ = SM_Gps;
	SM_VC_ = SM_VC;
}

error_state VC::processSharedMemory() {
	// Enter the monitor to ensure thread-safe access
	Monitor::Enter(SM_VC_->lockObject);
	try {
		// Read the speed and steering values
		speed = SM_VC_->Speed;
		steer = SM_VC_->Steering;
	}
	finally {
		// Exit the monitor
		Monitor::Exit(SM_VC_->lockObject);
	}
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
	// set watdog as 0
	wdog = 0;
	if (!getShutdownFlag() && connect(WEEDER_ADDRESS, 25000) == error_state::SUCCESS) {
		Console::WriteLine("Connected to VC Successfully.");
		if (sendCommand("5360742\n") == error_state::SUCCESS) {
			Console::WriteLine("Authentication Successful.");
		}
		while (!getShutdownFlag()) {
			// Console::WriteLine("VC Thread is running.");
			processHeartBeats();
			// VC functionality 
			processSharedMemory();
			double steerVal = steer * 40.0;
			String^ command = String::Format("# {0} {1} {2} #", steerVal, speed, wdog);
			// keep wdog alternating
			wdog = ~wdog;
			/*
			if (communicate() == error_state::SUCCESS) {
				// if communication is successful and data is successful, put the data in Display shared memory
				processSharedMemory();
			}
			*/
			Thread::Sleep(20);
		}
	}
	Console::WriteLine("VC Thread is terminating.");
}

error_state VC::sendCommand(String^ command) {
	if (Client == nullptr || Stream == nullptr) {
		return error_state::ERR_CONNECTION;
	}
	try {
		SendData = System::Text::Encoding::ASCII->GetBytes(command);
		// Stream->WriteByte(0x02);
		Stream->Write(SendData, 0, SendData->Length);
		// Stream->WriteByte(0x03);
		Threading::Thread::Sleep(10);
		Stream->Read(ReadData, 0, ReadData->Length);
		// String^ Response = System::Text::Encoding::ASCII->GetString(ReadData);
		/*
		// honestly probably not needed, seems like the exception is already caught
		for (int i = 0; i < Response->Length; i++) {
			if (Response[i] == '?') {
				throw gcnew Exception("Unknown Command.");
			}
		}
		*/
		return error_state::SUCCESS;
	}
	catch (Exception^ e) {
		std::string ErrorMessage = msclr::interop::marshal_as<std::string>(e->Message);
		std::cerr << "Exception found when sending command: " << ErrorMessage << '\n';
		return error_state::ERR_RESPONSE;
	}
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
