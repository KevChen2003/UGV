#include <msclr/marshal_cppstd.h>
#include "GNSS.h"

GNSS::GNSS() {}

GNSS::GNSS(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps) {
	SM_TM_ = SM_TM;
	SM_Gps_ = SM_Gps;
	SM_Laser_ = SM_Laser;
}

error_state GNSS::setupSharedMemory() {
	return error_state::SUCCESS;
}

bool GNSS::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_GPS);
}

void GNSS::threadFunction() {
	Console::WriteLine("GNSS Thread is starting.");
	// initialise stopwatch
	Watch = gcnew Stopwatch;
	// wait at the barrier for other threads
	SM_TM_->ThreadBarrier->SignalAndWait();
	// start stopwatch
	Watch->Start();
	if (!getShutdownFlag() && connect(WEEDER_ADDRESS, 24000) == error_state::SUCCESS) {
		Console::WriteLine("Connected to GNSS Successfully.");
		while (!getShutdownFlag()) {
			// Console::WriteLine("GNSS Thread is running.");
			processHeartBeats();
			// GNSS functionality 
			if (communicate() == error_state::SUCCESS && checkData() == error_state::SUCCESS) {
				// if communication is successful and data is successful, put the data in GNSS shared memory
				processSharedMemory();

				// print values
				Console::WriteLine("Northing: {0}, Easting: {1}, Height: {2}, Received CRC: {3}, Calculated CRC: {4}", Northing, Easting, Height, CRC, CalculatedCRC);
			}
			Thread::Sleep(20);
		}
	}
	Console::WriteLine("GNSS Thread is terminating.");
}

error_state GNSS::processHeartBeats() {
	if ((SM_TM_->heartbeat & bit_GPS) == 0) {
		// if the GNSS bit in the heartbeat byte is down, put the bit back up
		SM_TM_->heartbeat |= bit_GPS;
		// reset stopwatch
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			// if the GNSS bit is up and the watch has exceeded the limit
			// shutdown all threads
			shutdownModules();
			return error_state::ERR_TMM_FAILURE;
		}
	}
	return error_state::SUCCESS;
}


error_state GNSS::checkData() {
	// ensure calculated checksum and checksum sent from bytes are correct
	/*
	// convert between managed array ReadData to native array
	pin_ptr<unsigned char> PinnedArray = &ReadData[0];
	unsigned char* NativeArray = PinnedArray;
	// gives data size as ReadData is an array of chars anyway which is 1 byte each
	unsigned long DataSize = static_cast<unsigned long>(ReadData->Length);
	unsigned long crc = CalculateBlockCRC32(DataSize, NativeArray);
	*/

	unsigned long DataSizeWithoutCRC = sizeof(GNSSData) - 4; // -4 to remove CRC data

	CalculatedCRC = CalculateBlockCRC32(DataSizeWithoutCRC, GNSSNativeArray);
	if (CalculatedCRC != CRC) {
		Console::WriteLine("CRC do not match. Calculated: {0}, Received: {1}", CalculatedCRC, CRC);
		return error_state::ERR_INVALID_DATA;
	}

	return error_state::SUCCESS;
}

error_state GNSS::processSharedMemory() {
	return error_state::SUCCESS;
}

GNSS::~GNSS() {}

error_state GNSS::connect(String^ hostName, int portNumber) {
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
error_state GNSS::communicate() {
	if (Client == nullptr || Stream == nullptr) {
		return error_state::ERR_CONNECTION;
	}
	try {
		// Stream->Read(ReadData, 0, ReadData->Length);

		unsigned int Header = 0;
		Byte Data;
		do {
			Data = Stream->ReadByte();
			Header = (Header << 8) | Data;
		} while (Header != 0xaa44121c);

		GNSSData GNSSStruct;

		// read 108 bytes of data into GNSS Struct
		array<unsigned char>^ buffer = gcnew array<unsigned char>(108);
		int bytesRead = Stream->Read(buffer, 0, buffer->Length);
		// Ensure we've read all 108 bytes
		if (bytesRead != 108) {
			Console::WriteLine("Error: Could not read 108 bytes of data.");
			return error_state::ERR_RESPONSE;
		}
		// Pin the buffer and copy the data into the GNSSData struct
		pin_ptr<unsigned char> pinnedBuffer = &buffer[0];
		GNSSNativeArray = pinnedBuffer;
		memcpy(&GNSSStruct, pinnedBuffer, sizeof(GNSSData));

		CRC = GNSSStruct.CRC;
		Northing = GNSSStruct.Northing;
		Easting = GNSSStruct.Easting;
		Height = GNSSStruct.Height;

		return error_state::SUCCESS;
	}
	catch (Exception^ e) {
		std::string ErrorMessage = msclr::interop::marshal_as<std::string>(e->Message);
		std::cerr << "Exception found when sending command: " << ErrorMessage << '\n';
		return error_state::ERR_RESPONSE;
	}
}

void GNSS::shutdownModules() {
	SM_TM_->shutdown = bit_ALL;
}

// checksum functions obtained from appendix A
unsigned long GNSS::CRC32Value(int i)
{
	int j;
	unsigned long ulCRC;
	ulCRC = i;
	for (j = 8; j > 0; j--)
	{
		if (ulCRC & 1)
			ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
		else
			ulCRC >>= 1;
	}
	return ulCRC;
}

unsigned long GNSS::CalculateBlockCRC32(
	unsigned long ulCount, /* Number of bytes in the data block */
	unsigned char* ucBuffer) /* Data block */
{
	unsigned long ulTemp1;
	unsigned long ulTemp2;
	unsigned long ulCRC = 0;
	while (ulCount-- != 0)
	{
		ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
		ulTemp2 = CRC32Value(((int)ulCRC ^ * ucBuffer++) & 0xff);
		ulCRC = ulTemp1 ^ ulTemp2;
	}
	return(ulCRC);
}