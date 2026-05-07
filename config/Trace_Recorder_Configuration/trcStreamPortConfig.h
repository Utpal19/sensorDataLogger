/*
 * Trace Recorder for Tracealyzer v4.10.2
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * File stream port configuration — writes trace data to trace.psf
 */

#ifndef TRC_STREAM_PORT_CONFIG_H
#define TRC_STREAM_PORT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Output file opened in the working directory when the program runs */
#define TRC_CFG_STREAM_PORT_TRACE_FILE                              "trace.psf"

/* Direct write to file — no internal buffer needed for POSIX */
#define TRC_CFG_STREAM_PORT_USE_INTERNAL_BUFFER                     0
#define TRC_CFG_STREAM_PORT_INTERNAL_BUFFER_SIZE                    10240
#define TRC_CFG_STREAM_PORT_INTERNAL_BUFFER_WRITE_MODE              TRC_INTERNAL_EVENT_BUFFER_OPTION_WRITE_MODE_DIRECT
#define TRC_CFG_STREAM_PORT_INTERNAL_BUFFER_TRANSFER_MODE           TRC_INTERNAL_EVENT_BUFFER_OPTION_TRANSFER_MODE_ALL
#define TRC_CFG_STREAM_PORT_INTERNAL_BUFFER_CHUNK_SIZE              4096
#define TRC_CFG_STREAM_PORT_INTERNAL_BUFFER_CHUNK_TRANSFER_AGAIN_SIZE_LIMIT  1024
#define TRC_CFG_STREAM_PORT_INTERNAL_BUFFER_CHUNK_TRANSFER_AGAIN_COUNT_LIMIT 5

#ifdef __cplusplus
}
#endif

#endif /* TRC_STREAM_PORT_CONFIG_H */
