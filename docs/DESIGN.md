# FreeRTOS Sensor Data Logger — Design Document

## 1. Project Overview

A multi-task FreeRTOS application that simulates a real-world embedded sensor data acquisition system. It runs on the **FreeRTOS POSIX/Linux port** (no hardware needed) and exercises the core RTOS concepts that production firmware relies on: task management, inter-task communication, synchronization, bounded data structures, and centralized debug logging.

---

## 2. Learning Goals

1. **Task creation and prioritization** — five tasks at different priorities, understanding which runs when and why.
2. **Queue-based producer-consumer** — decoupling a fast sensor from a slower logger without blocking the producer.
3. **Mutex with priority inheritance** — protecting a shared linked list from concurrent access, understanding why a mutex is not the same as a binary semaphore.
4. **Bounded linked list (sliding FIFO window)** — append at tail, evict from head when cap exceeded. Always holds the most recent N readings.
5. **Static memory pool** — preallocated node pool with a free-list, replacing heap allocation for deterministic O(1) alloc/free. The standard embedded pattern.
6. **Centralized debug logging** — one task owns `printf`. All others send `PrintMsg` structs (embedded char array, not pointer) through a print queue. Non-blocking send, drop on overflow.
7. **Periodic vs relative delays** — `vTaskDelayUntil` for the sensor (drift-free sampling), `vTaskDelay` for stats (timing not critical).
8. **Runtime diagnostics** — heap high-water mark, per-task stack usage, CPU time stats, drop counters. Real firmware debugging skills.

---

## 3. System Architecture

Five tasks, two queues, two mutexes:

```
┌──────────────┐     xSensorQueue     ┌──────────────┐
│  Sensor (P4) │─────────────────────►│  Logger (P3) │
│  periodic    │   SensorReading       │  queue→list  │
│  100ms       │   copy-by-value       │  bounded 100 │
└──────────────┘                       └──────┬───────┘
                                              │
                                         xListMutex
                                              │
                              ┌───────────────┼───────────────┐
                              ▼               ▼               ▼
                       ┌────────────┐  ┌────────────┐  ┌────────────┐
                       │ Stats (P1) │    │ Shell (P2) │      │debug_dump  │
                       │ 2s period  │    │ stdin cmds │      │ (helper)   │
                       └─────┬──────┘  └─────┬──────┘  └─────┬──────┘
                             │               │               │
                             └───────┬───────┴───────────────┘
                                     │
                                     ▼  debug_log()
                               xPrintQueue
                              PrintMsg copy
                                     │
                                     ▼
                              ┌─────────────┐
                              │ Debug (P1)  │
                              │ sole printf │
                              └─────────────┘
```

---

## 4. Task Table

| Task   | Priority | Wakes on                  | Purpose                                                     |
|--------|----------|---------------------------|-------------------------------------------------------------|
| Sensor | 4        | `vTaskDelayUntil` (100ms) | Generate reading, `xQueueSend` (non-blocking), count drops  |
| Logger | 3        | `xQueueReceive` (blocks)  | Append to linked list tail, evict from head if >100 entries |
| Shell  | 2        | `fgets` (stdin)           | Parse commands: stats, dump, clear, rate, tasks, help       |
| Stats  | 1        | `vTaskDelay` (2s)         | Snapshot list under mutex, compute min/max/avg outside lock |
| Debug  | 1        | `xQueueReceive` (blocks)  | Drain print queue, `printf` each message, `fflush`         |

---

## 5. Three Communication Channels

### Channel 1: Sensor → Logger (via Queue)

```
Sensor produces SensorReading
    │
    ▼
xQueueSend(xSensorQueue, ..., timeout=0)
    │
    ├─ success → reading copied into queue, logger unblocks
    └─ full    → g_drop_count++, sensor moves on (never blocks!)
    │
    ▼
xQueueReceive(xSensorQueue, ..., portMAX_DELAY) in logger
    │
    ▼
Logger appends to linked list (under xListMutex)
```

### Channel 2: Logger ↔ Stats/Shell (via Linked List + xListMutex)

```
Logger task           Stats task / Shell task
    │                        │
    ▼                        ▼
take xListMutex        take xListMutex (waits if logger holds it)
mutate list            snapshot list into local buffer
release                release
                       compute / display from local copy
                       (no lock needed — local data)
```

The snapshot pattern is critical: **copy under lock, work outside lock**. Keeps lock hold times tiny.

### Channel 3: Everyone → Debug (via Print Queue)

