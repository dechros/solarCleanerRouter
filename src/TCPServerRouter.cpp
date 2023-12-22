#include "TCPServerRouter.h"
#include "serialDebugPrint.h"
#include "config.h"

TCPServerRouter::TCPServerRouter()
{
    /* Empty Constructor */
}

TCPServerRouter::~TCPServerRouter()
{
    /* Empty Destructor */
}

void TCPServerRouter::Init(void)
{
    if (initDone == true)
    {
        return;
    }

    server = WiFiServer(TCP_SERVER_LISTEN_PORT); /* Port */
    IPAddress staticIP(STATIC_IP[0], STATIC_IP[1], STATIC_IP[2], STATIC_IP[3]);
    IPAddress gateway(GATEWAY[0], GATEWAY[1], GATEWAY[2], GATEWAY[3]);
    IPAddress subnet(SUBNET[0], SUBNET[1], SUBNET[2], SUBNET[3]);
    bool result = WiFi.softAPConfig(staticIP, gateway, subnet);
    if (result == true)
    {
        result = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, 0, 4);
        if (result == true)
        {
            server.begin();

            TCPMessageTimer = xTimerCreate("TCPMessageTimer", pdMS_TO_TICKS(TCP_MESSAGE_TIMEOUT_MS), pdTRUE, (void *)0, TCPMessageTimerCallback);
            if (TCPMessageTimer == NULL)
            {
                SerialDebugPrint("Timer Creation Error");
                while (1)
                {
                    /* Error! */
                }
            }
        }
        else
        {
            SerialDebugPrint("softAP Error");
            while (1)
            {
                /* Error! */
            }
        }
        
    }
    else
    {
        SerialDebugPrint("softAPConfig Error");
        while (1)
        {
            /* Error! */
        }
    }
}

void TCPServerRouter::Deinit(void)
{
    if (initDone == false)
    {
        return;
    }
    
    StopTCPMessageTimeoutTimer();
    server.close();
    SetTCPConnectionState(WAITING_FOR_CLIENT);
    initDone = false;
}

