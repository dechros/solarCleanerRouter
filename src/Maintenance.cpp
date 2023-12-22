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
    if (initDone == true)
    {
        return;
    }

    WiFi.mode(WIFI_STA);
    initDone = true;
}

void Maintenance::Deinit(void)
{
    if (initDone == false)
    {
        return;
    }
    
    WiFi.disconnect();
    initDone = false;
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
    static Parameters_t machineParameters;

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
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
        break;
    }
    case CONNECTION_ESTABLISHED:
    {
        RequestApiState_t machineRequest = GetParamFromMachine(machineParameters);
        if (machineRequest == FAIL_REQ)
        {
            SerialDebugPrint("Machine request error.");
            SetMaintenanceState(WIFI_CONNECTION_STATE);
        }
        else
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            RequestApiState_t isMachineInDB = IsMachinePresentInDB(WEB_SITE, machineParameters);

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
        RequestApiState_t fetchResult = FetchMachineData(WEB_SITE, machineParameters);

        if (fetchResult == SUCCESS_REQ)
        {
            RequestApiState_t sentResult = SendParamToMachine(machineParameters);
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

bool Maintenance::ReceiveAck(const char *expectedAck)
{
    unsigned long startTime = millis();
    int ackLength = strlen(expectedAck);
    int matchIndex = 0;

    while (millis() - startTime < ACK_TIMEOUT_MS)
    {
        if (Serial.available() > 0)
        {
            char inChar = (char)Serial.read();
            if (inChar == expectedAck[matchIndex])
            {
                matchIndex++;
                if (matchIndex == ackLength)
                {
                    return true;
                }
            }
            else
            {
                matchIndex = 0;
            }
        }
    }
    return false;
}

RequestApiState_t Maintenance::SendParamToMachine(const Parameters_t machine)
{
    RequestApiState_t retVal = FAIL_REQ;
    MACHINE_SERIAL.print(SET_PARAMETERS_MESSAGE);
    MACHINE_SERIAL.write((uint8_t *)&machine, sizeof(machine));
    if (ReceiveAck(SET_PARAMETERS_ACK) == true)
    {
        retVal = SUCCESS_REQ;
    }
    return retVal;
}

RequestApiState_t Maintenance::GetParamFromMachine(Parameters_t &machine)
{
    RequestApiState_t retVal = FAIL_REQ;
    MACHINE_SERIAL.print(GET_PARAMETERS_MESSAGE);
    if (ReceiveAck(GET_PARAMETERS_ACK) == true)
    {
        MACHINE_SERIAL.readBytes((uint8_t *)&machine, sizeof(machine));
        retVal = SUCCESS_REQ;
    }
    return retVal;
}

MaintenanceState_t Maintenance::GetMaintenanceState(void)
{
    return maintenanceState;
}

void Maintenance::SetMaintenanceState(MaintenanceState_t state)
{
    maintenanceState = state;
}

void testMachine(Parameters_t &machine)
{
    strncpy((char *)machine.companyName, "Test Api 3", sizeof(machine.companyName) - 1);
    machine.companyName[sizeof(machine.companyName) - 1] = '\0';

    std::vector<int> ipVec = SplitIpString("192.168.31.31");
    machine.machineIP[0] = ipVec[0];
    machine.machineIP[1] = ipVec[1];
    machine.machineIP[2] = ipVec[2];
    machine.machineIP[3] = ipVec[3];

    int seedValue = analogRead(39);
    randomSeed(seedValue);

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
}