```
┌────────────────────────────────────────────────────────────┐
│  Any task:                                                 │
│     debug_log(LOG_INFO, "temp=%.1f", 23.5);                │
│           │                                                │
│           ▼                                                │
│     filter level  →  format into PrintMsg                  │
│           │                                                │
│           ▼                                                │
│     xQueueSend(xPrintQueue, &msg, 0)   [non-blocking]     │
│           │                                                │
│           ▼                                                │
│     xPrintQueue  [ring, size 32 × sizeof(PrintMsg)]        │
│           │                                                │
│           ▼                                                │
│  Debug task:                                               │
│     xQueueReceive(xPrintQueue, &msg, portMAX_DELAY)        │
│     printf("[%s] %s\n", level, msg)                        │
└────────────────────────────────────────────────────────────┘
```

---

## 6. Runtime Flow

### Phase 1: Boot & Initialization (single-threaded)

```
main() starts
   │
   ├─► Create xSensorQueue (holds 10 SensorReading structs)
   ├─► Create xListMutex   (protects linked list)
   ├─► Create xStatsMutex  (protects SensorStats struct)
   ├─► Create xPrintQueue  (holds 32 PrintMsg structs)
   ├─► configASSERT all are non-NULL
   │
   ├─► xTaskCreate(vSensorTask, prio=4)   ─┐
   ├─► xTaskCreate(vLoggerTask, prio=3)    │  Tasks created but
   ├─► xTaskCreate(vShellTask,  prio=2)    │  NOT yet running
   ├─► xTaskCreate(vStatsTask,  prio=1)    │
   ├─► xTaskCreate(vDebugTask,  prio=1)   ─┘
   │
   └─► vTaskStartScheduler()  ──► control transfers to FreeRTOS, never returns
```

### Phase 2: Scheduler Starts

Scheduler picks the highest-priority ready task → **Sensor Task (prio 4)** runs first.
The others sit in the ready queue waiting their turn or blocked on something.

### Phase 3: Steady-State Concurrent Flow

```
Time(ms) │ Sensor(P4)         │ Logger(P3)         │ Shell(P2)          │ Stats(P1)         │ Debug(P1)
─────────┼────────────────────┼────────────────────┼────────────────────┼───────────────────┼─────────────
   0     │ generate reading   │ blocked on queue   │ blocked on stdin   │ blocked delay     │ blocked queue
         │ xQueueSend ────────┼──► unblocks logger │                    │                   │
         │ vTaskDelayUntil    │                    │                    │                   │
         │ (sleeps 100ms)     │ take xListMutex    │                    │                   │
   1     │ ░░ blocked ░░      │ append to list     │                    │                   │
         │                    │ debug_log() ───────┼────────────────────┼───────────────────┼──► unblocks
         │                    │ release mutex      │                    │                   │ printf(...)
         │                    │ back to QueueRecv  │                    │                   │ back to recv
  100    │ wakes, generates   │ unblocks ─────────►│                    │                   │
         │ ...repeats...      │                    │                    │                   │
 2000    │                    │                    │                    │ wakes             │
         │                    │                    │                    │ logger_snapshot() │
         │                    │                    │                    │  ├─take mutex     │
         │                    │                    │                    │  ├─copy list      │
         │                    │                    │                    │  └─release        │
         │                    │                    │                    │ compute stats     │
         │                    │                    │                    │ debug_log() ──────┼──► printf
         │                    │                    │                    │ vTaskDelay 2000   │
  ...    │                    │                    │ user types "dump"  │                   │
         │                    │                    │ debug_dump_list()  │                   │
         │                    │                    │  calls debug_log() ┼───────────────────┼──► printf
```

### Priority Behavior

- Sensor (P4): usually blocked in `vTaskDelayUntil`. When it wakes, it preempts everything, runs ~microseconds, sleeps again.
- Logger (P3): usually blocked on `xQueueReceive`. When queue gets an item, it preempts
  shell and stats, processes one item, blocks again.
- Shell (P2): runs when sensor + logger are blocked, mostly sitting in `fgets`.
- Stats (P1): only runs when everyone else is blocked. Background task.
- Debug (P1): same priority as stats, runs when print queue has items and no higher task is ready.

---

## 7. Linked List Design

### Structure

Singly linked list with `head` (oldest) and `tail` (newest) pointers:

```
head                                           tail
 │                                              │
 ▼                                              ▼
┌─────────┐   ┌─────────┐   ┌─────────┐      ┌─────────┐
│ data: 1 │──►│ data: 2 │──►│ data: 3 │─...─►│data:100 │──► NULL
└─────────┘   └─────────┘   └─────────┘      └─────────┘
 OLDEST                                        NEWEST
```

