#pragma once


#define PRINT_QUEUE_LEN 32
#define PRINT_MSG_SIZE  128

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DUMP
} LogLevel_t;


typedef struct{
    LogLevel_t level;
    char msg[PRINT_MSG_SIZE];
}print_msg_t;



void debug_log(LogLevel_t lvl, const char *fmt, ...);
void vDebugTask(void *pvParameter);