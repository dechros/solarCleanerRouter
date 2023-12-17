#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define WEB_SITE                "www.dsfabrik.com"

#define MACHINE_SERIAL Serial2

#define AP_SSID                 "ESP32-AP"
#define AP_PASSWORD             "88888888"

#define TCP_MESSAGE_TIMEOUT_MS  1000

#define TCP_SERVER_LISTEN_PORT  3131
const int STATIC_IP[4]       = {192, 168, 31, 2};
const int GATEWAY[4]         = {192, 168, 31, 1};
const int SUBNET[4]          = {255, 255, 255, 0};

#define MAINTENANCE_SSID        "ESP_OTA"
#define MAINTENANCE_PASSWORD    "88888888"

#endif /* CONFIG_H */