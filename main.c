#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#define STATS_BUFFER_SIZE 256
#define INTERRUPT_KEY VK_SPACE
#define INPUT_BUFFER_SIZE 100

static SemaphoreHandle_t mutex;
static QueueHandle_t xInputQueue;

float globalFloatSum = 0.0f;
int globalFloatCount = 0;
int sharedVar = 0;

TaskHandle_t xInterruptTaskHandle = NULL;

void initializeMutex() {
    mutex = xSemaphoreCreateMutex();
}

float stringToFloat(const char* str) {
    int sum = 0;
    while (*str) {
        sum += (unsigned char)(*str);
        str++;
    }
    return sum / 10.0f;  
}

static void prvStatsTask(void* pvParameters)
{
    //static char buf[STATS_BUFFER_SIZE];

    for (;;)
    {

        vTaskDelay(pdMS_TO_TICKS(1000));

        //  printf("\nTask          Abs(ms)    %%Time\n");

         // vTaskGetRunTimeStatistics(buf, STATS_BUFFER_SIZE);

         // printf("%s\n", buf);
    }
}

void vPrintLine(const char* msg) {
    printf("%s\n", msg);
}

void vTaskFunction1(void* pvParameters)
{
    //char* pcTaskName = (char*)pvParameters;

    for (;;)
    {
        //printf("%s\n", pcTaskName);

        //printf("High water mark (words): %d\n", uxTaskGetStackHighWaterMark(NULL));

        //printf("Heap size: %d\n", xPortGetFreeHeapSize());

        if ((xSemaphoreTake(mutex, pdMS_TO_TICKS(1000))) == pdTRUE) {


            sharedVar++;
            vTaskDelay(pdMS_TO_TICKS(250));

            xSemaphoreGive(mutex);

            printf("Shared variable updated by Task 1: %d\n", sharedVar);
        }
        else
        {
            // Add a valid statement to the else block  
            printf("Failed to take mutex at Task 1\n");

        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void vTaskFunction2(void* pvParameters)
{
    //char* pcTaskName = (char*)pvParameters;

    for (;;)
    {
        //printf("%s\n", pcTaskName);

        //printf("High water mark (words): %d\n", uxTaskGetStackHighWaterMark(NULL));

        //printf("Heap size: %d\n", xPortGetFreeHeapSize());

        if ((xSemaphoreTake(mutex, pdMS_TO_TICKS(1000))) == pdTRUE) {


            sharedVar++;
            vTaskDelay(pdMS_TO_TICKS(250));

            xSemaphoreGive(mutex);

            printf("Shared variable updated by Task 2: %d\n", sharedVar);
        }
        else
        {
            // Add a valid statement to the else block  
            printf("Failed to take mutex at Task 2\n");

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

void vInterruptHandledTask(void* pvParameter)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        printf(">>> Interrupt handled by ISR Task at tick %lu\n", xTaskGetTickCount());

        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            // Simulate some processing
            sharedVar += 10;
            xSemaphoreGive(mutex);
            printf(">>> Interrupt Task added +10 to sharedVar: %d\n", sharedVar);
        }
    }
}

void vKeyboardInterruptTask(void* pvParameters)
{
    for (;;)
    {
        if (GetAsyncKeyState(INTERRUPT_KEY) & 0x8000)
        {
            vTaskNotifyGiveFromISR(xInterruptTaskHandle, 0, NULL);
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vUARTHandlerTask(void* pvParameters)
{
    char rxBuffer[INPUT_BUFFER_SIZE];
    for (;;) 
    {
        if (xQueueReceive(xInputQueue, &rxBuffer, portMAX_DELAY) == pdTRUE) 
        {
            rxBuffer[strcspn(rxBuffer, "\n")] = 0; // Remove newline
            printf("[UART] Received message: %s\n", rxBuffer);

            if (strcmp(rxBuffer, "avg") == 0)
            {
                if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
                {
                    if (globalFloatCount == 0)
                        printf(">>> No inputs yet.\n");
                    else
                        printf(">>> Current average: %.2f\n", globalFloatSum / globalFloatCount);

                    xSemaphoreGive(mutex);
                }
            }
            else
            {
                float val;
                char* endptr;
                val = strtof(rxBuffer, &endptr);

                if (*endptr != '\0') {
                    // Not a valid float, so convert to float via ASCII
                    val = stringToFloat(rxBuffer);
                    printf(">>> Converted string to float: %.2f\n", val);
                }

                if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
                {
                    globalFloatSum += val;
                    globalFloatCount++;
                    sharedVar += 10;
                    xSemaphoreGive(mutex);

                    printf(">>> Stored value: %.2f | Total count: %d\n",
                        val, globalFloatCount);
                }
            }
        }
    }
}

DWORD WINAPI UARTSimThread(LPVOID lpParam) 
{
    char buffer[INPUT_BUFFER_SIZE];
    for (;;) 
    {
        if (fgets(buffer, sizeof(buffer), stdin)) 
        {
            buffer[strcspn(buffer, "\n")] = 0;  
            xQueueSend(xInputQueue, buffer, portMAX_DELAY);
        }
    }
    return 0;
}

int main(void)
{
    initializeMutex();

    xInputQueue = xQueueCreate(5, sizeof(char[INPUT_BUFFER_SIZE]));

    
    CreateThread(NULL, 0, UARTSimThread, NULL, 0, NULL);

    xTaskCreate(vUARTHandlerTask, "UART RX", 256, NULL, 2, NULL);


    //static const char* pcTextForTask1 = "Task 1 is running";
    //static const char* pcTextForTask2 = "Task 2 is running";

    //xTaskCreate(vTaskFunction1, "Task 1", 256, NULL, 1, NULL);
    //xTaskCreate(vTaskFunction2, "Task 2", 256, NULL, 1, NULL);
    //xTaskCreate(vPeriodicTask, "Periodic Task", 256, NULL, 2, NULL);

    //xTaskCreate(prvStatsTask, "Stats", 256, NULL, 3, NULL);
    //xTaskCreate(vInterruptHandledTask, "ISR Task", 256, NULL, 2, &xInterruptTaskHandle);
    //xTaskCreate(vKeyboardInterruptTask, "Keyboard ISR", 256, NULL, 3, NULL);

    vTaskStartScheduler();

    for (;;);
}