#include <Arduino.h>
#include "globals.h"
#include "config.h"
#include "serialDebugPrint.h"
#include "TCPServerRouter.h"
#include "Maintenance.h"

bool testMode = true;
Maintenance MaintenanceHandle;

void setup()
{
    MACHINE_SERIAL.begin(9600);
    SerialDebugPrintInit();
    SerialDebugPrint("Setup Complete!");
}

void loop()
{
    if (MaintenanceHandle.IsMaintenanceModeActive() == true || testMode == true)
    {
        TCPServer.Deinit();
        MaintenanceHandle.Init();
        MaintenanceHandle.Run();
    }
    else
    {
        MaintenanceHandle.Deinit();
        TCPServer.Init();
        TCPServer.Run();
    }
}

void TCPMessageTimerCallback(TimerHandle_t xTimer)
{
    TCPServer.SetTCPMessageTimeout(true);
}
