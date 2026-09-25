# Bug Register

| ID | Severity | Category | File | Function | Finding | Risk | Status | Verification |
| -- | -------- | -------- | ---- | -------- | ------- | ---- | ------ | ------------ |
| 01 | MEDIUM | Scheduler | `timer.cpp` | `onTimer()` | `RUNNING_TICK_COUNTER` resets to 0 but `LAST_*_TIME_VALUE` also resets to 0 blindly on threshold. | Drift | **FIXED** (Modulo offsets implemented) | VERIFIED BY STATIC ANALYSIS |
| 02 | MEDIUM | Refactor | `main.cpp` / `gsm.cpp` | `gsmSendJSON()` | Duplicated implementation of GSM logic. | Confusion | **FIXED** (`main.cpp` retained as active path) | VERIFIED BY STATIC ANALYSIS |
| 03 | HIGH | Memory | `main.cpp` | `buildGSM_JSON()` | Extensive repeated string concatenation. | Heap fragmentation / 2-3 day crash | **FIXED** (`json.reserve(512)` buffer pre-allocation) | VERIFIED BY BUILD |
| 04 | HIGH | Security | `main.cpp` | `handleSet()` | Potential buffer overflow in `server.arg()`. | NVS corruption | **FIXED** (Length checking bounds added before assigning configs) | VERIFIED BY STATIC ANALYSIS |
| 05 | MEDIUM | Sensor | `gas_sensor.cpp` | `gas_sensor_task()` | Hardcoded `delay()` calls. | Loop starvation | **DEFERRED** (No changes made to native sensor logic per explicit instructions) | NOT APPLICABLE |
| 06 | HIGH | Sensor | `main.cpp` / Sensors | global | Values are initialized to `0.0f` but there is no invalidation mechanism. | Transmitting stale data. | **FIXED** (`SensorValidFlags` outputs JSON `null` on failure) | VERIFIED BY STATIC ANALYSIS |
| 07 | HIGH | Security | `main.cpp` | multiple | Missing authentication validation on state endpoints. | Unauthorized changes | **FIXED** (`!isAuthenticated` checks on `/set`, `/restart`, `/ota`, etc.) | VERIFIED BY STATIC ANALYSIS |
| 08 | CRITICAL | Network | `main.cpp` | `gsmSendJSON()` | No context recovery on PDP failure. Leads to 2-3 day hard failure. | Permanent drop | **FIXED** (3-fail socket `QIDEACT`, 5-fail `gsm_Init()`, and 7-fail `RESET_N` GPIO 15 recovery added) | VERIFIED BY STATIC ANALYSIS (HARDWARE REQ) |
