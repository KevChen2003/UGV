#include "Laser.h"

Laser::Laser() {}

error_state Laser::setupSharedMemory() {
	return error_state::SUCCESS;
}

void Laser::threadFunction() {}

void Laser::shutdownThreads() {}

bool Laser::getShutdownFlag() {
	return true;
}

error_state Laser::communicate() {
	return error_state::SUCCESS;
}

error_state Laser::processHeartBeats() {
	return error_state::SUCCESS;
}

error_state Laser::checkData() {
	return error_state::SUCCESS;
}

// Send/Recieve data from shared memory structures
error_state Laser::processSharedMemory() {
	return error_state::SUCCESS;
}

Laser::~Laser() {}