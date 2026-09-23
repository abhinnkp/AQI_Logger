# GSM Long-Duration Analysis

## 1. Existing GSM Architecture
The project possesses two locations for GSM-related functions:
1. `src/main.cpp`: Implements `gsmSendJSON()`, `gsmSendAT()`, `gsm_Init()`, `gsm_DeInit()`, and `buildGSM_JSON()`.
2. `src/gsm.cpp`: Implements `tcpConnect()`, `tcpSend()`, `tcpClose()`, `sendAT()`, and `buildJSON()`.

## 2. Active GSM Implementation
**Confirmed from source code:** The active telemetry path for GSM calls `gsmSendJSON()` in `main.cpp` exclusively.

## 3. Duplicate / Legacy Implementation Analysis
**Confirmed from source code:** `src/gsm.cpp` is effectively dead code. `gsmProcessLoop()` from `gsm.cpp` is never invoked in `loop()`. The active connection path is hardcoded into `gsmSendJSON()` inside `main.cpp`.

## 4. Complete GSM Lifecycle (Active Path)
The active connection lifecycle on transmission is:
1. `AT+QIOPEN=1,0,"TCP",...` -> Waits up to 15s (was 30s).
2. `AT+QISEND=0` -> Waits for `>` prompt up to 3s (was a blind 300ms delay).
3. Payload sent -> Waits up to 10s for `SEND OK`.
4. `AT+QICLOSE=0` -> Closes socket.

## 5. Current Failure Paths (Pre-Stage 2)
- Modems often return an asynchronous `+QIOPEN: 0,0` some time after acknowledging the `AT+QIOPEN` command with `OK`. The pre-Stage-2 code blocked up to 30s waiting for `+QIOPEN: 0,0`. If it missed it, it aborted without tracking failure.
- Missing `AT+QIDEACT` or full reset mechanisms if the PDP context dropped.
- Memory leak via `String` fragmentation.
- UART unbounded reading in `gsmSendAT()` if modem dumped unsolicited data.

## 6. Suspected Causes of 2–3 Day Failure
1. **Memory Fragmentation (Strongly Suspected):** `buildGSM_JSON()` creates and discards large `String` objects every 30 seconds. Over 2-3 days, heap fragmentation could crash the ESP32 (causing the observed reboot).
2. **PDP Context Loss Without Recovery (Strongly Suspected):** Cellular networks typically drop inactive or long-lived data contexts every 24-72 hours. When this happens, `AT+QIOPEN` fails. Because `main.cpp` had no logic to reset the PDP context (`AT+QIDEACT`) or reboot the modem organically, `AT+QIOPEN` would fail forever until physical reset.

## 7. Evidence
- Line 403 in original `main.cpp` showed the `AT+QIOPEN` logic without PDP context recovery mechanics.
- `buildGSM_JSON()` used `json += "..."` over 20 times per call without `reserve()`.

## 8. Changes Made in Stage 2
1. **Memory**: Pre-allocated the `String` buffer using `json.reserve(512)` to mitigate heap fragmentation. Injected a 128-byte bound buffer on `gsmSendAT()` to handle UART dump exhaustion.
2. **GSM State Tracking**: Tracked consecutive transmission failures via `static int consecutive_failures`.
3. **Layered Recovery Strategy**:
   - Level 1: TCP/Socket Recovery. Occurs organically every cycle.
   - Level 2: PDP Recovery. At 3 consecutive failures, `AT+QIDEACT=1` is sent.
   - Level 3: Software Re-initialization. At 5 consecutive failures, `gsm_Init()` is called to reset UART and AT configs.
   - Level 4: Hardware `RESET_N`. At 7 consecutive failures, GPIO 15 is cycled LOW for 200ms to trigger a hard EC200U reset. The counter is only reset back to 0 once a successful telemetry payload hits the server, ensuring failed initializations don't mask permanent network failures.

## 9. Recovery Strategy Flow
```text
Transmission failure
        ↓
Retry current operation (next cycle)
        ↓
3 consecutive failures -> Force AT+QIDEACT=1
        ↓
5 consecutive failures -> Call gsm_Init()
        ↓
7 consecutive failures -> Call gsm_Hardware_Reset(), cycle GPIO 15, then gsm_Init()
        ↓
Resume normal operation
```

## 10. Verification
**VERIFIED BY STATIC ANALYSIS AND BUILD**.

## 11. Recommended Field Test Procedure
1. Flash stabilized firmware.
2. Confirm GSM registration and first successful transmission.
3. Run continuously for minimum 72 hours, preferably 7 days.
4. Monitor `consecutive_failures` output via serial when failure occurs.
5. Verify the device organically recovers via the Level 2/3/4 escalations.
6. Specifically monitor if Level 4 Hardware Reset is invoked during network drops and successfully brings the TCP socket back online after 5 seconds of boot delay.
