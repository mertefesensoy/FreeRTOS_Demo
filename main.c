#include <stdio.h>
#include <stdint.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#define STATS_BUFFER_SIZE 256

static void prvStatsTask(void* pvParameters)
{
    TickType_t xLastWake = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(1000);
    static char buf[STATS_BUFFER_SIZE];

    for (;;)
    {

        vTaskDelayUntil(&xLastWake, xPeriod);

        printf("\nTask          Abs(ms)    %%Time\n");

        vTaskGetRunTimeStatistics(buf, STATS_BUFFER_SIZE);

        printf("%s\n", buf);
    }
}

void vPrintLine(const char* msg) {
    printf("%s\n", msg);
}

void vTaskFunction(void* pvParameters)
{
    char* pcTaskName = (char*)pvParameters;
    TickType_t xLastWakeTime;
    const TickType_t xDelay250ms = pdMS_TO_TICKS(250);

    xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        printf("%s\n", pcTaskName);

        printf("High water mark (words): %d\n", uxTaskGetStackHighWaterMark(NULL));

        printf("Heap size: %d\n", xPortGetFreeHeapSize());

        for (volatile uint32_t i = 0; i < 500000; ++i);
        vTaskDelayUntil(&xLastWakeTime, xDelay250ms);
    }
}

void vPeriodicTask(void* pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xDelay5s = pdMS_TO_TICKS(5000);

    xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        printf("Periodic task is running\n");
        vTaskDelayUntil(&xLastWakeTime, xDelay5s);
    }
}

int main(void)
{
    static const char* pcTextForTask1 = "Task 1 is running";
    static const char* pcTextForTask2 = "Task 2 is running";

    xTaskCreate(vTaskFunction, "Task 1", 256, (void*)pcTextForTask1, 1, NULL);
    xTaskCreate(vTaskFunction, "Task 2", 256, (void*)pcTextForTask2, 1, NULL);
    xTaskCreate(vPeriodicTask, "Periodic Task", 256, NULL, 2, NULL);

    xTaskCreate(prvStatsTask, "Stats", 256, NULL, 3, NULL);

    vTaskStartScheduler();

    for (;;);
}