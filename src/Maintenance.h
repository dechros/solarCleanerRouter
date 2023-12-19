#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#define MAINTENANCE_MODE_ACTIVE_MESSAGE "M_A*"
#define MAINTENANCE_MODE_DEACTIVE_MESSAGE "M_D*"

#define GET_MACHINE_NAME_MESSAGE "GET0*"
#define GET_PARAMETERS_MESSAGE "GET1*"
#define SET_PARAMETERS_MESSAGE "SET1*"

typedef enum
{
    WIFI_CONNECTION_STATE,
    CHECK_WEBSITE_STATE,
    CONNECTION_ESTABLISHED
} MaintenanceState_t;

typedef enum
{
    FAIL_REQ,
    SUCCESS_REQ,
    TRUE_REQ,
    FALSE_REQ
} RequestApiState_t;

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

class Maintenance
{
public:
    Maintenance();
    ~Maintenance();
    void Init(void);
    void Deinit(void);

    bool IsMaintenanceModeActive(void);
    void Run(void);

    MaintenanceState_t GetMaintenanceState(void);
    void SetMaintenanceState(MaintenanceState_t);

private:
    Parameters_t RequestParametersFromMachine();
    RequestApiState_t CreateMachine(const char *apiUrl, Parameters_t &parameters);
    RequestApiState_t FetchMachineData(const char *machineName, const char *apiUrl, Parameters_t &parameters);
    RequestApiState_t IsMachinePresentInDB(const char *apiUrl, const char *targetMachineName);
    String GetJsonValue(const String &json, const String &key);
    std::vector<int> SplitIpString(const String &ipString);
    MaintenanceState_t maintenanceState;
    bool maintenanceModeActive = false;
};

#endif /* MAINTENANCE_H */