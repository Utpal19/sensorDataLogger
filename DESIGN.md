Project 1: Sensor Data Logger (your first one)

This hits every core RTOS concept in one project. Build this first before anything else.

Requirements

    Simulate a temperature sensor sampling every 100ms
    Log readings with timestamps
    Compute rolling statistics (min/max/avg over last 50 samples)
    A "shell" task reads commands from stdin: stats, dump, clear, rate <ms>
    Handle sensor "faults" (occasional bad readings) gracefully

Architecture

Four tasks with distinct priorities:

    Sensor Task (priority 4, highest) — every 100ms wakes via vTaskDelayUntil, generates a fake reading (20.0 + (rand()%100)/10.0), packages it into a SensorReading struct with timestamp, sends to queue via xQueueSend. Non-blocking send — if queue full, increment a drop counter.
    Logger Task (priority 3) — blocks on xQueueReceive, appends reading to a linked list (capped at 100 entries, drops oldest). Takes a mutex before touching the list.
    Stats Task (priority 1, lowest) — every 2 seconds, takes the mutex, walks the list, computes min/max/avg, releases mutex, stores results in a shared stats struct (protected by a separate mutex or just atomic reads for scalar values).
    Shell Task (priority 2) — reads stdin, parses commands, prints stats or dumps log. Uses the same list mutex.

Sync primitives used

    1 Queue (sensor → logger), size 10
    1 Mutex protecting the linked list (use xSemaphoreCreateMutex — it has priority inheritance, important!)
    1 Binary semaphore or event group for "clear log" signal from shell to logger

Key learning moments

    Use vTaskDelayUntil not vTaskDelay in the sensor task — teaches you the difference between periodic and relative delays (critical for real sensor sampling).
    Deliberately give the stats task the lowest priority, then observe what happens when the sensor rate is too high — you'll see the stats task starving. This is priority inversion territory.
    Use configUSE_TRACE_FACILITY and uxTaskGetSystemState to print task runtime stats. Real firmware debugging skill.
    Try removing the mutex and running under valgrind or TSan — watch the race conditions surface.

Files

main.c              - task creation, FreeRTOS start
sensor_task.c/h     - simulated sensor
logger_task.c/h     - linked list + logger
stats_task.c/h      - stats computation
shell_task.c/h      - command parser
reading.h           - SensorReading struct, shared types
FreeRTOSConfig.h    - tune heap size, tick rate, priorities

I personally added a debug_log instead of printf.

Start with just the sensor task printing to stdout. Then add the queue and logger. Then stats. Then shell. Incremental — don't write all four at once.
