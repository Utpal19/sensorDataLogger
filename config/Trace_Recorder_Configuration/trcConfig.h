/*
 * Trace Recorder for Tracealyzer v4.10.2
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Main configuration parameters for the trace recorder library.
 */

#ifndef TRC_CONFIG_H
#define TRC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Using Win32 software timer port — matches vTraceTimerReset/uiTraceTimerGetValue
 * defined in main.c */
#define TRC_CFG_HARDWARE_PORT                   TRC_HARDWARE_PORT_Win32

/* Record all events, not just scheduling */
#define TRC_CFG_SCHEDULING_ONLY                 0

/* Trace pvPortMalloc / vPortFree calls */
#define TRC_CFG_INCLUDE_MEMMANG_EVENTS          1

/* Enable vTracePrint / vTracePrintF user events */
#define TRC_CFG_INCLUDE_USER_EVENTS             1

/* ISR tracing (no ISRs on POSIX port, but keep enabled) */
#define TRC_CFG_INCLUDE_ISR_TRACING             1

/* Show task ready events — gives accurate response-time stats */
#define TRC_CFG_INCLUDE_READY_EVENTS            1

/* Trace OS tick increments */
#define TRC_CFG_INCLUDE_OSTICK_EVENTS           1

/* Stack monitor — disabled to reduce overhead */
#define TRC_CFG_ENABLE_STACK_MONITOR            0
#define TRC_CFG_STACK_MONITOR_MAX_TASKS         10
#define TRC_CFG_STACK_MONITOR_MAX_REPORTS       1

/* TzCtrl task: low priority, moderate delay */
#define TRC_CFG_CTRL_TASK_PRIORITY              1
#define TRC_CFG_CTRL_TASK_DELAY                 10
#define TRC_CFG_CTRL_TASK_STACK_SIZE            PTHREAD_STACK_MIN

/* Static buffer allocation */
#define TRC_CFG_RECORDER_BUFFER_ALLOCATION      TRC_RECORDER_BUFFER_ALLOCATION_STATIC

#define TRC_CFG_MAX_ISR_NESTING                 8
#define TRC_CFG_ISR_TAILCHAINING_THRESHOLD      0
#define TRC_CFG_RECORDER_DATA_INIT              1
#define TRC_CFG_RECORDER_DATA_ATTRIBUTE
#define TRC_CFG_USE_TRACE_ASSERT                0

#ifdef __cplusplus
}
#endif

#endif /* TRC_CONFIG_H */
