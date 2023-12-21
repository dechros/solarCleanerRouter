/**
 * @file Maintenance.cpp
 * @author Halit Cetin (halitcetin@live.com)
 * @brief Maintenance mod header file.
 * @version 0.1
 * @date 2023-12-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "Maintenance.h"

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
    Parameters_t machineParameters;

    switch (GetMaintenanceState())
    {
    case WIFI_CONNECTION_STATE:
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            SerialDebugPrint("Connected to WiFi.");
            SetMaintenanceState(CONNECTION_ESTABLISHED);
        }
        else
        {
            SerialDebugPrint("Connecting to WiFi...");
            WiFi.begin(MAINTENANCE_SSID, MAINTENANCE_PASSWORD);
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
        break;
    }
    case CONNECTION_ESTABLISHED:
    {
        RequestApiState_t machineRequest = RequestParametersFromMachine(machineParameters);
        if (machineRequest == FAIL_REQ)
        {
            SerialDebugPrint("Machine request error.");
            SetMaintenanceState(WIFI_CONNECTION_STATE);
        }
        else
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            RequestApiState_t isMachineInDB = IsMachinePresentInDB(WEB_SITE, (char *)machineParameters.companyName);

            if (isMachineInDB == FALSE_REQ)
            {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                RequestApiState_t createResult = CreateMachine(WEB_SITE, machineParameters);

                if (createResult == SUCCESS_REQ)
                {
                    SerialDebugPrint("Machine is created.");
                    SetMaintenanceState(CONTINIOUS_UPDATE);
                }
                else
                {
                    SerialDebugPrint("CreateMachine API Communication error.");
                    SetMaintenanceState(WIFI_CONNECTION_STATE);
                }
            }
            else if (isMachineInDB == TRUE_REQ)
            {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                RequestApiState_t setResult = SetMachineData(WEB_SITE, machineParameters);

                if (setResult == SUCCESS_REQ)
                {
                    SerialDebugPrint("Machine DB is updated.");
                    SetMaintenanceState(CONTINIOUS_UPDATE);
                }
                else
                {
                    SerialDebugPrint("SetMachineData API Communication error.");
                    SetMaintenanceState(WIFI_CONNECTION_STATE);
                }
            }
            else
            {
                SerialDebugPrint("IsMachinePresentInDB API Communication error.");
                SetMaintenanceState(WIFI_CONNECTION_STATE);
            }
        }
        break;
    }
    case CONTINIOUS_UPDATE:
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        RequestApiState_t fetchResult = FetchMachineData((char *)machineParameters.companyName, WEB_SITE, machineParameters);

        if (fetchResult == SUCCESS_REQ)
        {
            RequestApiState_t sentResult = SentParametersToMachine(machineParameters);
            if (sentResult == SUCCESS_REQ)
            {
                SerialDebugPrint("Machine is updated.");
            }
            else
            {
                SerialDebugPrint("Machine send error.");
                SetMaintenanceState(WIFI_CONNECTION_STATE);
            }
        }
        else
        {
            SerialDebugPrint("FetchMachineData API Communication error.");
            SetMaintenanceState(WIFI_CONNECTION_STATE);
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

RequestApiState_t Maintenance::SentParametersToMachine(Parameters_t machine)
{
    return SUCCESS_REQ;
}

RequestApiState_t Maintenance::RequestParametersFromMachine(Parameters_t &machine)
{
    char *testName = "Test Api 3";
    char *testIP = "192.168.31.31";

    strncpy((char *)machine.companyName, testName, strlen(testName));

    std::vector<int> ipVec = SplitIpString(testIP);
    machine.machineIP[0] = ipVec[0];
    machine.machineIP[1] = ipVec[1];
    machine.machineIP[2] = ipVec[2];
    machine.machineIP[3] = ipVec[3];

    machine.leftErrorCount = rand() % 5 + 1;
    machine.rightErrorCount = rand() % 5 + 1;
    machine.brushErrorCount = rand() % 5 + 1;
    machine.controllerErrorCount = rand() % 5 + 1;

    machine.leftRampUp = 6;
    machine.leftRampDown = 4;
    machine.rightRampUp = 7;
    machine.rightRampDown = 3;
    machine.brushRampUp = 5;
    machine.brushRampDown = 2;

    machine.leftMinSpeed = 1;
    machine.leftMaxSpeed = 8;
    machine.rightMinSpeed = 2;
    machine.rightMaxSpeed = 9;
    machine.brushMinSpeed = 3;
    machine.brushMaxSpeed = 10;

    machine.joystickMiddleValue = 127;
    machine.joystickDeadZone = 5;
    machine.joystickMinValue = 0;
    machine.joystickMaxValue = 255;
    machine.potantiometerMinValue = 20;
    machine.potantiometerMaxValue = 230;

    return SUCCESS_REQ;
}

MaintenanceState_t Maintenance::GetMaintenanceState(void)
{
    return maintenanceState;
}

void Maintenance::SetMaintenanceState(MaintenanceState_t state)
{
    maintenanceState = state;
}
