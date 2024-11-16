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
	try {
		// should already connect to it
		Client = gcnew TcpClient(hostName, portNumber);
		Stream = Client->GetStream();
		Client->NoDelay = true;
		Client->ReceiveTimeout = 5000;
		Client->SendTimeout = 5000;
		Client->ReceiveBufferSize = 2048;
		Client->SendBufferSize = 1024;

		ReadData = gcnew array<unsigned char>(2048);
		SendData = gcnew array<unsigned char>(1024);

		SendData = System::Text::Encoding::ASCII->GetBytes("5360742\n");
		Stream->Write(SendData, 0, SendData->Length);
		Threading::Thread::Sleep(10);
		return error_state::SUCCESS;
	}
	catch (Exception^ e) {
		// Handle exceptions
		std::string ErrorMessage = msclr::interop::marshal_as<std::string>(e->Message);
		std::cerr << "Exception when connecting: " << ErrorMessage << '\n';
		return error_state::ERR_CONNECTION; // Return connection error state
	}
}
error_state VC::communicate() {
	if (Client == nullptr || Stream == nullptr) {
		Console::WriteLine("Connection Error.");
		return error_state::ERR_CONNECTION;
	}
	try {
		double steerVal = steer * 40.0;
		String^ command = String::Format("# {0} {1} {2} #", steerVal, speed, wdog);
		// keep wdog alternating
		wdog = ~wdog;
		SendData = System::Text::Encoding::ASCII->GetBytes(command);
		Stream->Write(SendData, 0, SendData->Length);
		return error_state::SUCCESS;
	}
	catch (Exception^ e) {
		std::string ErrorMessage = msclr::interop::marshal_as<std::string>(e->Message);
		std::cerr << "Exception found when sending command: " << ErrorMessage << '\n';
		return error_state::ERR_RESPONSE;
	}
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
		Console::WriteLine("Connected to VC Successfully + Authenticated.");
		while (!getShutdownFlag()) {
			// Console::WriteLine("VC Thread is running.");
			processHeartBeats();
			// VC functionality 
			processSharedMemory();
			communicate();
			Thread::Sleep(10);
		}
	}
	Console::WriteLine("VC Thread is terminating.");
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
