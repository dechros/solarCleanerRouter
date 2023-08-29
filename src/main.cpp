#include <Arduino.h>
#include <HardwareSerial.h>
#include <WiFi.h>

#define DEBUG_PRINT 1

void SerialDebugPrint(const char *text);
void SerialDebugPrint(int number);

/*
const char *ssid = "FiberHGW_ZT3Q34_2.4GHz";
const char *password = "phJJEy9HPd";
const char *ip = "192.168.1.39";
const uint16_t port = 3131;
*/

const char *ssid = "QuaNet";
const char *password = "a987654999";
const char *ip = "192.168.0.200";
const uint16_t port = 1234;

const int TCPConnectTimeout = 1000;

const char *ACK_MESSAGE = "ACK";
const char *NAK_MESSAGE = "NAK";

const char *DISCONNECTED = "CONN0";
const char *CONNECTED = "CONN1";

const char RESET_COMMAND = 'R';
const char TCP_COMMAND = 'T';

static void FlushSerialReceiveBuffer();
static bool CalculateChecksum(uint8_t *dataPointer, uint8_t size);
void SerialMessageParser();
void SendConnectionStatusToRemote();
void SendACKToRemote(const char messageType);
void SendNAKToRemote(const char messageType);

void vSerialParserTask(void *pvParameters);
void vTCPConnectionTask(void *pvParameters);

TaskHandle_t xSerialParserTaskHandle = NULL;
TaskHandle_t xTCPConnectionTaskHandle = NULL;

SemaphoreHandle_t xSerialPrintSemaphore;
QueueHandle_t xTCPMessageQueue;

WiFiClient client;

typedef enum
{
	CONNECTING_TO_WIFI,
	CONNECTING_TO_TCP_SERVER,
	CONNECTION_ESTABLISHED
} TCPConnectionState_t;
TCPConnectionState_t TCPConnectionState;

typedef struct
{
	uint8_t headerT;
	uint8_t headerC;
	uint8_t headerP;
	uint8_t rightPalletAnalogValue;
	uint8_t leftPalletAnalogValue;
	uint8_t brushSpeedAnalogValue;
	union
	{
		uint8_t data;
		struct
		{
			uint8_t reserved:3;
			uint8_t emergencyStop:1;
			uint8_t waterPump:1;
			uint8_t brushWay:1;
			uint8_t leftPalletWay:1;
			uint8_t rightPalletWay:1;
		};
	};
	uint8_t reserved_2;
	uint8_t checksum;
}TCPMessage_t;

void setup()
{
	xSerialPrintSemaphore = xSemaphoreCreateMutex();
    xTCPMessageQueue = xQueueCreate(1, sizeof(TCPMessage_t));

	Serial.begin(9600);
	TCPConnectionState = CONNECTING_TO_WIFI;
	SendConnectionStatusToRemote();

	BaseType_t xReturned;
	xReturned = xTaskCreate(vSerialParserTask,
							"vSerialParserTask",
							2048,
							(void *)1,
							tskIDLE_PRIORITY,
							&xSerialParserTaskHandle);

	if (xReturned != pdPASS)
	{
		SerialDebugPrint("vSerialParserTask Creation Failed!");
		while (1)
		{
			/* Error */
		}
	}
	else
	{
		SerialDebugPrint("vSerialParserTask Created!");
	}
	xReturned = xTaskCreate(vTCPConnectionTask,
							"vTCPConnectionTask",
							2048,
							(void *)1,
							tskIDLE_PRIORITY,
							&xTCPConnectionTaskHandle);

	if (xReturned != pdPASS)
	{
		SerialDebugPrint("vTCPConnectionTask Creation Failed!");
		while (1)
		{
			/* Error */
		}
	}
	else
	{
		SerialDebugPrint("vTCPConnectionTask Created!");
	}

	/** 
	 *  vTaskStartScheduler() must not be used. 
	 *	It causes constant crash of the code with the error below:
	 *
	 *	"Guru Meditation Error: Core 1 panic'ed (LoadProhibited). Exception was unhandled."
	 */
}

void loop()
{
	/* Leave empty! */
}

