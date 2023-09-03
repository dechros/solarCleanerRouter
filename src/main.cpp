#include <Arduino.h>
#include <WiFi.h>

const char *ssid = "ESP32-AP";
const char *password = "88888888";
IPAddress staticIP(192, 168, 31, 2);
IPAddress gateway(192, 168, 31, 1);
IPAddress subnet(255, 255, 255, 0);
const int serverPort = 3131;

WiFiServer server(serverPort);
WiFiClient client;

typedef enum
{
    WAITING_FOR_CLIENT,
    CLIENT_CONNECTED,
} TCPConnectionState_t;
TCPConnectionState_t TCPConnectionState;

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
    uint8_t batteryVoltage;
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
    union
	{
		uint8_t leds;
		struct
		{
			uint8_t warningLedRed   :1;
            uint8_t warningLedGreen :1;
            uint8_t batteryLedRed   :1;
            uint8_t batteryLedGreen :1;
            uint8_t                 :4;
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
    Serial.begin(9600);
    WiFi.softAPConfig(staticIP, gateway, subnet);
    WiFi.softAP(ssid, password);
    server.begin();
    TCPMessageTimer = xTimerCreate("TCPMessageTimer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, TCPMessageTimerCallback);
    if (TCPMessageTimer == NULL)
    {
        Serial.println("Timer Creation Error!");
        while(1)
        {
            /* Error! */
        }
    }
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
                Serial.println("New client connected");
                TCPConnectionState = CLIENT_CONNECTED;
            }
            break;
        }
        case CLIENT_CONNECTED:
        {
            if (xTimerReset(TCPMessageTimer, 10) != pdPASS) 
            {
                Serial.println("Failed to start timer!");
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
                        Serial.println("Timer Reset Failed!");
                    }
                    bufferClearCounter = 0;
                    int readData = client.peek();
                    if (readData == 'T' && tcpBufferSize >= sizeof(TCPMessage_t))
                    {
                        //Serial.println("Header Byte and size are correct!");
                        TCPMessage_t readTCPMessage;
                        client.readBytes((uint8_t*)&readTCPMessage, sizeof(TCPMessage_t));
                        if (readTCPMessage.headerT == 'T' && readTCPMessage.headerC == 'C' && readTCPMessage.headerP == 'P' )
                        {
                            //Serial.println("Message is correct!");
                            Serial.write((uint8_t*)&readTCPMessage, sizeof(TCPMessage_t));
                            uint8_t ackArray[3] = {'A', 'C', 'K'};
                            client.write(ackArray, 3);
                        }
                        else
                        {
                            int discardData = client.read();
                            Serial.println("Unkown Header !");
                        }
                    }
                    else if (readData == 'T' && tcpBufferSize < sizeof(TCPMessage_t))
                    {
                        delay(10);
                        bufferClearCounter++;
                        if (bufferClearCounter == 5)
                        {
                            bufferClearCounter = 0;
                            int discardData = client.read();
                            Serial.println("Half Message! Discarded!");
                        }
                    }
                    else
                    {
                        int discardData = client.read();
                        Serial.println("Unkown Header!");
                    }
                }
            }
            client.stop();
            if(xTimerStop(TCPMessageTimer, 10) != pdPASS)
            {
                Serial.println("Timer Stop Failed!");
            }
            Serial.println("Client disconnected");
            TCPConnectionState = WAITING_FOR_CLIENT;
            break;
        }
        default:
        {
            break;
        }
    }
}
