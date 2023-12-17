#ifndef SERIAL_DEBUG_PRINT_H
#define SERIAL_DEBUG_PRINT_H

#include <Arduino.h>

#define SERIAL_DEBUG_PRINT 1

void SerialDebugPrintInit(void);
void SerialDebugPrint(const char *text);
void SerialDebugPrint(int number);

#endif /* SERIAL_DEBUG_PRINT_H */