static bool CalculateChecksum(uint8_t *dataPointer, uint8_t size)
{
	uint8_t checksum = 0;
	for (uint8_t i = 0; i < size - 1; i++)
	{
		checksum ^= dataPointer[i];
	}
	checksum ^= 255;
	return (checksum == dataPointer[size - 1]);
}

void SendConnectionStatusToRemote()
{
	if (TCPConnectionState == CONNECTION_ESTABLISHED)
	{
		xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);   
		Serial.print(CONNECTED);
		xSemaphoreGive(xSerialPrintSemaphore);
	}
	else
	{
		xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);   
		Serial.print(DISCONNECTED);
		xSemaphoreGive(xSerialPrintSemaphore);
	}
}

void SendACKToRemote(const char messageType)
{
	char array[4] = {0};
	memcpy(array, ACK_MESSAGE, 3);
	array[3] = messageType;

	xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);   
	Serial.print(array);
	xSemaphoreGive(xSerialPrintSemaphore);

}
void SendNAKToRemote(const char messageType)
{
	char array[4] = {0};
	memcpy(array, NAK_MESSAGE, 3);
	array[3] = messageType;

	xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);   
	Serial.print(array);
	xSemaphoreGive(xSerialPrintSemaphore);
}

void vTCPConnectionTask(void *pvParameters)
{
	configASSERT(((uint32_t)pvParameters) == 1);

	/* Assigning static IP because TP-Link AP does not have DHCP */
	IPAddress localIP(192, 168, 0, 201);
	IPAddress subnet(255, 255, 0, 0);

	/* Subnet and IP is enough */
	IPAddress primaryDNS(0, 0, 0, 0);
	IPAddress secondaryDNS(0, 0, 0, 0);
	IPAddress gateway(0, 0, 0, 0);

	if (!WiFi.config(localIP, gateway, subnet, primaryDNS, secondaryDNS)) 
	{
    	SerialDebugPrint("Failed to IP and subnet");
		while (1)
		{
			vTaskDelay(10000);
		}
 	}

	for (;;)
	{
		switch (TCPConnectionState)
		{
			case CONNECTING_TO_WIFI:
			{
				wl_status_t wifiBeginStatus = WiFi.begin(ssid, password);
				while (WiFi.status() != WL_CONNECTED)
				{
					delay(1000);
					SerialDebugPrint("Connecting to WiFi..");
				}
				SerialDebugPrint("Wifi Connected! SSID: ");
				SerialDebugPrint(ssid);
				SerialDebugPrint(" - Password: ");
				SerialDebugPrint(password);
				TCPConnectionState = CONNECTING_TO_TCP_SERVER;
				break;
			}
			case CONNECTING_TO_TCP_SERVER:
			{
				if (client.connect(ip, port, TCPConnectTimeout) == 0)
				{
					SerialDebugPrint("Client Connect Fail!");
					SerialDebugPrint("Error No: ");
					SerialDebugPrint(errno);
					SerialDebugPrint(" - Error Text: ");
					SerialDebugPrint(strerror(errno));
					if (WiFi.status() != WL_CONNECTED)
					{
						close(client.fd());
						client.stop();
						TCPConnectionState = CONNECTING_TO_WIFI;
					}
				}
				else
				{
					SerialDebugPrint("Client Connected! IP: ");
					SerialDebugPrint(ip);
					SerialDebugPrint(" - Port: ");
					SerialDebugPrint(port);
					TCPConnectionState = CONNECTION_ESTABLISHED;
					SendConnectionStatusToRemote();
				}
				break;
			}
			case CONNECTION_ESTABLISHED:
			{
				TCPMessage_t TCPMessage;
				while (client.connected())
				{
					if(xQueueReceive(xTCPMessageQueue, &TCPMessage, (TickType_t)10) == pdPASS)
					{
						client.write((char*)&TCPMessage, sizeof(TCPMessage_t));
					}
				}

				if (WiFi.status() != WL_CONNECTED)
				{
					close(client.fd());
					client.stop();
					TCPConnectionState = CONNECTING_TO_WIFI;
				}
				else
				{
					TCPConnectionState = CONNECTING_TO_TCP_SERVER;
				}
				SendConnectionStatusToRemote();
				break;
			}
			default:
			{
				break;
			}
		}
		vTaskDelay(1);
	}
}

