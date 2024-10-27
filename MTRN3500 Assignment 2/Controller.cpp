#include "Controller.h"

error_state Controller::setupSharedMemory() {
	return error_state::SUCCESS;
}

error_state Controller::processSharedMemory() {
	return error_state::SUCCESS;
}

void Controller::shutdownModules() {}

bool Controller::getShutdownFlag() {
	return true;
}

error_state Controller::processHeartBeats() {
	return error_state::SUCCESS;
}

void Controller::threadFunction() {}

Controller::Controller() {}

Controller::~Controller() {}