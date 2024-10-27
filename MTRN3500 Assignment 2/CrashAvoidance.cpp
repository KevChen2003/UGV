#include "CrashAvoidance.h"

error_state CrashAvoidance::setupSharedMemory() {
	return error_state::SUCCESS;
}

error_state CrashAvoidance::processSharedMemory() {
	return error_state::SUCCESS;
}

void CrashAvoidance::shutdownModules() {}

bool CrashAvoidance::getShutdownFlag() {
	return true;
}

void CrashAvoidance::threadFunction() {}

CrashAvoidance::CrashAvoidance() {}

CrashAvoidance::~CrashAvoidance() {}

error_state CrashAvoidance::processHeartBeats() {
	return error_state::SUCCESS;
}