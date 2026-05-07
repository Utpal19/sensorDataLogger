#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../inc/debug_task.h"
#include "../inc/extern.h"


// print_msg_t p_msg;

uint32_t g_print_drop_count; 

void debug_log(LogLevel_t lvl, const char* fmt, ...)
{
    if(lvl < g_log_level) return;

    BaseType_t status;

    print_msg_t msg;
    msg.level = lvl;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg.msg, PRINT_MSG_SIZE, fmt, ap);
    va_end(ap);
    
    /* Non blocking send. Drop on overflow */
    if(xQueueSend(xPrintQueue, &msg, (TickType_t) 0) != pdPASS)
    {
        // If queue send fails
        g_print_drop_count++;
    }

}



void vPrintRTOSStats(void)
{
    /* 40 chars per task is enough for vTaskList + vTaskGetRunTimeStats */
    static char buf[40 * 10];

    printf("\n===== TASK LIST =====\n");
    printf("%-16s %-8s %-5s %-12s %s\n", "Task", "State", "Pri", "StackLeft", "Num");
    vTaskList(buf);
    printf("%s", buf);

    printf("\n===== CPU USAGE =====\n");
    printf("%-16s %-14s %s\n", "Task", "AbsTicks", "%CPU");
    vTaskGetRunTimeStats(buf);
    printf("%s\n", buf);

    printf("xMyQueue    drops: %lu\n", g_drop_count);
    printf("xPrintQueue drops: %lu\n\n", g_print_drop_count);
    fflush(stdout);
}

/*
 * A DebugTask will be wake-up by a QueueRecevie and then print.
*/
void vDebugTask(void *pvParameter)
{
    print_msg_t p_msg;
    TickType_t xLastStatsTime = xTaskGetTickCount();

    while(1)
    {
        xQueueReceive(xPrintQueue, &p_msg, portMAX_DELAY);
        printf("%s\n", p_msg.msg);
        fflush(stdout);

        /* Print RTOS stats every 5 seconds */
        if((xTaskGetTickCount() - xLastStatsTime) >= pdMS_TO_TICKS(5000))
        {
            vPrintRTOSStats();
            xLastStatsTime = xTaskGetTickCount();
        }
    }
}