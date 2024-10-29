#pragma once

#using <System.dll>
#include "SMObjects.h"
#include <UGVModule.h>
#include <NetworkedModule.h>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class Laser : public NetworkedModule {
public:

    Laser();

    Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps);

    // Create shared memory objects
    error_state setupSharedMemory();

    void threadFunction() override;

    error_state processHeartBeats();

    // Get Shutdown signal for module 
    bool getShutdownFlag() override;
    
    error_state checkData();

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    void shutdownModules();

    virtual error_state connect(String^ hostName, int portNumber) override;
    virtual error_state communicate() override;
    error_state communicate(String^ command);

    ~Laser();

private:
    // Add any additional data members or helper functions here
    SM_ThreadManagement^ SM_TM_;
    SM_GPS^ SM_Gps_;
    SM_Laser^ SM_Laser_;
    Stopwatch^ Watch;
    array<unsigned char>^ SendData;		// array for sending data
};
