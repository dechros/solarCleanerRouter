/**
 * @file Maintenance.h
 * @author Halit Cetin (halitcetin@live.com)
 * @brief Maintenance mod file.
 * @version 0.1
 * @date 2023-12-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#include "config.h"
#include "globals.h"
#include "serialDebugPrint.h"
#include "api.h"

#define MAINTENANCE_MODE_ACTIVE_MESSAGE "M_A*"
#define MAINTENANCE_MODE_DEACTIVE_MESSAGE "M_D*"

#define GET_PARAMETERS_MESSAGE "GET*"
#define SET_PARAMETERS_MESSAGE "SET*"

typedef enum
{
    WIFI_CONNECTION_STATE,
    CONNECTION_ESTABLISHED,
    CONTINIOUS_UPDATE
} MaintenanceState_t;

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
    RequestApiState_t RequestParametersFromMachine(Parameters_t &machine);
    RequestApiState_t SentParametersToMachine(Parameters_t machine);
    MaintenanceState_t maintenanceState;
    bool maintenanceModeActive = false;
};

#endif /* MAINTENANCE_H */