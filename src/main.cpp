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
    SerialDebugPrint("Setup Complete!");
}

void loop()
{
    if (MaintenanceHandle.IsMaintenanceModeActive() == true)
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
