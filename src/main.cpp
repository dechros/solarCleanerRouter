#include <Arduino.h>
#include "globals.h"
#include "config.h"
#include "serialDebugPrint.h"
#include "TCPServerRouter.h"
#include "Maintenance.h"

Maintenance MaintenanceHandle;

void setup()
{
    MACHINE_SERIAL.begin(9600);
    SerialDebugPrintInit();
    TCPServer.Init();
    SerialDebugPrint("Setup Complete!");
}

void loop()
{
    if (MaintenanceHandle.IsMaintenanceModeActive() == true)
    {
        MaintenanceHandle.Run();
    }
    else
    {
        TCPServer.Run();
    }
}

void TCPMessageTimerCallback(TimerHandle_t xTimer)
{
    TCPServer.SetTCPMessageTimeout(true);
}
