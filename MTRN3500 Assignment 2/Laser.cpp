#include <msclr/marshal_cppstd.h>
#include "Laser.h"

Laser::Laser() {}

Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_Gps_ = SM_Gps;
}

error_state Laser::setupSharedMemory() {
	return error_state::SUCCESS;
}

bool Laser::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_LASER);
}

void Laser::threadFunction() {
	Console::WriteLine("Laser Thread is starting.");
	// initialise stopwatch
	Watch = gcnew Stopwatch;
	// wait at the barrier for other threads
	SM_TM_->ThreadBarrier->SignalAndWait();
	// start stopwatch
	Watch->Start();
	// connect once to the laser
	if (!getShutdownFlag() && connect(WEEDER_ADDRESS, 23000) == error_state::SUCCESS) {
		Console::WriteLine("Connected to Laser Successfully + Authenticated.");
		while (!getShutdownFlag()) {
			processHeartBeats();
			// start scanning, setting parameter to 1 enables the continuous transmission of scan data
			// communicate("\x02sEN LMDscandata 1\x03");
			// only scan once to not hold up the thread 

			// keep track of how many points have been printed
			int PointNum = 0;
			error_state response = communicate();

			if (response != error_state::SUCCESS) {
				Console::WriteLine("Error trying to scan data.");
				break;
			}
			// read data
			String^ ScanData = System::Text::Encoding::ASCII->GetString(ReadData);
			// Console::WriteLine("SCAN DATA BELOWWWWWWWWW!!!!!");
			// Console::WriteLine(ScanData);
			// array<wchar_t>^ Space = { ' ' };
			array<String^>^ StringArray = ScanData->Split(' ');
			// Console::WriteLine(StringArray);

			// validate the data before using it
			if (StringArray->Length > 25) {
				try {
					// point calculation from lectuers
					double StartAngle = System::Convert::ToInt32(StringArray[23], 16);
					double Resolution = System::Convert::ToInt32(StringArray[24], 16) / 10000.0;
					int NumRanges = System::Convert::ToInt32(StringArray[25], 16);

					array<double>^ Range = gcnew array<double>(NumRanges);
					RangeX = gcnew array<double>(NumRanges);
					RangeY = gcnew array<double>(NumRanges);

					for (int i = 0; i < NumRanges; i++) {
						Range[i] = System::Convert::ToInt32(StringArray[26 + i], 16);

						double pi = Math::PI;
						RangeX[i] = Range[i] * cos(i * Resolution * pi/180);
						RangeY[i] = Range[i] * sin(i * Resolution * pi/180);
						// print out the X and Y
						// Console::WriteLine("Point {0:D}:,  X: {1:F3}, Y: {2:F3}", PointNum++, RangeX[i], RangeY[i]);

						// send it to Display
						processSharedMemory();
					}
				} catch (System::FormatException^ e) {
					Console::WriteLine("Format Exception: {0}", e->Message);
				}
				catch (Exception^ e) {
					Console::WriteLine("Error found: {0}", e->Message);
				}
			}

			Thread::Sleep(10);
		}
	}
	// stop measurement
	/*
	error_state response = communicate("\x02sMN LMCstopmeas\x03");
	if (response != error_state::SUCCESS) {
		Console::WriteLine("Error trying to stop measurement.");
	}
	*/
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
			Console::WriteLine("TMM Failure.");
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
	// Enter the monitor to ensure thread-safe access
	Monitor::Enter(SM_Laser_->lockObject);
	try {
		// set the x and y values
		SM_Laser_->x = RangeX;
		SM_Laser_->y = RangeY;
	}
	finally {
		// Exit the monitor
		Monitor::Exit(SM_Laser_->lockObject);
	}
	return error_state::SUCCESS;
}

error_state Laser::communicate() {
	if (Client == nullptr || Stream == nullptr) {
		return error_state::ERR_CONNECTION;
	}
	try {
		SendData = System::Text::Encoding::ASCII->GetBytes("\x02sRN LMDscandata\x03");
		Stream->Write(SendData, 0, SendData->Length);
		// increase delay to remove trhe string error, but this delays the sim more
		Threading::Thread::Sleep(20);
		Stream->Read(ReadData, 0, ReadData->Length);
		return error_state::SUCCESS;
	}
	catch (Exception^ e) {
		std::string ErrorMessage = msclr::interop::marshal_as<std::string>(e->Message);
		std::cerr << "Exception found when sending command: " << ErrorMessage << '\n';
		return error_state::ERR_RESPONSE;
	}
}

error_state Laser::connect(String^ hostName, int portNumber) {
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

void Laser::shutdownModules() {
	// surely can find a way to make shutdownModules transferrable to all modules, but that probably means
	// having a UGVModule.cpp which i dont have so will keep repeating for now
	SM_TM_->shutdown = bit_ALL;
}

Laser::~Laser() {}