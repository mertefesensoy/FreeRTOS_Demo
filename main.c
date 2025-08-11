/* -------------------- Includes -------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <conio.h>   /* _kbhit(), _getch() */
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* -------------------- Knobs -------------------- */
#define SENSOR_PERIOD_MS          100      /* “interrupt” every 100ms */
#define LOGGER_PERIOD_MS         1000      /* print status every 1s   */
#define HEARTBEAT_PERIOD_MS      2000
#define CONSOLE_POLL_MS            20

#define CBUF_CAPACITY              64      /* circular buffer length  */
#define USE_MUTEX                   1      /* flip to 0 to test races */

/* -------------------- Shared State -------------------- */
typedef struct {
    uint32_t buf[CBUF_CAPACITY];
    size_t   head;        /* next write index */
    size_t   count;       /* number of valid samples */
    uint64_t sum;         /* running sum for O(1) average */
} sensor_buffer_t;

static sensor_buffer_t gSensorBuf = { 0 };

#if USE_MUTEX
static SemaphoreHandle_t gBufMutex;
#endif

/* Task handles so the “ISR shim” can notify the handler */
static TaskHandle_t xSensorHandlerTask = NULL;

/* Console (UART) line queue */
#define LINE_MAX 80
static QueueHandle_t xLineQueue;

/* -------------------- Small Helpers -------------------- */
static inline void lockBuf(void) {
#if USE_MUTEX
    xSemaphoreTake(gBufMutex, portMAX_DELAY);
#endif
}
static inline void unlockBuf(void) {
#if USE_MUTEX
    xSemaphoreGive(gBufMutex);
#endif
}

static void cbuf_push(sensor_buffer_t* b, uint32_t sample)
{
    /* if buffer not full, just append; if full, overwrite oldest and fix sum */
    if (b->count < CBUF_CAPACITY) {
        b->buf[b->head] = sample;
        b->head = (b->head + 1) % CBUF_CAPACITY;
        b->count++;
        b->sum += sample;
    }
    else {
        /* overwrite oldest = element at head, because head always points to next write */
        uint32_t old = b->buf[b->head];
        b->buf[b->head] = sample;
        b->head = (b->head + 1) % CBUF_CAPACITY;
        b->sum += sample;
        b->sum -= old;
    }
}

static void cbuf_clear(sensor_buffer_t* b)
{
    memset(b, 0, sizeof(*b));
}

/* -------------------- Tasks -------------------- */

/* High-priority “ISR shim”: fires every 100 ms and notifies the handler with the reading.
   In real HW, the ISR would use vTaskNotifyGiveFromISR/xTaskNotifyFromISR; here we use a task. */
static void vSensorIsrShimTask(void* pv)
{
    (void)pv;
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(SENSOR_PERIOD_MS));

        /* Pseudo reading: 12-bit ADC-ish value with some slow drift */
        static uint32_t t = 0;
        uint32_t reading = (uint32_t)((rand() % 4096) + (t++ % 50));

        /* Pass the reading to the handler via direct notification value */
        /* Overwrite mode: we don’t queue many interrupts; we just deliver latest sample. */
        xTaskNotify(xSensorHandlerTask, reading, eSetValueWithOverwrite);
        /* Keep ISR-shim short: do not take mutex or do heavy work here. */
    }
}

/* Sensor handler: blocked on notification; on wake reads value and updates buffer under mutex. */
static void vSensorHandlerTask(void* pv)
{
    (void)pv;
    uint32_t value = 0;

    for (;;) {
        /* Wait forever for next “interrupt” sample */
        BaseType_t ok = xTaskNotifyWait(
            0,                   /* don’t clear any bits on entry */
            0xFFFFFFFF,          /* clear all bits on exit */
            &value,              /* out: last written value */
            portMAX_DELAY);
        if (ok == pdTRUE) {
            TickType_t now = xTaskGetTickCount();

            /* Simulate the “deferred work” of an ISR: update shared buffer */
            lockBuf();
            cbuf_push(&gSensorBuf, value);
            unlockBuf();

            printf("[Handler] t=%u ms  sample=%lu\n",
                (unsigned)pdTICKS_TO_MS(now), (unsigned long)value);
        }
    }
}

/* Logger: once per second, prints average and count under protection. */
static void vLoggerTask(void* pv)
{
    (void)pv;
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(LOGGER_PERIOD_MS));

        uint64_t sum;
        size_t   count;

        lockBuf();
        sum = gSensorBuf.sum;
        count = gSensorBuf.count;
        unlockBuf();

        double avg = (count == 0) ? 0.0 : (double)sum / (double)count;

        printf("[Logger] samples=%zu  avg=%.2f%s\n",
            count, avg,
#if USE_MUTEX
            ""
#else
            "   (mutex OFF: expect occasional glitches!)"
#endif
        );
    }
}

