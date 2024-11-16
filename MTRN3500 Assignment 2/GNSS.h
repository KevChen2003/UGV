#pragma once

#using <System.dll>
#include "SMObjects.h"
#include <UGVModule.h>
#include <NetworkedModule.h>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

#define CRC32_POLYNOMIAL 0xEDB88320L

#pragma pack(push, 4)
struct GNSSData {
    unsigned int header; // 4 bytes: 0xAA 0x44 0x12 0x1C
    unsigned char Discards1[40];
    double Northing;
    double Easting;
    double Height;
    unsigned char Discards2[40];
    unsigned int CRC;
};
#pragma pack(pop, 4)

ref class GNSS : public NetworkedModule {
public:

    GNSS();

    GNSS(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_Gps);

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

    // from appendix A
    unsigned long CRC32Value(int i);
    unsigned long CalculateBlockCRC32(unsigned long ulCount, unsigned char* ucBuffer);

    ~GNSS();

private:
    // Add any additional data members or helper functions here
    SM_ThreadManagement^ SM_TM_;
    SM_Laser^ SM_Laser_;
    SM_GPS^ SM_Gps_;
    Stopwatch^ Watch;
    unsigned char* GNSSNativeArray;
    double Northing;
    double Easting;
    double Height;
    unsigned int CRC;
    unsigned int CalculatedCRC;
};