### Operations

**Append to tail (on every queue receive):**

```c
newNode->next = NULL;
if (tail == NULL) {
    head = tail = newNode;      // list was empty
} else {
    tail->next = newNode;
    tail = newNode;
}
list_count++;
```

**Drop from head (when count exceeds MAX_LOG_ENTRIES):**

```c
LogNode *old = head;
head = head->next;
if (head == NULL) tail = NULL;  // list became empty
list_count--;
pool_free(old);                 // return to pool
```

### Sliding Window Behavior

```
After reading 100:  head→[1][2][3]...[99][100]←tail       count=100
Reading 101 arrives:
  append:           head→[1][2][3]...[100][101]←tail      count=101
  over cap, drop 1: head→[2][3][4]...[100][101]←tail      count=100
Reading 102 arrives:
  append:           head→[2][3][4]...[101][102]←tail      count=101
  over cap, drop 2: head→[3][4][5]...[101][102]←tail      count=100
```

Steady-state: count stays at 100, head and tail keep advancing. The list
doesn't reset or wrap — it slides.

### Key Rules

- It is NOT circular. Last node's `next` is always NULL.
- Memory is reused (pool), positions are not.
- Always append at tail, always drop at head. Never mix.

---

## 8. Memory Model

### Phase 1: Heap-Based (pvPortMalloc)

Use FreeRTOS heap_4 (`pvPortMalloc`/`vPortFree`) for linked list nodes.
Monitor with `xPortGetFreeHeapSize()` and `xPortGetMinimumEverFreeHeapSize()`.

**Why not raw `malloc`**: not thread-safe on MCUs, unpredictable timing,
doesn't integrate with FreeRTOS hooks.

### Phase 2: Static Node Pool (target)

Preallocated pool of `LogNode` structs managed via a free-list:

```c
#define POOL_SIZE 128

static LogNode  node_pool[POOL_SIZE];
static LogNode *free_list = NULL;

void pool_init(void) {
    for (int i = 0; i < POOL_SIZE - 1; i++)
        node_pool[i].next = &node_pool[i + 1];
    node_pool[POOL_SIZE - 1].next = NULL;
    free_list = &node_pool[0];
}

LogNode* pool_alloc(void) {
    if (!free_list) return NULL;
    LogNode *n = free_list;
    free_list = free_list->next;
    return n;
}

void pool_free(LogNode *n) {
    n->next = free_list;
    free_list = n;
}
```

Deterministic, zero fragmentation, O(1) alloc/free.

### Phase 3: Fully Static (optional)

Switch tasks/queues/semaphores to `xTaskCreateStatic`, `xQueueCreateStatic`, etc.
Set `configTOTAL_HEAP_SIZE = 0`. Zero-heap system.

---

## 9. Shared Types (`reading.h`)

```c
#ifndef READING_H
#define READING_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define MAX_LOG_ENTRIES     100
#define SENSOR_QUEUE_LEN    10
#define PRINT_QUEUE_LEN     32
#define PRINT_MSG_SIZE      128

typedef struct {
    uint32_t timestamp_ms;
    float    temperature;
    uint8_t  fault;
} SensorReading;

typedef struct {
    float    min;
    float    max;
    float    avg;
    uint32_t sample_count;
    uint32_t drop_count;
} SensorStats;

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DUMP
} LogLevel;

typedef struct {
    LogLevel level;
    char     msg[PRINT_MSG_SIZE];   // embedded array, NOT a pointer
} PrintMsg;

/* Global handles — defined in main.c, extern'd here */
extern QueueHandle_t      xSensorQueue;
extern QueueHandle_t      xPrintQueue;
extern SemaphoreHandle_t  xListMutex;
extern SemaphoreHandle_t  xStatsMutex;
extern volatile uint32_t  g_sample_period_ms;
extern volatile uint32_t  g_drop_count;
extern volatile LogLevel  g_log_level;

#endif
```

### Why PrintMsg embeds a char array (not a char pointer)

FreeRTOS queues copy items by value (`memcpy` of `sizeof(PrintMsg)`). If `msg` were
a `char*`, only the pointer would be copied — but the string it points to lives on
the sender's stack, which vanishes when the sender returns. The debug task would
dereference freed memory. Embedding the array ensures the entire string is copied
into the queue's internal storage.

---

## 10. FreeRTOSConfig.h — Key Settings

