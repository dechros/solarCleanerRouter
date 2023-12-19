#include <WiFi.h>
#include <HTTPClient.h>
#include <vector>
#include <sstream>
#include <ArduinoJson.h>
#include "Maintenance.h"
#include "config.h"
#include "globals.h"
#include "serialDebugPrint.h"

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
    WiFi.mode(WIFI_STA);
}

void Maintenance::Deinit(void)
{
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
    switch (GetMaintenanceState())
    {
    case WIFI_CONNECTION_STATE:
    {
        WiFi.begin(MAINTENANCE_SSID, MAINTENANCE_PASSWORD);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        if (WiFi.status() == WL_CONNECTED)
        {
            SetMaintenanceState(CHECK_WEBSITE_STATE);
        }
        else
        {
            SerialDebugPrint("Cannot connect to WiFi.");
        }
        break;
    }
    case CHECK_WEBSITE_STATE:
    {
        IPAddress ip;
        if (WiFi.hostByName(WEB_SITE, ip) == true)
        {
            SetMaintenanceState(CONNECTION_ESTABLISHED);
        }
        else if (WiFi.status() != WL_CONNECTED)
        {
            SerialDebugPrint("WiFi connection is lost.");
            SetMaintenanceState(WIFI_CONNECTION_STATE);
        }
        else
        {
            SerialDebugPrint("Website is unreachable.");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
        break;
    }
    case CONNECTION_ESTABLISHED:
    {
        SerialDebugPrint("Get Parameters State");
        static bool updateDB = false;
        Parameters_t testMachine = RequestParametersFromMachine();
        RequestApiState_t isMachineInDB = IsMachinePresentInDB(WEB_SITE, (char *)testMachine.companyName);

        if (isMachineInDB == FALSE_REQ)
        {
            Serial.println("Machine is created.");
            CreateMachine(WEB_SITE, testMachine);
        }
        else if (isMachineInDB == TRUE_REQ)
        {
            Serial.println("Machine is alredy present.");
            FetchMachineData((char *)testMachine.companyName, WEB_SITE, testMachine);
        }
        else if (isMachineInDB == FAIL_REQ)
        {
            Serial.println("API Communication error.");
        }
        else
        {
            Serial.println("Unimplemented excepion.");
        }

        vTaskDelay(3000 / portTICK_PERIOD_MS);
        if (WiFi.status() != WL_CONNECTED)
        {
            SerialDebugPrint("WiFi connection is lost.");
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

Parameters_t Maintenance::RequestParametersFromMachine(void)
{
    Parameters_t testMachine = {0};
    char *testName = "Test Api 3";
    char *testIP = "192.168.31.31";

    strncpy((char *)testMachine.companyName, testName, strlen(testName));

    std::vector<int> ipVec = SplitIpString(testIP);
    testMachine.machineIP[0] = ipVec[0];
    testMachine.machineIP[1] = ipVec[1];
    testMachine.machineIP[2] = ipVec[2];
    testMachine.machineIP[3] = ipVec[3];

    testMachine.leftErrorCount = 3;
    testMachine.rightErrorCount = 2;
    testMachine.brushErrorCount = 1;
    testMachine.controllerErrorCount = 0;

    testMachine.leftRampUp = 6;
    testMachine.leftRampDown = 4;
    testMachine.rightRampUp = 7;
    testMachine.rightRampDown = 3;
    testMachine.brushRampUp = 5;
    testMachine.brushRampDown = 2;

    testMachine.leftMinSpeed = 1;
    testMachine.leftMaxSpeed = 8;
    testMachine.rightMinSpeed = 2;
    testMachine.rightMaxSpeed = 9;
    testMachine.brushMinSpeed = 3;
    testMachine.brushMaxSpeed = 10;

    testMachine.joystickMiddleValue = 127;
    testMachine.joystickDeadZone = 5;
    testMachine.joystickMinValue = 0;
    testMachine.joystickMaxValue = 255;
    testMachine.potantiometerMinValue = 20;
    testMachine.potantiometerMaxValue = 230;

    return testMachine;
}

MaintenanceState_t Maintenance::GetMaintenanceState(void)
{
    return maintenanceState;
}

void Maintenance::SetMaintenanceState(MaintenanceState_t state)
{
    maintenanceState = state;
}

RequestApiState_t Maintenance::CreateMachine(const char *apiUrl, Parameters_t &parameters)
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

    if (httpResponseCode == HTTP_CODE_OK)
    {
        String response = http.getString();
        Serial.println("HTTP Response: " + response);
        retVal = SUCCESS_REQ;
    }
    else
    {
        Serial.println("HTTP Request failed. Error code: " + String(httpResponseCode));
    }

    http.end();

    return retVal;
}

RequestApiState_t Maintenance::IsMachinePresentInDB(const char *apiUrl, const char *targetMachineName)
{
    RequestApiState_t retVal = FALSE_REQ;
    HTTPClient http;

    String fullApiUrl = String(apiUrl) + "/robot/api/getMachine.php";
    String postData = "";

    http.begin(fullApiUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);

    if (httpResponseCode == HTTP_CODE_OK)
    {
        String response = http.getString();
        String targetString = "\"machineName\":\"" + String(targetMachineName) + "\"";
        if (response.indexOf(targetString) != -1)
        {
            retVal = TRUE_REQ;
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

RequestApiState_t Maintenance::FetchMachineData(const char *machineName, const char *apiUrl, Parameters_t &parameters)
{
    RequestApiState_t retVal = FAIL_REQ;
    HTTPClient http;

    String fullApiUrl = String(apiUrl) + "/robot/api/getMachine.php";
    String postData = "machineName=" + String(machineName);

    http.begin(fullApiUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);

    if (httpResponseCode == HTTP_CODE_OK)
    {
        String response = http.getString();

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
    else
    {
        Serial.println("HTTP Request failed. Error code: " + String(httpResponseCode));
    }

    http.end();

    return retVal;
}

std::vector<int> Maintenance::SplitIpString(const String &ipString)
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

String Maintenance::GetJsonValue(const String &json, const String &key)
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
