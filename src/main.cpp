#include <Arduino.h>
#include <HardwareSerial.h>
#include <WiFi.h>

const char* ssid = "FiberHGW_ZT3Q34_2.4GHz";
const char* password = "phJJEy9HPd";
const char* ip = "192.168.1.39";
const uint16_t port = 3131;
const int TCPConnectTimeout = 2000;

WiFiClient client;

typedef enum
{
	CONNECTING_TO_WIFI,
	CONNECTING_TO_TCP_SERVER,
	CONNECTION_ESTABLISHED
}TCPConnectionState_t;
TCPConnectionState_t TCPConnectionState;

void setup()
{
	Serial.begin(9600);
	TCPConnectionState = CONNECTING_TO_WIFI;
}

void loop()
{
	switch (TCPConnectionState)
	{
		case CONNECTING_TO_WIFI:
		{
			wl_status_t wifiBeginStatus = WiFi.begin(ssid, password);
			while (WiFi.status() != WL_CONNECTED) 
			{
				delay(5000);
				Serial.println("Connecting to WiFi..");
			}
			Serial.print("Wifi Connected! SSID: ");
			Serial.print(ssid);
			Serial.print(" - Password: ");
			Serial.println(password);
			TCPConnectionState = CONNECTING_TO_TCP_SERVER;
			break;
		}
		case CONNECTING_TO_TCP_SERVER:
		{
			if(client.connect(ip, port, TCPConnectTimeout) == 0)
			{
				Serial.println("Client Connect Fail!");
				Serial.print("Error No: ");
				Serial.print(errno);
				Serial.print(" - Error Text: ");
				Serial.println(strerror(errno));
				if (WiFi.status() != WL_CONNECTED)
				{
					close(client.fd());
					client.stop();
					TCPConnectionState = CONNECTING_TO_WIFI;
				}
			}
			else
			{
				Serial.println("Client Connected! IP: ");
				Serial.print(ip);
				Serial.print(" - Port: ");
				Serial.println(port);
				TCPConnectionState = CONNECTION_ESTABLISHED;
			}
			break;
		}
		case CONNECTION_ESTABLISHED:
		{
			while (client.connected())
			{
				Serial.print("TCP Test!");
				delay(1000);
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
			break;
		}
		default:
		{
			break;
		}
	}

}
