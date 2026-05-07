/*
 * Trace Recorder for Tracealyzer v4.10.2
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Configuration parameters for the FreeRTOS kernel port.
 */

#ifndef TRC_KERNEL_PORT_CONFIG_H
#define TRC_KERNEL_PORT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Streaming mode: writes continuously to trace.psf via File stream port */
#define TRC_CFG_RECORDER_MODE                   TRC_RECORDER_MODE_STREAMING

/* FreeRTOS version used in this project */
#define TRC_CFG_FREERTOS_VERSION                TRC_FREERTOS_VERSION_10_4_1

/* Enable all event categories — gives full visibility in Tracealyzer */
#define TRC_CFG_INCLUDE_EVENT_GROUP_EVENTS      1
#define TRC_CFG_INCLUDE_TIMER_EVENTS            1
#define TRC_CFG_INCLUDE_PEND_FUNC_CALL_EVENTS   1
#define TRC_CFG_INCLUDE_STREAM_BUFFER_EVENTS    1

#define TRC_CFG_ACKNOWLEDGE_QUEUE_SET_SEND      0 /* TRC_ACKNOWLEDGED */

#ifdef __cplusplus
}
#endif

#endif /* TRC_KERNEL_PORT_CONFIG_H */
