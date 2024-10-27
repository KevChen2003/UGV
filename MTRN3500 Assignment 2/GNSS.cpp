#include "GNSS.h"

GNSS::GNSS() {}

error_state GNSS::setupSharedMemory() {
	return error_state::SUCCESS;
}

void GNSS::threadFunction() {}

void GNSS::shutdownThreads() {}

bool GNSS::getShutdownFlag() {
	return true;
}

error_state GNSS::communicate() {
	return error_state::SUCCESS;
}

error_state GNSS::processHeartBeats() {
	return error_state::SUCCESS;
}

error_state GNSS::checkData() {
	return error_state::SUCCESS;
}

error_state GNSS::processSharedMemory() {
	return error_state::SUCCESS;
}

GNSS::~GNSS() {}