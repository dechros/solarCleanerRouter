/**
 * @file api.h
 * @author Halit Cetin (halitcetin@live.com)
 * @brief WEB API communication header file
 * @version 0.1
 * @date 2023-12-21
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef API_H
#define API_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <vector>
#include <sstream>

typedef struct /* 84 byte */
{
    uint8_t controlValue;
    uint8_t versionMajor;
    uint8_t versionMinor;
    uint8_t versionPatch;
    uint8_t companyName[50];
    uint8_t machineIP[4];
    uint16_t leftErrorCount;
    uint16_t rightErrorCount;
    uint16_t brushErrorCount;
    uint16_t controllerErrorCount;
    uint8_t leftRampUp;
    uint8_t leftRampDown;
    uint8_t leftMinSpeed;
    uint8_t leftMaxSpeed;
    uint8_t rightRampUp;
    uint8_t rightRampDown;
    uint8_t rightMinSpeed;
    uint8_t rightMaxSpeed;
    uint8_t brushRampUp;
    uint8_t brushRampDown;
    uint8_t brushMinSpeed;
    uint8_t brushMaxSpeed;
    uint8_t joystickMiddleValue;
    uint8_t joystickDeadZone;
    uint8_t joystickMinValue;
    uint8_t joystickMaxValue;
    uint8_t potantiometerMinValue;
    uint8_t potantiometerMaxValue;
} Parameters_t;

typedef enum
{
    FAIL_REQ,
    SUCCESS_REQ,
    TRUE_REQ,
    FALSE_REQ
} RequestApiState_t;

bool IsResponseFalse(const String &response);
RequestApiState_t SetMachineData(const char *apiUrl, const Parameters_t parameters);
RequestApiState_t CreateMachine(const char *apiUrl, const Parameters_t parameters);
RequestApiState_t FetchMachineData(const char *apiUrl, Parameters_t &parameters);
RequestApiState_t IsMachinePresentInDB(const char *apiUrl, const Parameters_t parameters);
String GetJsonValue(const String &json, const String &key);
std::vector<int> SplitIpString(const String &ipString);

#endif