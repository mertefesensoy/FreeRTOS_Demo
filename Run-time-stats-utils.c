/* run_time_stats_utils.c
 * FreeRTOS V202411.00
 * High-resolution run-time stats using QueryPerformanceCounter (microsecond resolution)
 */

#include <windows.h>      /* For QueryPerformanceCounter/Frequency */
#include "FreeRTOS.h"     /* For port macros */
#include <stdint.h>

 /* Variables for run-time stats time base (microseconds). */
static long long llInitialRunTimeCounterValue = 0LL;
static long long llTicksPerMicrosecond = 0LL;

/*-----------------------------------------------------------*/

void vConfigureTimerForRunTimeStats(void)
{
    LARGE_INTEGER liFreq, liCount;

    /* Retrieve performance-counter frequency */
    if (QueryPerformanceFrequency(&liFreq) == 0)
    {
        /* Fallback: assume 1 tick per microsecond */
        llTicksPerMicrosecond = 1LL;
    }
    else
    {
        /* Calculate how many performance ticks occur in 1 microsecond */
        llTicksPerMicrosecond = liFreq.QuadPart / 1000000LL;
    }

    /* Capture initial counter value as baseline */
    QueryPerformanceCounter(&liCount);
    llInitialRunTimeCounterValue = liCount.QuadPart;
}
/*-----------------------------------------------------------*/

unsigned long ulGetRunTimeCounterValue(void)
{
    LARGE_INTEGER liCurrent;
    unsigned long ulReturn;

    /* Get current performance-counter value */
    QueryPerformanceCounter(&liCurrent);

    /* Compute elapsed ticks since start, then convert to microseconds */
    ulReturn = (unsigned long)
        ((liCurrent.QuadPart - llInitialRunTimeCounterValue)
            / llTicksPerMicrosecond);

    return ulReturn;
}
/*-----------------------------------------------------------*/
