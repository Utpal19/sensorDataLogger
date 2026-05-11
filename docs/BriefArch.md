Requirements

Simulate a temperature sensor sampling every 100ms
Log readings with timestamps
Compute rolling statistics (min/max/avg over last 50 samples)
A "shell" task reads commands from stdin: stats, dump, clear, rate <ms>
Handle sensor "faults" (occasional bad readings) gracefully

Architecture
Four tasks with distinct priorities:

    1. Sensor Task (priority 4, highest) — every 100ms wakes via vTaskDelayUntil, generates a fake reading (20.0 + (rand()%100)/10.0), packages it into a SensorReading struct with timestamp, sends to queue via xQueueSend. Non-blocking send — if queue full, increment a drop counter.

    2. Logger Task (priority 3) — blocks on xQueueReceive, appends reading to a linked list (capped at 100 entries, drops oldest). Takes a mutex before touching the list.
    
    3. Stats Task (priority 1, lowest) — every 2 seconds, takes the mutex, walks the list, computes min/max/avg, releases mutex, stores results in a shared stats struct (protected by a separate mutex or just atomic reads for scalar values).
    
    4. Shell Task (priority 2) — reads stdin, parses commands, prints stats or dumps log. Uses the same list mutex.

Sync primitives used

1 Queue (sensor → logger), size 10
1 Mutex protecting the linked list (use xSemaphoreCreateMutex — it has priority inheritance, important!)
1 Binary semaphore or event group for "clear log" signal from shell to logger.