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



/*
 * A DebugTask will be wake-up by a QueueRecevie and then print.
*/
void vDebugTask(void *pvParameter)
{
    print_msg_t p_msg;
    while(1)
    {
        xQueueReceive(xPrintQueue, &p_msg, portMAX_DELAY);
        printf("%s\n", p_msg.msg);
        fflush(stdout);
    }
}