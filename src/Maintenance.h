#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#define MAINTENANCE_MODE_ACTIVE_MESSAGE         "M_A*"
#define MAINTENANCE_MODE_DEACTIVE_MESSAGE       "M_D*"

#define GET_MACHINE_NAME_MESSAGE                "GET0*"
#define GET_PARAMETERS_MESSAGE                  "GET1*"
#define SET_PARAMETERS_MESSAGE                  "SET1*"

typedef enum
{
    WIFI_CONNECTION_STATE,
    CHECK_WEBSITE_STATE,
    GET_PARAMETERS_STATE,
    UPDATE_MACHINE_STATE
}MaintenanceState_t;

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

    MaintenanceState_t maintenanceState;
    bool maintenanceModeActive = false;
};




#endif /* MAINTENANCE_H */