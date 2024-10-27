#pragma once

#using <System.dll>
#include "SMObjects.h"
#include <UGVModule.h>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class Laser {
public:

    Laser();

    // Create shared memory objects
    error_state setupSharedMemory();

    void threadFunction();

    error_state processHeartBeats();

    void shutdownThreads();

    // Get Shutdown signal for module 
    bool getShutdownFlag() override;

    error_state communicate();
    
    error_state checkData();

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    ~Laser() {};

private:
    // Add any additional data members or helper functions here
    SM_ThreadManagement^ SM_TM_;
    SM_GPS^ SM_Gps_;
    Stopwatch^ Watch;
};