void TCPServerRouter::Run(void)
{
    switch (GetTCPConnectionState())
    {
        case WAITING_FOR_CLIENT:
        {
            if(WaitingForClient() == true)
            {
                SerialDebugPrint("New client connected");
                SetTCPConnectionState(CLIENT_CONNECTED);
                ResetTCPMessageTimeoutTimer();
            }
            break;
        }
        case CLIENT_CONNECTED:
        {
            if (GetTCPMessageTimeout() == true)
            {
                SetTCPMessageTimeout(false);
                SerialDebugPrint("TCP message timeout!");
                client.stop();
                ResetTCPMessageTimeoutTimer();
                SetTCPConnectionState(WAITING_FOR_CLIENT);
            }
            else if (IsClientConnected() != true)
            {
                SerialDebugPrint("Client disconnected!");
                client.stop();
                ResetTCPMessageTimeoutTimer();
                SetTCPConnectionState(WAITING_FOR_CLIENT);
            }
            else if (TCPMessageBufferSize() > 0)
            {
                ResetTCPMessageTimeoutTimer();
                SetTCPConnectionState(PARSE_TCP_MESSAGE);
            }
            break;
        }
        case PARSE_TCP_MESSAGE:
        {
            TCPMessage_t readTCPMessage;
            if (CheckForProperTCPMessage(&readTCPMessage) == true)
            {
                SerialDebugPrint("Proper TCP Message Came!");
                SendMessageToMachine(readTCPMessage);
                SetTCPConnectionState(MACHINE_ACK_AWAITING);
            }
            break;
        }
        case MACHINE_ACK_AWAITING:
        {
            MachineResponseState_t response = WaitACKFromMachine();
            if (response == ACK_CAME)
            {
                SendACKToRemote();
                SetTCPConnectionState(CLIENT_CONNECTED);
            }
            else if (response == RESPONSE_TIMEOUT)
            {
                SetTCPConnectionState(CLIENT_CONNECTED);
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

TCPConnectionState_t TCPServerRouter::GetTCPConnectionState(void)
{
    return TCPConnectionState;
}

void TCPServerRouter::SetTCPConnectionState(TCPConnectionState_t state)
{
    TCPConnectionState = state;
}

bool TCPServerRouter::WaitingForClient(void)
{
    client = server.available();
    if (client)
    {
        return true;
    }
    return false;
}

bool TCPServerRouter::IsClientConnected(void)
{
    return client.connected();
}

int TCPServerRouter::TCPMessageBufferSize(void)
{ 
    return client.available();
}

bool TCPServerRouter::CheckForProperTCPMessage(TCPMessage_t* messagePointer)
{
    int readData = client.peek();
    int bufferSize = TCPMessageBufferSize();
    bool firstControlResult = false;
    if (readData == 'T' && bufferSize >= sizeof(TCPMessage_t))
    {
        firstControlResult = true;
    }
    else if (readData == 'T' && bufferSize < sizeof(TCPMessage_t))
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            bufferSize = TCPMessageBufferSize();
            if (bufferSize >= sizeof(TCPMessage_t))
            {
                firstControlResult = true;
            }
        }
        int discardData = client.read();
        SerialDebugPrint("Half Message! Discarded!");
        firstControlResult = false;
    }
    else
    {
        firstControlResult = false;
        int discardData = client.read();
        SerialDebugPrint("Wrong Header Byte! Discarded!");
    }

    if (firstControlResult == false)
    {
        return false;
    }
    else
    {
        client.readBytes((uint8_t *)messagePointer, sizeof(TCPMessage_t));

        /* Check the "TCP Message Header" */
        if (strncmp((const char *)messagePointer, TCP_MESSAGE_HEADER, 3) == 0)
        {
            /* Control the checksum of message */
            bool result = ControlChecksum((uint8_t *)messagePointer, sizeof(TCPMessage_t));
            if (result == true)
            {
                return true;
            }
        }
        int discardData = client.read();
        return false;
    }
}

void TCPServerRouter::SendACKToRemote(void)
{
    client.write(ACK_MESSAGE, 3);
}

void TCPServerRouter::SendMessageToMachine(TCPMessage_t message)
{
    MACHINE_SERIAL.write((uint8_t *)&message, sizeof(TCPMessage_t));
}

MachineResponseState_t TCPServerRouter::WaitACKFromMachine(void)
{
    bool ackCameFromMachine = false;
    static uint8_t ackWaitCounter = 0;
    
    if (MACHINE_SERIAL.available() > 0)
    {
        uint8_t firstByte = MACHINE_SERIAL.peek();
        if (firstByte == 'A' && MACHINE_SERIAL.available() >= 3)
        {
            ackWaitCounter = 0;
            uint8_t message[3] = {0};
            MACHINE_SERIAL.readBytes(message, 3);
            if (strncmp((const char *)message, ACK_MESSAGE, 3) == 0)
            {
                /* ACK Came from machine */
                ackCameFromMachine = true;
                return ACK_CAME;
            }
            else
            {
                /* Unknown 3 byte message came */
                SerialDebugPrint("Unknown 3 byte message came from machine");
                return UNKNOWN_MESSAGE;
            }
        }
        else if(firstByte == 'A' && MACHINE_SERIAL.available() < 3)
        {
            /* Wait max 50ms for ACK */
            vTaskDelay(10 / portTICK_PERIOD_MS);
            ackWaitCounter++;
            if (ackWaitCounter == 5)
            {
                SerialDebugPrint("ackWaitCounter timeout!");
                /* Clear serial buffer */
                for (uint8_t i = 0; i < MACHINE_SERIAL.available(); i++)
                {
                    MACHINE_SERIAL.read();
                }
                ackWaitCounter = 0;
                return RESPONSE_TIMEOUT;
            }
            return MISSING_MESSAGE;
        }
        else
        {
            /* Discard unknown header */
            MACHINE_SERIAL.read();
            return UNKNOWN_MESSAGE;
        }
    }
    else
    {
        /* Wait max 50ms for ACK */
        vTaskDelay(10 / portTICK_PERIOD_MS);
        ackWaitCounter++;
        if (ackWaitCounter == 5)
        {
            ackWaitCounter = 0;
            return RESPONSE_TIMEOUT;
        }
        return MISSING_MESSAGE;
    }
}

bool TCPServerRouter::GetTCPMessageTimeout(void)
{
    return TCPMessageTimeout;
}

void TCPServerRouter::SetTCPMessageTimeout(bool timeout)
{
    TCPMessageTimeout = timeout;
}

void TCPServerRouter::ResetTCPMessageTimeoutTimer(void)
{
    if (xTimerReset(TCPMessageTimer, 10) != pdPASS)
    {
        SerialDebugPrint("Failed to start timer!");
    }
}

void TCPServerRouter::StopTCPMessageTimeoutTimer(void)
{
    if (xTimerStop(TCPMessageTimer, 10) != pdPASS)
    {
        SerialDebugPrint("Failed to stop timer!");
    }
}

bool TCPServerRouter::ControlChecksum(uint8_t *dataPointer, uint8_t size)
{
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < size - 1; i++)
    {
        checksum ^= dataPointer[i];
    }
    checksum ^= 255;
    return (dataPointer[size - 1] == checksum);
}