```c
#define configUSE_PREEMPTION            1
#define configUSE_TIME_SLICING          1
#define configTICK_RATE_HZ              ((TickType_t)1000)   // 1ms tick
#define configMAX_PRIORITIES            7
#define configMINIMAL_STACK_SIZE        ((unsigned short)256)
#define configTOTAL_HEAP_SIZE           ((size_t)(64 * 1024))
#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_TRACE_FACILITY        1
#define configGENERATE_RUN_TIME_STATS   1
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configUSE_MALLOC_FAILED_HOOK    1
```

**Why each matters:**

- `configTICK_RATE_HZ = 1000`: 1ms resolution, matches typical firmware.
- `configUSE_MUTEXES`: enables priority inheritance mutexes.
- `configUSE_TRACE_FACILITY` + `configGENERATE_RUN_TIME_STATS`: shell's `tasks` command can print per-task CPU usage.
- `configCHECK_FOR_STACK_OVERFLOW = 2`: canary check; the hook fires if a task blows its stack.
- `configUSE_MALLOC_FAILED_HOOK`: catches heap exhaustion early.

---

## 11. File Structure

```
sensor_logger/
├── CLAUDE.md                     # Claude Code context (points to @docs/DESIGN.md)
├── Makefile
├── FreeRTOS/                     # Kernel source + POSIX port + heap_4.c
│   ├── Source/
│   ├── include/
│   └── portable/
│       ├── ThirdParty/GCC/Posix/
│       └── MemMang/heap_4.c
├── config/
│   └── FreeRTOSConfig.h
├── docs/
│   └── DESIGN.md                 # This document
└── src/
    ├── main.c                    # Create primitives, create tasks, start scheduler
    ├── reading.h                 # Shared types, extern handles, constants
    ├── sensor_task.c/h           # Periodic producer
    ├── logger_task.c/h           # Queue consumer, linked list owner, snapshot API
    ├── stats_task.c/h            # Periodic stats computation
    ├── shell_task.c/h            # Command parser
    └── debug_task.c/h            # Print queue consumer, debug_log API
```

---

## 12. API Reference by File

### `main.c`

| API                           | Purpose                                           |
|-------------------------------|---------------------------------------------------|
| `xQueueCreate()`              | Create xSensorQueue and xPrintQueue               |
| `xSemaphoreCreateMutex()`     | Create xListMutex and xStatsMutex (with priority inheritance) |
| `xTaskCreate()`               | Spawn all five tasks                              |
| `vTaskStartScheduler()`       | Hand control to FreeRTOS (never returns)          |
| `vApplicationStackOverflowHook()` | Fires on stack overflow (enabled in config)   |
| `vApplicationMallocFailedHook()`  | Fires on heap exhaustion                      |

### `sensor_task.c`

| API                           | Purpose                                           |
|-------------------------------|---------------------------------------------------|
| `vTaskDelayUntil()`           | Periodic wake — drift-free, NOT `vTaskDelay`      |
| `xTaskGetTickCount()`         | Current tick for timestamp                        |
| `xQueueSend(..., 0)`         | Non-blocking send; drop + count on full           |
| `debug_log()`                 | Report faults and drops                           |

### `logger_task.c`

| API                           | Purpose                                           |
|-------------------------------|---------------------------------------------------|
| `xQueueReceive(..., portMAX_DELAY)` | Block until sensor sends data               |
| `xSemaphoreTake(xListMutex)`  | Lock list for mutation                           |
| `xSemaphoreGive(xListMutex)`  | Unlock list                                      |
| `pool_alloc()` / `pool_free()`| Static pool allocation                           |
| `debug_log()`                 | Report evictions and pool errors                  |
| `logger_snapshot()`           | Thread-safe: lock, copy entries to caller buffer, unlock |
| `logger_clear()`              | Thread-safe: lock, free all nodes, unlock         |
| `logger_count()`              | Thread-safe: lock, return count, unlock           |

### `stats_task.c`

| API                           | Purpose                                           |
|-------------------------------|---------------------------------------------------|
| `vTaskDelay()`                | 2-second period (timing not critical)             |
| `logger_snapshot()`           | Get list copy under lock                          |
| `xSemaphoreTake(xStatsMutex)` | Lock shared SensorStats for writing              |
| `debug_log()`                 | Report computed stats                             |
| `stats_get()`                 | Thread-safe getter for shell to read SensorStats  |

### `shell_task.c`

