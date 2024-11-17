#pragma once

#include <UGVModule.h>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class CrashAvoidance : public UGVModule {
public:

    CrashAvoidance();

    CrashAvoidance(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps, SM_CrashAvoidance^ SM_CA);

    ~CrashAvoidance();

    // Create shared memory objects
    error_state setupSharedMemory();

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    // Get Shutdown signal for module, from Thread Management SM
    bool getShutdownFlag() override;

    // Thread function for TMM
    void threadFunction() override;

    error_state processHeartBeats();

    void shutdownModules();

    bool CheckDistance(double x, double y);

    error_state SetFlags(bool CanGoForwards, bool CanSteerLeft, bool CanSteerRight);

private:
    // Add any additional data members or helper functions here
    SM_ThreadManagement^ SM_TM_;
    SM_Laser^ SM_Laser_;
    SM_GPS^ SM_Gps_;
    SM_CrashAvoidance^ SM_CA_;
    Stopwatch^ Watch;
    array<double>^ RangeX;
    array<double>^ RangeY;
};
