#ifndef GLOBALS_H
#define GLOBALS_H

#include "TCPServerRouter.h"

extern TCPServerRouter TCPServer;

void TCPMessageTimerCallback(TimerHandle_t xTimer);

#endif /* GLOBALS_H */