| API                           | Purpose                                           |
|-------------------------------|---------------------------------------------------|
| `fgets()`                     | Read command from stdin                           |
| `stats_get()`                 | Retrieve latest stats for display                 |
| `debug_dump_list()`           | Request linked list print                         |
| `logger_clear()`              | Clear the list                                    |
| `vTaskGetRunTimeStats()`      | Per-task CPU usage string                         |
| `uxTaskGetStackHighWaterMark()` | Stack usage per task                            |
| `xPortGetFreeHeapSize()`      | Current free heap                                 |
| `xPortGetMinimumEverFreeHeapSize()` | Heap high-water mark                        |

### `debug_task.c`

| API                           | Purpose                                           |
|-------------------------------|---------------------------------------------------|
| `xQueueReceive(xPrintQueue)`  | Block until a PrintMsg arrives                    |
| `printf()` / `fflush()`      | ONLY place in the entire project that calls printf|
| `debug_log()`                 | Producer API: filter → vsnprintf → queue send     |
| `debug_dump_list()`           | Snapshot list → enqueue each entry as PrintMsg    |

---

## 13. Shell Commands

| Command         | Action                                                |
|-----------------|-------------------------------------------------------|
| `stats`         | Print latest min/max/avg/count/drops                  |
| `dump`          | Print all linked list entries via debug_dump_list()    |
| `clear`         | Clear the linked list                                 |
| `rate <ms>`     | Change sensor sample period (e.g., `rate 50`)         |
| `tasks`         | Print per-task CPU usage and stack high-water marks   |
| `loglevel <n>`  | Set log verbosity: 0=INFO, 1=WARN, 2=ERROR, 3=DUMP  |
| `help`          | List available commands                               |

---

## 14. Lock Ordering (Deadlock Avoidance)

One rule: **never hold two locks at once.**

Each task takes one mutex, does its work, releases, then optionally takes another.
No nesting = no deadlock possible.

- `xListMutex`: protects the linked list (logger, stats, shell, dump).
- `xStatsMutex`: protects the SensorStats struct (stats writer, shell reader).
- These two are never held simultaneously by any task.

---

## 15. Key Design Decisions & Rationale

| Decision                                          | Why                                                                  |
|---------------------------------------------------|----------------------------------------------------------------------|
| `vTaskDelayUntil` for sensor, `vTaskDelay` for stats | Sensor needs drift-free periodic sampling; stats doesn't          |
| Non-blocking queue send in sensor                  | Sensor must never miss its deadline waiting on downstream            |
| Mutex (not binary semaphore) for list              | Mutex has priority inheritance; binary semaphore does not            |
| PrintMsg embeds char array, not pointer            | Queue copies by value; pointer would dangle after sender returns    |
| Static node pool over pvPortMalloc                 | Deterministic, zero fragmentation, O(1) — the embedded standard    |
| Snapshot pattern for stats                         | Copy under lock, compute outside lock — minimizes lock hold time    |
| Single debug task owns printf                      | Prevents interleaved output, isolates slow I/O from real-time tasks |
| debug_log drops on full queue                      | Logging must never block a producer task                            |

---

## 16. Build Order (Incremental)

Build and test each step before proceeding:

1. **`main.c` + stub sensor task** — prints "tick" every 100ms. Confirm build and FreeRTOS scheduler runs.
2. **Add queue + logger task** — sensor sends, logger receives and prints. Queue IPC working.
3. **Add linked list inside logger** — mutex-protected append/evict. Bounded FIFO working.
4. **Add debug task + debug_log()** — remove all direct `printf`, route through print queue.
5. **Add stats task** — uses `logger_snapshot()` to compute min/max/avg.
6. **Add shell task** — interactive commands: stats, dump, clear, rate, tasks, help.
7. **Refactor to static memory pool** — replace pvPortMalloc with pool_alloc/pool_free.

---

## 17. Build & Run

```bash
# Clone FreeRTOS (if not already done)
git clone https://github.com/FreeRTOS/FreeRTOS.git --recurse-submodules

# Copy Posix demo as project template
cp -r FreeRTOS/FreeRTOS/Demo/Posix_GCC sensor_logger

# Add your src/ files, update Makefile source list

# Build
cd sensor_logger
make

# Run
./build/posix_demo

# Shell commands available at stdin
# Ctrl+C to exit
```

---

## 18. Future Enhancements

- Port to **QEMU** with a real ARM Cortex-M target (lm3s6965evb) for bare-metal UART I/O.
- Replace stdin shell with **UART ISR + stream buffer** pattern.
- Add **event groups** for state machine transitions.
- Implement **task notifications** as a lighter-weight IPC alternative.
- Add **software watchdog** — supervisor task monitors check-ins from all other tasks.
- Build a **lock-free SPSC ring buffer** between an ISR-like producer and the logger task.
