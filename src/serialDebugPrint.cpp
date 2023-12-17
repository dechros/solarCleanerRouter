#include "serialDebugPrint.h"

SemaphoreHandle_t xSerialPrintSemaphore;

void SerialDebugPrintInit(void)
{
    Serial.begin(115200);
    xSerialPrintSemaphore = xSemaphoreCreateMutex();
}

void SerialDebugPrint(const char *text)
{
#if SERIAL_DEBUG_PRINT
    xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);
    Serial.println(text);
    xSemaphoreGive(xSerialPrintSemaphore);
#endif
}

void SerialDebugPrint(int number)
{
#if SERIAL_DEBUG_PRINT
    xSemaphoreTake(xSerialPrintSemaphore, portMAX_DELAY);
    Serial.println(number);
    xSemaphoreGive(xSerialPrintSemaphore);
#endif
}