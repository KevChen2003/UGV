#include <msclr/marshal_cppstd.h>
#include "Laser.h"

Laser::Laser() {}

Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_Gps_ = SM_Gps;
	// not sure if stopwatch required below as its intiialised in the thread function again later
	//Watch = gcnew Stopwatch;
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
		Console::WriteLine("Connected to Laser Successfully.");
		if (sendCommand("5360742\n") == error_state::SUCCESS) {
			Console::WriteLine("Authentication Successful.");
		}
		// configure scan and start measurement
		// if (communicate("\x02sMN mLMPsetscancfg 5000 1 5000 0 1800000\x03") == error_state::SUCCESS && communicate("\x02sMN LMCstartmeas\x03") == error_state::SUCCESS) {
		while (!getShutdownFlag()) {
			Console::WriteLine("Laser Thread is running.");
			processHeartBeats();
			// laser functionality
			/*
			if (communicate() == error_state::SUCCESS && checkData() == error_state::SUCCESS) {
				// if communication is successful and data is successful, put the data in laser shared memory
				processSharedMemory();
			}
			*/
			// start scanning, setting parameter to 1 enables the continuous transmission of scan data
			// communicate("\x02sEN LMDscandata 1\x03");
			// only scan once to not hold up the thread 

			// keep track of how many points have been printed
			int PointNum = 0;
			error_state response = sendCommand("\x02sRN LMDscandata\x03");
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
			Console::WriteLine(StringArray);
			if (StringArray->Length > 25) {
				try {
					// point calculation from lectuers
					double StartAngle = System::Convert::ToInt32(StringArray[23], 16);
					double Resolution = System::Convert::ToInt32(StringArray[24], 16) / 10000.0;
					int NumRanges = System::Convert::ToInt32(StringArray[25], 16);

					array<double>^ Range = gcnew array<double>(NumRanges);
					array<double>^ RangeX = gcnew array<double>(NumRanges);
					array<double>^ RangeY = gcnew array<double>(NumRanges);

					for (int i = 0; i < NumRanges; i++) {
						Range[i] = System::Convert::ToInt32(StringArray[26 + i], 16);
						//
						// ERROR: USE REAL PI VALUE!!!!!!
						//
						RangeX[i] = Range[i] * cos(i * Resolution * 3.1415/180);
						RangeY[i] = Range[i] * sin(i * Resolution * 3.1415/180);
						// print out the X and Y
						Console::WriteLine("Point {0:D}:,  X: {1:F3}, Y: {2:F3}", PointNum++, RangeX[i], RangeY[i]);
					}
				} catch (System::FormatException^ e) {
					Console::WriteLine("Format Exception: {0}", e->Message);
				}
				catch (Exception^ e) {
					Console::WriteLine("Error found: {0}", e->Message);
				}
			}

			Thread::Sleep(20);
		}
		/*
		}
		else {
			Console::WriteLine("Failed to configure scan, or start measurement.");
		}
		*/ 
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
	return error_state::SUCCESS;
}

error_state Laser::communicate() {
	return error_state::SUCCESS;
}

error_state Laser::sendCommand(String^ command) {
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