void vSerialParserTask(void *pvParameters)
{
	configASSERT(((uint32_t)pvParameters) == 1);

	for (;;)
	{
		static int bufferClearCounter = 0;
		xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);   
		int size = Serial.available();
		xSemaphoreGive(xSerialPrintSemaphore);
		if (size > 0)
		{
			xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);   
			uint8_t readByte = Serial.peek();
			xSemaphoreGive(xSerialPrintSemaphore);
			if (readByte == 'R' && size >= 5)
			{
				bufferClearCounter = 0;
				uint8_t readArray[5] = {0};
				xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);
				for (uint8_t i = 0; i < 5; i++)
				{
					readArray[i] = Serial.read();
				}
				xSemaphoreGive(xSerialPrintSemaphore);
				if (strncmp((char *)readArray, "RESET", 5) == 0)
				{
					SerialDebugPrint("Reset Command Came!");
					WiFi.disconnect();
					client.flush();
					close(client.fd());
					client.stop();
					TCPConnectionState = CONNECTING_TO_WIFI;
					SendConnectionStatusToRemote();
				}
				else
				{
					SerialDebugPrint("Reset Command Discarded!");
					SendNAKToRemote(RESET_COMMAND);
				}
			}
			else if (readByte == 'T' && size >= 9)
			{
				bufferClearCounter = 0;
				uint8_t readArray[9] = {0};
				xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);
				for (uint8_t i = 0; i < 9; i++)
				{
					readArray[i] = Serial.read();
				}
				xSemaphoreGive(xSerialPrintSemaphore);
				if ((strncmp((char *)readArray, "TCP", 3) == 0) && (CalculateChecksum(readArray, 9) == true))
				{
					SerialDebugPrint("TCP Command Came!");
					SendACKToRemote(TCP_COMMAND);
					TCPMessage_t TCPMessage;
					memcpy(&TCPMessage, readArray, sizeof(TCPMessage_t));
					if(xQueueSendToBack(xTCPMessageQueue, (void*)&TCPMessage, (TickType_t)10) != pdPASS)
					{
						SerialDebugPrint("TCP Queue is full!");
					}
				}
				else
				{
					SerialDebugPrint("TCP Command Discarded!");
					SendNAKToRemote(TCP_COMMAND);
				}
			}
			else if (readByte == 'S' && size >= 4)
			{
				bufferClearCounter = 0;
				uint8_t readArray[4] = {0};
				xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);
				for (uint8_t i = 0; i < 4; i++)
				{
					readArray[i] = Serial.read();
				}
				xSemaphoreGive(xSerialPrintSemaphore);
				if (strncmp((char *)readArray, "STAT", 4) == 0)
				{
					SerialDebugPrint("STAT Command Came!");
					SendConnectionStatusToRemote();
				}
				else
				{
					SerialDebugPrint("STAT Command Discarded!");
					SendConnectionStatusToRemote();
				}
			}
			else if ((readByte == 'R' && size < 5) || (readByte == 'T' && size < 9) || (readByte == 'S' && size < 4))
			{
				bufferClearCounter++;
				if (bufferClearCounter == 1000)
				{
					bufferClearCounter = 0;
					/* Discard one byte */
					SerialDebugPrint("Command size timeout!");
					if (readByte == 'R')
					{
						SendNAKToRemote(RESET_COMMAND);
					}
					else if (readByte == 'T')
					{
						SendNAKToRemote(TCP_COMMAND);
					}
					xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);
					char discardByte = Serial.read();
					xSemaphoreGive(xSerialPrintSemaphore);
				}
			}
			else
			{
				/* Discard the byte that is not a header */
				SerialDebugPrint("Non-header byte discarded");
				bufferClearCounter = 0;
				xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);
				char discardByte = Serial.read();
				xSemaphoreGive(xSerialPrintSemaphore);
			}
		}
		vTaskDelay(1);
	}
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
