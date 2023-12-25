#include "globals.h"

TCPServerRouter TCPServer;

void TCPMessageTimerCallback(TimerHandle_t xTimer)
{
    TCPServer.SetTCPMessageTimeout(true);
}