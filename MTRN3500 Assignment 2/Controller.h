#pragma once

#include <ControllerInterface.h>
#include <UGVModule.h>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class Controller : public UGVModule {
public:

    Controller();
    
    Controller(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps, SM_VehicleControl^ SM_VC);

    ~Controller();

    error_state processHeartBeats();

    // Create shared memory objects
    error_state setupSharedMemory();

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    // Get Shutdown signal for module, from Thread Management SM
    bool getShutdownFlag() override;

    // Thread function for TMM
    void threadFunction() override;

    void shutdownModules();

private:
    // Add any additional data members or helper functions here
    SM_ThreadManagement^ SM_TM_;
    SM_Laser^ SM_Laser_;
    SM_GPS^ SM_Gps_;
    SM_VehicleControl^ SM_VC_;
    Stopwatch^ Watch;
    ControllerInterface* ControllerInterface_;
};
