/**
 * @file api.cpp
 * @author Halit Cetin (halitcetin@live.com)
 * @brief WEB API communication file
 * @version 0.1
 * @date 2023-12-21
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "api.h"

RequestApiState_t SetMachineData(const char *apiUrl, const Parameters_t parameters)
{
    RequestApiState_t retVal = FAIL_REQ;
    HTTPClient http;

    String fullApiUrl = String(apiUrl) + "/robot/api/setMachine.php";

    String columns = "machineIp,errLeftCnt,errRightCnt,errBrushCnt,errControllerCnt,prmLeftRampUp,prmLeftRampDown,prmLeftMinSpeed,prmLeftMaxSpeed,prmRightRampUp,prmRightRampDown,prmRightMinSpeed,prmRightMaxSpeed,prmBrushRampUp,prmBrushRampDown,prmBrushMinSpeed,prmBrushMaxSpeed,prmJyMiddleVal,prmJyDeadZone,prmJoyMinVal,prmJoyMaxVal,prmPotMinVal,prmPotMaxVal";
    String values = String(parameters.machineIP[0]) + "." + String(parameters.machineIP[1]) + "." + String(parameters.machineIP[2]) + "." + String(parameters.machineIP[3]) + "," +
                    String(parameters.leftErrorCount) + "," +
                    String(parameters.rightErrorCount) + "," +
                    String(parameters.brushErrorCount) + "," +
                    String(parameters.controllerErrorCount) + "," +
                    String(parameters.leftRampUp) + "," +
                    String(parameters.leftRampDown) + "," +
                    String(parameters.leftMinSpeed) + "," +
                    String(parameters.leftMaxSpeed) + "," +
                    String(parameters.rightRampUp) + "," +
                    String(parameters.rightRampDown) + "," +
                    String(parameters.rightMinSpeed) + "," +
                    String(parameters.rightMaxSpeed) + "," +
                    String(parameters.brushRampUp) + "," +
                    String(parameters.brushRampDown) + "," +
                    String(parameters.brushMinSpeed) + "," +
                    String(parameters.brushMaxSpeed) + "," +
                    String(parameters.joystickMiddleValue) + "," +
                    String(parameters.joystickDeadZone) + "," +
                    String(parameters.joystickMinValue) + "," +
                    String(parameters.joystickMaxValue) + "," +
                    String(parameters.potantiometerMinValue) + "," +
                    String(parameters.potantiometerMaxValue);

    String postData = "machineName=" + String((char *)parameters.companyName) +
                      "&columns=" + columns +
                      "&values=" + values;

    http.begin(fullApiUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);
    Serial.println("Set HTTP Post: " + postData);

    if (httpResponseCode == HTTP_CODE_OK)
    {
        String response = http.getString();
        Serial.println("Set HTTP Response: " + response);

        if (IsResponseFalse(response) == false)
        {
            retVal = SUCCESS_REQ;
        }
    }
    else
    {
        Serial.println("HTTP Request failed. Error code: " + String(httpResponseCode));
    }

    http.end();

    return retVal;
}

RequestApiState_t CreateMachine(const char *apiUrl, const Parameters_t parameters)
{
    RequestApiState_t retVal = FAIL_REQ;
    HTTPClient http;

    String fullApiUrl = String(apiUrl) + "/robot/api/createMachine.php";
    String postData = "machineName=" + String((char *)parameters.companyName) +
                      "&machineIp=" + String(parameters.machineIP[0]) + "." + String(parameters.machineIP[1]) +
                      "." + String(parameters.machineIP[2]) + "." + String(parameters.machineIP[3]) +
                      "&errLeftCnt=" + String(parameters.leftErrorCount) +
                      "&errRightCnt=" + String(parameters.rightErrorCount) +
                      "&errBrushCnt=" + String(parameters.brushErrorCount) +
                      "&errControllerCnt=" + String(parameters.controllerErrorCount) +
                      "&prmLeftRampUp=" + String(parameters.leftRampUp) +
                      "&prmLeftRampDown=" + String(parameters.leftRampDown) +
                      "&prmLeftMinSpeed=" + String(parameters.leftMinSpeed) +
                      "&prmLeftMaxSpeed=" + String(parameters.leftMaxSpeed) +
                      "&prmRightRampUp=" + String(parameters.rightRampUp) +
                      "&prmRightRampDown=" + String(parameters.rightRampDown) +
                      "&prmRightMinSpeed=" + String(parameters.rightMinSpeed) +
                      "&prmRightMaxSpeed=" + String(parameters.rightMaxSpeed) +
                      "&prmBrushRampUp=" + String(parameters.brushRampUp) +
                      "&prmBrushRampDown=" + String(parameters.brushRampDown) +
                      "&prmBrushMinSpeed=" + String(parameters.brushMinSpeed) +
                      "&prmBrushMaxSpeed=" + String(parameters.brushMaxSpeed) +
                      "&prmJyMiddleVal=" + String(parameters.joystickMiddleValue) +
                      "&prmJyDeadZone=" + String(parameters.joystickDeadZone) +
                      "&prmJoyMinVal=" + String(parameters.joystickMinValue) +
                      "&prmJoyMaxVal=" + String(parameters.joystickMaxValue) +
                      "&prmPotMinVal=" + String(parameters.potantiometerMinValue) +
                      "&prmPotMaxVal=" + String(parameters.potantiometerMaxValue);

    http.begin(fullApiUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);
    Serial.println("Create HTTP Post: " + postData);

    if (httpResponseCode == HTTP_CODE_OK)
    {
        String response = http.getString();
        Serial.println("Create HTTP Response: " + response);

        if (IsResponseFalse(response) == false)
        {
            retVal = SUCCESS_REQ;
        }
    }
    else
    {
        Serial.println("HTTP Request failed. Error code: " + String(httpResponseCode));
    }

    http.end();

    return retVal;
}

RequestApiState_t IsMachinePresentInDB(const char *apiUrl, const Parameters_t parameters)
{
    RequestApiState_t retVal = FALSE_REQ;
    HTTPClient http;

    String fullApiUrl = String(apiUrl) + "/robot/api/getMachine.php";
    String postData = "";

    http.begin(fullApiUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);
    Serial.println("Check HTTP Post: " + postData);

    if (httpResponseCode == HTTP_CODE_OK)
    {
        String response = http.getString();
        Serial.println("Machine Check HTTP Response: " + response);

        if (IsResponseFalse(response) == false)
        {
            String targetString = "\"machineName\":\"" + String((char *)parameters.companyName) + "\"";
            if (response.indexOf(targetString) != -1)
            {
                retVal = TRUE_REQ;
            }
        }
    }
    else
    {
        Serial.println("HTTP Request failed. Error code: " + String(httpResponseCode));
        retVal = FAIL_REQ;
    }

    http.end();

    return retVal;
}

RequestApiState_t FetchMachineData(const char *apiUrl, Parameters_t &parameters)
{
    RequestApiState_t retVal = FAIL_REQ;
    HTTPClient http;

    String fullApiUrl = String(apiUrl) + "/robot/api/getMachine.php";
    String postData = "machineName=" + String((char *)parameters.companyName);

    http.begin(fullApiUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);
    Serial.println("Fetch HTTP Post: " + postData);

    if (httpResponseCode == HTTP_CODE_OK)
    {
        String response = http.getString();
        Serial.println("Fetch HTTP Response: " + response);

        if (IsResponseFalse(response) == false)
        {
            String companyName = GetJsonValue(response, "machineName");
            strncpy((char *)parameters.companyName, companyName.c_str(), sizeof(parameters.companyName));

            std::vector<int> ipVec = SplitIpString(GetJsonValue(response, "machineIp"));
            parameters.machineIP[0] = ipVec[0];
            parameters.machineIP[1] = ipVec[1];
            parameters.machineIP[2] = ipVec[2];
            parameters.machineIP[3] = ipVec[3];

            parameters.leftErrorCount = GetJsonValue(response, "errLeftCnt").toInt();
            parameters.rightErrorCount = GetJsonValue(response, "errRightCnt").toInt();
            parameters.brushErrorCount = GetJsonValue(response, "errBrushCnt").toInt();
            parameters.controllerErrorCount = GetJsonValue(response, "errControllerCnt").toInt();
            parameters.leftRampUp = GetJsonValue(response, "prmLeftRampUp").toInt();
            parameters.leftRampDown = GetJsonValue(response, "prmLeftRampDown").toInt();
            parameters.leftMinSpeed = GetJsonValue(response, "prmLeftMinSpeed").toInt();
            parameters.leftMaxSpeed = GetJsonValue(response, "prmLeftMaxSpeed").toInt();
            parameters.rightRampUp = GetJsonValue(response, "prmRightRampUp").toInt();
            parameters.rightRampDown = GetJsonValue(response, "prmRightRampDown").toInt();
            parameters.rightMinSpeed = GetJsonValue(response, "prmRightMinSpeed").toInt();
            parameters.rightMaxSpeed = GetJsonValue(response, "prmRightMaxSpeed").toInt();
            parameters.brushRampUp = GetJsonValue(response, "prmBrushRampUp").toInt();
            parameters.brushRampDown = GetJsonValue(response, "prmBrushRampDown").toInt();
            parameters.brushMinSpeed = GetJsonValue(response, "prmBrushMinSpeed").toInt();
            parameters.brushMaxSpeed = GetJsonValue(response, "prmBrushMaxSpeed").toInt();
            parameters.joystickMiddleValue = GetJsonValue(response, "prmJyMiddleVal").toInt();
            parameters.joystickDeadZone = GetJsonValue(response, "prmJyDeadZone").toInt();
            parameters.joystickMinValue = GetJsonValue(response, "prmJoyMinVal").toInt();
            parameters.joystickMaxValue = GetJsonValue(response, "prmJoyMaxVal").toInt();
            parameters.potantiometerMinValue = GetJsonValue(response, "prmPotMinVal").toInt();
            parameters.potantiometerMaxValue = GetJsonValue(response, "prmPotMaxVal").toInt();

            retVal = SUCCESS_REQ;
        }
    }
    else
    {
        Serial.println("HTTP Request failed. Error code: " + String(httpResponseCode));
    }

    http.end();

    return retVal;
}

std::vector<int> SplitIpString(const String &ipString)
{
    std::vector<int> ipParts;
    int startPos = 0;

    for (int i = 0; i < 4; ++i)
    {
        int dotPos = ipString.indexOf('.', startPos);
        if (dotPos == -1)
        {
            dotPos = ipString.length();
        }

        String part = ipString.substring(startPos, dotPos);
        ipParts.push_back(part.toInt());

        startPos = dotPos + 1;
    }

    return ipParts;
}

String GetJsonValue(const String &json, const String &key)
{
    int keyIndex = json.indexOf('\"' + key + '\"');
    if (keyIndex == -1)
    {
        Serial.println("Key not found");
        return "";
    }

    int valueIndex = json.indexOf(':', keyIndex);
    if (valueIndex == -1)
    {
        Serial.println("Value not found");
        return "";
    }

    int startIndex = json.indexOf('\"', valueIndex);
    if (startIndex == -1)
    {
        Serial.println("Start double quote for value not found");
        return "";
    }

    int endIndex = json.indexOf('\"', startIndex + 1);
    if (endIndex == -1)
    {
        Serial.println("End double quote for value not found");
        return "";
    }

    return json.substring(startIndex + 1, endIndex);
}

bool IsResponseFalse(const String &response)
{
    if (!response.isEmpty() && (response[0] == ' ' || response[0] == '\0'))
    {
        return true;
    }

    if (response.length() <= 20)
    {
        return true;
    }

    String lowercaseResponse = response;
    lowercaseResponse.toLowerCase();
    return lowercaseResponse.indexOf("false") != -1; 
}
