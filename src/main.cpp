#include <Arduino.h>
#include <WiFi.h>

#define DEBUG_PRINT         1
#define MACHINE_SERIAL      Serial2
#define ACK_MESSAGE         "ACK"
#define TCP_MESSAGE_HEADER  "TCP"
#define SSID                "ESP32-AP"
#define PASSWORD            "88888888"
#define PORT                3131


bool ControlChecksum(uint8_t *dataPointer, uint8_t size);
void SerialDebugPrint(const char *text);
void SerialDebugPrint(int number);

IPAddress staticIP(192, 168, 31, 2);
IPAddress gateway(192, 168, 31, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(PORT);
WiFiClient client;

typedef enum
{
    WAITING_FOR_CLIENT,
    CLIENT_CONNECTED,
} TCPConnectionState_t;
TCPConnectionState_t TCPConnectionState;

SemaphoreHandle_t xSerialPrintSemaphore;
TimerHandle_t TCPMessageTimer;
bool TCPMessageTimeout = false;

typedef struct
{
	uint8_t headerT;
	uint8_t headerC;
	uint8_t headerP;
	uint8_t joystickX;
	uint8_t joystickY;
    uint8_t driveSpeed;
    uint8_t brushSpeed;
	union
	{
		uint8_t buttons;
		struct
		{
			uint8_t waterButton     :1;
            uint8_t lampButton      :1;
			uint8_t startButton     :1;
			uint8_t passButton      :1;
			uint8_t brushFrontCW    :1;
			uint8_t brushFrontCCW   :1;
			uint8_t brushRearCW     :1;
            uint8_t brushRearCCW    :1;
		};
	};
    union
	{
		uint8_t data;
		struct
		{
			uint8_t emergencyButton :1;
            uint8_t                 :7;
		};
	};
	uint8_t checksum;
}TCPMessage_t;

void TCPMessageTimerCallback(TimerHandle_t xTimer)
{
    TCPMessageTimeout = true;
}

void setup()
{
    TCPConnectionState = WAITING_FOR_CLIENT;
    xSerialPrintSemaphore = xSemaphoreCreateMutex();
    Serial.begin(115200);
    MACHINE_SERIAL.begin(9600);
    WiFi.softAPConfig(staticIP, gateway, subnet);
    WiFi.softAP(SSID, PASSWORD, 1, 1, 4);
    server.begin();
    TCPMessageTimer = xTimerCreate("TCPMessageTimer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, TCPMessageTimerCallback);
    if (TCPMessageTimer == NULL)
    {
        SerialDebugPrint("Timer Creation Error");
        while(1)
        {
            /* Error! */
        }
    }
    SerialDebugPrint("Setup Complete!");
}

void loop()
{
    switch (TCPConnectionState)
    {
        case WAITING_FOR_CLIENT:
        {
            client = server.available();
            if (client)
            {
                SerialDebugPrint("New client connected");
                TCPConnectionState = CLIENT_CONNECTED;
            }
            break;
        }
        case CLIENT_CONNECTED:
        {
            if (xTimerReset(TCPMessageTimer, 10) != pdPASS) 
            {
                SerialDebugPrint("Failed to start timer!");
            }
            uint8_t bufferClearCounter = 0;
            while (client.connected())
            {
                if (TCPMessageTimeout)
                {
                    TCPMessageTimeout = false;
                    break;
                }
                
                int tcpBufferSize = client.available();
                if (tcpBufferSize > 0)
                {
                    if(xTimerReset(TCPMessageTimer, 10) != pdPASS)
                    {
                        SerialDebugPrint("Timer Reset Failed!");
                    }
                    bufferClearCounter = 0;
                    int readData = client.peek();
                    if (readData == 'T' && tcpBufferSize >= sizeof(TCPMessage_t))
                    {
                        TCPMessage_t readTCPMessage;
                        client.readBytes((uint8_t*)&readTCPMessage, sizeof(TCPMessage_t));
                        /* Check the "TCP Message Header" */
                        if (strncmp((const char*)&readTCPMessage, TCP_MESSAGE_HEADER, 3) == 0)
                        {
                            /* Control the checksum of message */
                            bool result = ControlChecksum((uint8_t*)&readTCPMessage, sizeof(TCPMessage_t));
                            if (result == true)
                            {
                                MACHINE_SERIAL.write((uint8_t*)&readTCPMessage, sizeof(TCPMessage_t));
                                bool ackCameFromMachine = false;
                                uint8_t ackWaitCounter = 0;
                                while (1)
                                {
                                    if (MACHINE_SERIAL.available() >= 3)
                                    {
                                        ackWaitCounter = 0;
                                        uint8_t message[3] = {0};
                                        MACHINE_SERIAL.readBytes(message, 3);
                                        if (strncmp((const char*)message, ACK_MESSAGE, 3) == 0)
                                        {
                                            /* ACK Came from machine */
                                            ackCameFromMachine = true;
                                            break;
                                        }
                                        else
                                        {
                                            /* Unknown 3 byte message came */
                                            SerialDebugPrint("Unknown 3 byte message came from machine");
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        /* Wait max 20ms for ACK */
                                        vTaskDelay(5/portTICK_PERIOD_MS);
                                        ackWaitCounter++;
                                        if (ackWaitCounter == 4)
                                        {
                                            /* Clear Serial receive buffer in case of half message */
                                            SerialDebugPrint("ackWaitCounter timeout!");
                                            int bufferSize = MACHINE_SERIAL.available();
                                            for (uint8_t i = 0; i < bufferSize; i++)
                                            {
                                                int discardByte = MACHINE_SERIAL.read();
                                            }
                                            ackWaitCounter = 0;
                                            ackCameFromMachine = false;
                                            break;
                                        }
                                    }
                                }
                                if (ackCameFromMachine == true)
                                {
                                    /* Send ACK to remote */
                                    client.write(ACK_MESSAGE, 3);
                                }
                            }
                            else
                            {
                                SerialDebugPrint("Checksum Error!");
                            }
                        }
                        else
                        {
                            SerialDebugPrint("Unkown Header !");
                        }
                    }
                    else if (readData == 'T' && tcpBufferSize < sizeof(TCPMessage_t))
                    {
                        vTaskDelay(10 / portTICK_PERIOD_MS);
                        bufferClearCounter++;
                        if (bufferClearCounter == 5)
                        {
                            bufferClearCounter = 0;
                            int discardData = client.read();
                            SerialDebugPrint("Half Message! Discarded!");
                        }
                    }
                    else
                    {
                        int discardData = client.read();
                        SerialDebugPrint("Unkown Header!");
                    }
                }
            }
            client.stop();
            if(xTimerStop(TCPMessageTimer, 10) != pdPASS)
            {
                SerialDebugPrint("Timer Stop Failed!");
            }
            SerialDebugPrint("Client disconnected");
            TCPConnectionState = WAITING_FOR_CLIENT;
            break;
        }
        default:
        {
            break;
        }
    }
}

bool ControlChecksum(uint8_t *dataPointer, uint8_t size)
{
	uint8_t checksum = 0;
	for (uint8_t i = 0; i < size - 1; i++)
	{
		checksum ^= dataPointer[i];
	}
	checksum ^= 255;
    return (dataPointer[size - 1] == checksum);
}

void SerialDebugPrint(const char *text)
{
#if DEBUG_PRINT
	xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);   
	Serial.println(text);
	xSemaphoreGive(xSerialPrintSemaphore);
#endif
}

void SerialDebugPrint(int number)
{
#if DEBUG_PRINT
	xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);   
	Serial.println(number);
	xSemaphoreGive(xSerialPrintSemaphore);
#endif
}
