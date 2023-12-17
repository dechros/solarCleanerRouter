#include <WiFi.h>
#include "Maintenance.h"
#include "config.h"
#include "globals.h"
#include "serialDebugPrint.h"

Maintenance::Maintenance()
{
    /* Empty Constructor */
}

Maintenance::~Maintenance()
{
    /* Empty Destructor */
}

void Maintenance::Init(void)
{
    WiFi.mode(WIFI_STA);
}

void Maintenance::Deinit(void)
{

}

bool Maintenance::IsMaintenanceModeActive(void)
{
    if (MACHINE_SERIAL.peek() == 'M' && MACHINE_SERIAL.available() >= 4)
    {
        uint8_t buffer[4];
        MACHINE_SERIAL.readBytes((uint8_t *)buffer, 4);
        if (strncmp((const char *)buffer, MAINTENANCE_MODE_ACTIVE_MESSAGE, 4) == 0)
        {
            TCPServer.Deinit();
            Init();
            maintenanceModeActive = true;
        }
        else if (strncmp((const char *)buffer, MAINTENANCE_MODE_DEACTIVE_MESSAGE, 4) == 0)
        {
            TCPServer.Init();
            maintenanceModeActive = false;
        }
    }
    return maintenanceModeActive;
}

void Maintenance::Run(void)
{
    switch (GetMaintenanceState())
    {
        case WIFI_CONNECTION_STATE:
        {   
            WiFi.begin(MAINTENANCE_SSID, MAINTENANCE_PASSWORD);
            if (WiFi.status() != WL_CONNECTED) 
            {
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                Serial.println("Connecting to WiFi...");
            }
            else
            {
                SetMaintenanceState(CHECK_WEBSITE_STATE);
            }
            break;
        }
        case CHECK_WEBSITE_STATE:
        {   
            IPAddress ip;
            if ( WiFi.hostByName(WEB_SITE, ip)) 
            {
                SerialDebugPrint("Ping successful!");
                SetMaintenanceState(GET_PARAMETERS_STATE);
            } 
            else 
            {
                SerialDebugPrint("Ping failed.");
                vTaskDelay(3000 / portTICK_PERIOD_MS);
            }
            break;
        }
        case GET_PARAMETERS_STATE:
        {   
            SerialDebugPrint("Get Parameters State");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            break;
        }
        case UPDATE_MACHINE_STATE:
        {   
            break;
        }
        default:
        {
            break;
        }
    }
}

MaintenanceState_t Maintenance::GetMaintenanceState(void)
{
    return maintenanceState;
}

void Maintenance::SetMaintenanceState(MaintenanceState_t state)
{
    maintenanceState = state;
}