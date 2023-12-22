#ifndef TCP_SERVER_ROUTER_H
#define TCP_SERVER_ROUTER_H

#include <WiFi.h>
#include <stdint.h>

#define ACK_MESSAGE "ACK"
#define TCP_MESSAGE_HEADER "TCP"

typedef enum
{
    WAITING_FOR_CLIENT,
    CLIENT_CONNECTED,
    PARSE_TCP_MESSAGE,
    MACHINE_ACK_AWAITING
}TCPConnectionState_t;

typedef enum
{
    ACK_CAME,
    UNKNOWN_MESSAGE,
    MISSING_MESSAGE,
    RESPONSE_TIMEOUT
}MachineResponseState_t;

typedef struct
{
    uint8_t headerT;
    uint8_t headerC;
    uint8_t headerP;
    uint8_t joystickX;
    uint8_t joystickY;
    uint8_t driveSpeed;
    uint8_t brushSpeed;
    union
    {
        uint8_t buttons;
        struct
        {
            uint8_t waterButton : 1;
            uint8_t lampButton : 1;
            uint8_t startButton : 1;
            uint8_t passButton : 1;
            uint8_t brushFrontCW : 1;
            uint8_t brushFrontCCW : 1;
            uint8_t brushRearCW : 1;
            uint8_t brushRearCCW : 1;
        };
    };
    union
    {
        uint8_t data;
        struct
        {
            uint8_t emergencyButton : 1;
            uint8_t : 7;
        };
    };
    uint8_t checksum;
}TCPMessage_t;

class TCPServerRouter
{
public:
    TCPServerRouter();
    ~TCPServerRouter();

    void Init(void);
    void Deinit(void);
    void Run(void);

    TCPConnectionState_t GetTCPConnectionState(void);
    void SetTCPConnectionState(TCPConnectionState_t);

    bool WaitingForClient(void);
    bool IsClientConnected(void);

    int TCPMessageBufferSize(void);
    bool CheckForProperTCPMessage(TCPMessage_t*);
    void SendACKToRemote(void);

    void SendMessageToMachine(TCPMessage_t);
    MachineResponseState_t WaitACKFromMachine(void);

    bool GetTCPMessageTimeout(void);
    void SetTCPMessageTimeout(bool);
    void ResetTCPMessageTimeoutTimer(void);
    void StopTCPMessageTimeoutTimer(void);

private:
    WiFiServer server;
    WiFiClient client;

    TCPConnectionState_t TCPConnectionState = WAITING_FOR_CLIENT;
    bool initDone = false;
    bool TCPMessageTimeout = false;
    TimerHandle_t TCPMessageTimer;
    
    bool ControlChecksum(uint8_t *dataPointer, uint8_t size);
};

void TCPMessageTimerCallback(TimerHandle_t xTimer);



#endif /* TCP_SERVER_ROUTER_H */