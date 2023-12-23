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
#include "webApi.h"

#define TIMEOUT_MS 1000

#define MAINTENANCE_MODE_ACTIVE_MESSAGE "M_A*"
#define MAINTENANCE_MODE_DEACTIVE_MESSAGE "M_D*"

#define GET_PARAMETERS_MESSAGE "GET"
#define SET_PARAMETERS_MESSAGE "SET"
#define GET_PARAMETERS_ACK "GET_ACK"
#define SET_PARAMETERS_ACK "SET_ACK"

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
    bool ReceiveAck(const char *expectedAck);
    RequestApiState_t GetParamFromMachine(Parameters_t &machine);
    RequestApiState_t SendParamToMachine(const Parameters_t machine);
    MaintenanceState_t maintenanceState;
    bool maintenanceModeActive = false;
    bool initDone = false;
};

#endif /* MAINTENANCE_H */