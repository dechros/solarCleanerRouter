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

    SerialDebugPrint("Maintenance Mode");
    WiFi.mode(WIFI_STA);
    WiFi.begin(MAINTENANCE_SSID, MAINTENANCE_PASSWORD);
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
            SerialDebugPrint("Maintenance Mode Active");
            TCPServer.Deinit();
            Init();
            maintenanceModeActive = true;
            MACHINE_SERIAL.write(ACK_MESSAGE, 3);
        }
        else if (strncmp((const char *)buffer, MAINTENANCE_MODE_DEACTIVE_MESSAGE, 4) == 0)
        {
            SerialDebugPrint("Maintenance Mode deactive");
            Deinit();
            TCPServer.Init();
            maintenanceModeActive = false;
            MACHINE_SERIAL.write(ACK_MESSAGE, 3);
        }
    }
    else if(MACHINE_SERIAL.peek() == 'M' && MACHINE_SERIAL.available() < 4)
    {
        for (uint8_t i = 0; i < 10; i++)
        {
            if (MACHINE_SERIAL.available() < 4)
            {
                vTaskDelay(5 / portTICK_PERIOD_MS);
            }
            else
            {
                uint8_t buffer[4];
                MACHINE_SERIAL.readBytes((uint8_t *)buffer, 4);
                if (strncmp((const char *)buffer, MAINTENANCE_MODE_ACTIVE_MESSAGE, 4) == 0)
                {
                    SerialDebugPrint("Maintenance Mode Active");
                    TCPServer.Deinit();
                    Init();
                    maintenanceModeActive = true;
                    MACHINE_SERIAL.write(ACK_MESSAGE, 3);
                }
                else if (strncmp((const char *)buffer, MAINTENANCE_MODE_DEACTIVE_MESSAGE, 4) == 0)
                {
                    SerialDebugPrint("Maintenance Mode deactive");
                    Deinit();
                    TCPServer.Init();
                    maintenanceModeActive = false;
                    MACHINE_SERIAL.write(ACK_MESSAGE, 3);
                }
            }
        }
    }
    else if(MACHINE_SERIAL.peek() != 'M' && MACHINE_SERIAL.available() > 0)
    {
        SerialDebugPrint("Discarded buffer wrong header");
        SerialDebugPrint(MACHINE_SERIAL.peek());
        MACHINE_SERIAL.read();
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
                vTaskDelay(3000 / portTICK_PERIOD_MS);
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
            static uint8_t cannotSentCounter = 0;
            RequestApiState_t fetchResult = FetchMachineData(WEB_SITE, machineParameters);

            if (fetchResult == SUCCESS_REQ)
            {
                uint8_t size = MACHINE_SERIAL.available();
                for (uint16_t i = 0; i < size; i++)
                {
                    MACHINE_SERIAL.read();
                }
                RequestApiState_t sentResult = SendParamToMachine(machineParameters);
                if (sentResult == SUCCESS_REQ)
                {
                    SerialDebugPrint("Machine is updated.");
                }
                else
                {
                    SerialDebugPrint("Machine send error.");
                    cannotSentCounter++;
                    if (cannotSentCounter == 3)
                    {
                        cannotSentCounter = 0;
                        SetMaintenanceState(WIFI_CONNECTION_STATE);
                    }
                }
            }
            else
            {
                SerialDebugPrint("FetchMachineData API Communication error.");
                SetMaintenanceState(WIFI_CONNECTION_STATE);
            }
            
            vTaskDelay(5000 / portTICK_PERIOD_MS);
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

    while (millis() - startTime < TIMEOUT_MS)
    {
        if (MACHINE_SERIAL.available() > 0)
        {
            char inChar = (char)MACHINE_SERIAL.read();
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
    MACHINE_SERIAL.write(SET_PARAMETERS_MESSAGE, 3);
    MACHINE_SERIAL.write((uint8_t *)&machine, sizeof(machine));
    if (ReceiveAck(SET_PARAMETERS_ACK) == false)
    {
        Serial.println("SET ACK Failed.");
        return retVal;
    }

    Serial.println("SET ACK OK.");

    retVal = SUCCESS_REQ;
    return retVal;
}

RequestApiState_t Maintenance::GetParamFromMachine(Parameters_t &machine)
{
    RequestApiState_t retVal = FAIL_REQ;
    unsigned long startTime = millis();
    MACHINE_SERIAL.write(GET_PARAMETERS_MESSAGE, 3);

    if (ReceiveAck(GET_PARAMETERS_ACK) == false)
    {
        Serial.println("GET ACK Failed.");
        return retVal;
    }

    Serial.println("GET ACK OK.");

    size_t bytesReceived = 0;
    while ((millis() - startTime < TIMEOUT_MS) && (bytesReceived < sizeof(machine)))
    {
        if (MACHINE_SERIAL.available() > 0)
        {
            bytesReceived += MACHINE_SERIAL.readBytes((uint8_t *)&machine + bytesReceived, sizeof(machine) - bytesReceived);
        }
    }

    if (millis() - startTime > TIMEOUT_MS)
    {
        Serial.println("GET timeout.");
        return retVal;
    }

    if (bytesReceived != sizeof(machine))
    {
        Serial.println("GET incorrect number of bytes received.");
        return retVal;
    }

    machine.machineIP[0] = STATIC_IP[0];
    machine.machineIP[1] = STATIC_IP[1];
    machine.machineIP[2] = STATIC_IP[2];
    machine.machineIP[3] = STATIC_IP[3];

    retVal = SUCCESS_REQ;
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