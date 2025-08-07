#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#define STATS_BUFFER_SIZE 256

int sharedVar = 0; // Initialize sharedVar to a default value


static SemaphoreHandle_t mutex; // Correct the type name to SemaphoreHandle_t
void initializeMutex() {
    mutex = xSemaphoreCreateMutex();
}

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

void vTaskFunction1(void* pvParameters)
{
   char* pcTaskName = (char*)pvParameters;
   TickType_t xLastWakeTime;
   const TickType_t xDelay250ms = pdMS_TO_TICKS(250);

   xLastWakeTime = xTaskGetTickCount();

void vTaskFunction2(void* pvParameters)
{
    char* pcTaskName = (char*)pvParameters;

    for (;;)
    {
        //printf("%s\n", pcTaskName);

        //printf("High water mark (words): %d\n", uxTaskGetStackHighWaterMark(NULL));

        //printf("Heap size: %d\n", xPortGetFreeHeapSize());

        if ((xSemaphoreTake(mutex, pdMS_TO_TICKS(1000))) == pdTRUE) {


            sharedVar++;
            vTaskDelay(pdMS_TO_TICKS(250));

            xSemaphoreGive(mutex);

            printf("Shared variable updated: %d\n", sharedVar);
        }
        else
        {
            // Add a valid statement to the else block  
            printf("Failed to take mutex\n");

        }

        vTaskDelay(pdMS_TO_TICKS(250));
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
    initializeMutex();
    srand((unsigned int)xTaskGetTickCount());

   static const char* pcTextForTask1 = "Task 1 is running";
   static const char* pcTextForTask2 = "Task 2 is running";

   xTaskCreate(vTaskFunction, "Task 1", 256, (void*)pcTextForTask1, 1, NULL);
   xTaskCreate(vTaskFunction, "Task 2", 256, (void*)pcTextForTask2, 1, NULL);
   xTaskCreate(vPeriodicTask, "Periodic Task", 256, NULL, 2, NULL);

   xTaskCreate(prvStatsTask, "Stats", 256, NULL, 3, NULL);

    vTaskStartScheduler();

    for (;;);
}