/* Heartbeat/background: runs only when others are idle/blocked. */
static void vHeartbeatTask(void* pv)
{
    (void)pv;
    TickType_t last = xTaskGetTickCount();
    int led = 0;

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS));
        led ^= 1;
        printf("[Heartbeat] LED %s\n", led ? "ON" : "OFF");
    }
}

/* Console (UART) task: reads keystrokes, builds a line, and handles simple commands:
   - status : prints buffer length and last sample time
   - avg    : prints the current average
   - clear  : clears the buffer
*/
static void vConsoleTask(void* pv)
{
    (void)pv;

    char line[LINE_MAX] = { 0 };
    size_t len = 0;
    TickType_t last = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(CONSOLE_POLL_MS));

#ifdef _WIN32
        while (_kbhit()) {
            int ch = _getch();
            if (ch == '\r' || ch == '\n') {
                line[len] = '\0';
                if (len > 0) {
                    /* Echo & process */
                    printf("[UART] cmd: %s\n", line);

                    if (strcmp(line, "status") == 0) {
                        lockBuf();
                        size_t count = gSensorBuf.count;
                        size_t head = gSensorBuf.head;
                        unlockBuf();
                        printf("[UART] status: count=%zu, head=%zu, cap=%d\n",
                            count, head, CBUF_CAPACITY);
                    }
                    else if (strcmp(line, "avg") == 0) {
                        lockBuf();
                        uint64_t sum = gSensorBuf.sum;
                        size_t count = gSensorBuf.count;
                        unlockBuf();
                        double avg = (count == 0) ? 0.0 : (double)sum / (double)count;
                        printf("[UART] avg=%.4f over %zu samples\n", avg, count);
                    }
                    else if (strcmp(line, "clear") == 0) {
                        lockBuf();
                        cbuf_clear(&gSensorBuf);
                        unlockBuf();
                        printf("[UART] buffer cleared.\n");
                    }
                    else {
                        printf("[UART] unknown cmd. Try: status | avg | clear\n");
                    }
                }
                len = 0;
                line[0] = '\0';
            }
            else if (ch == 8 /* backspace */) {
                if (len > 0) { len--; line[len] = '\0'; }
            }
            else if (ch >= 32 && ch < 127 && len < LINE_MAX - 1) {
                line[len++] = (char)ch;
            }
        }
#else
        /* Non-Windows builds could poll stdin here if needed. */
#endif
    }
}

/* -------------------- Main -------------------- */
int main(void)
{
    /* Seed randomness for pseudo sensor */
    srand((unsigned)time(NULL));

#if USE_MUTEX
    gBufMutex = xSemaphoreCreateMutex();
    configASSERT(gBufMutex != NULL);
#endif

    xLineQueue = xQueueCreate(4, LINE_MAX);
    (void)xLineQueue; /* (reserved if you later swap to a producer thread) */

    /* Create tasks */
    BaseType_t ok;

    ok = xTaskCreate(vSensorHandlerTask, "SensorHandler",
        configMINIMAL_STACK_SIZE + 256, NULL,
        tskIDLE_PRIORITY + 3, &xSensorHandlerTask);
    configASSERT(ok == pdPASS && xSensorHandlerTask != NULL);

    ok = xTaskCreate(vSensorIsrShimTask, "SensorISR",
        configMINIMAL_STACK_SIZE + 128, NULL,
        tskIDLE_PRIORITY + 4, NULL);
    configASSERT(ok == pdPASS);

    ok = xTaskCreate(vLoggerTask, "Logger",
        configMINIMAL_STACK_SIZE + 256, NULL,
        tskIDLE_PRIORITY + 2, NULL);
    configASSERT(ok == pdPASS);

    ok = xTaskCreate(vHeartbeatTask, "Heartbeat",
        configMINIMAL_STACK_SIZE + 128, NULL,
        tskIDLE_PRIORITY + 1, NULL);
    configASSERT(ok == pdPASS);

    ok = xTaskCreate(vConsoleTask, "Console",
        configMINIMAL_STACK_SIZE + 256, NULL,
        tskIDLE_PRIORITY + 1, NULL);
    configASSERT(ok == pdPASS);

    /* Go! */
    vTaskStartScheduler();

    /* Should never get here */
    for (;;);
    return 0;
}
