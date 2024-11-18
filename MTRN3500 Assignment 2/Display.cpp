#include <msclr/marshal_cppstd.h>
#include "Display.h"
#include <NetworkedModule.h>

Display::Display() { };

Display::Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_Gps_ = SM_Gps;
}

error_state Display::processSharedMemory() {
	// Enter the monitor to ensure thread-safe access
	Monitor::Enter(SM_Laser_->lockObject);
	try {
		// set the x and y values
		RangeX = SM_Laser_->x;
		RangeY = SM_Laser_->y;
	}
	finally {
		// Exit the monitor
		Monitor::Exit(SM_Laser_->lockObject);
	}
	return error_state::SUCCESS;
}

bool Display::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_DISPLAY);
}

error_state Display::connect(String^ hostName, int portNumber) {
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

		return error_state::SUCCESS;
	}
	catch (Exception^ e) {
		// Handle exceptions
		std::string ErrorMessage = msclr::interop::marshal_as<std::string>(e->Message);
		std::cerr << "Exception when connecting: " << ErrorMessage << '\n';
		return error_state::ERR_CONNECTION; // Return connection error state
	}
}
error_state Display::communicate() {
	// adapted from appendix B
	// serialise data array to byte array
	//Console::WriteLine(RangeX);
	//Console::WriteLine(RangeY);
	array<Byte>^ dataX = gcnew array<Byte>(RangeX->Length * sizeof(double));
	Buffer::BlockCopy(RangeX, 0, dataX, 0, dataX->Length);

	array<Byte>^ dataY = gcnew array<Byte>(RangeY->Length * sizeof(double));
	Buffer::BlockCopy(RangeY, 0, dataY, 0, dataY->Length);

	// send byte data over connection
	Stream->Write(dataX, 0, dataX->Length);
	Thread::Sleep(10);
	Stream->Write(dataY, 0, dataY->Length);
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
	if (!getShutdownFlag() && connect("127.0.0.1", 28000) == error_state::SUCCESS) {
		Console::WriteLine("Connected to Display Successfully.");
		while (!getShutdownFlag()) {
			// Console::WriteLine("Display Thread is running.");
			processHeartBeats();
			// Display functionality
			processSharedMemory();
			communicate();
			Thread::Sleep(10);
		}
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
			// printing error here before modules are shut down
			printError(error_state::ERR_TMM_FAILURE);
			shutdownModules();
			return error_state::ERR_TMM_FAILURE;
		}
	}
	return error_state::SUCCESS;
}
