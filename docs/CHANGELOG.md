# Changelog

## Stage 2 Updates
- **GSM Hardware Recovery**: Added `gsm_Hardware_Reset()` tied to GPIO 15 (Active LOW) acting as the EC200U-CN `RESET_N` pin. If 7 consecutive TCP transmission failures occur, the ESP32 performs a hard reset pulse on the modem followed by a full software AT reconfiguration via `gsm_Init()`. The failure counter only resets to 0 upon a successfully acknowledged JSON payload transmission. Throttled post-7 recoveries using modulo offsets.
- **GSM Recovery**: Implemented a `consecutive_failures` counter in `gsmSendJSON()`. On 3 consecutive failures, it triggers `AT+QIDEACT=1` to force PDP context renegotiation. On 5 consecutive failures, it invokes `gsm_Init()` to fully reinitialize the modem software layer. Command timeouts were bounded (e.g. `QIOPEN` reduced from 30s to 15s).
- **GSM Diagnostic Counters**: `gsm_diag_tcp_failures`, `gsm_diag_pdp_recoveries`, `gsm_diag_soft_resets`, `gsm_diag_hard_resets`, and `gsm_diag_success_after_recovery` implemented in RAM to track escalation paths precisely.
- **Memory Safety**: `buildGSM_JSON()` was updated to pre-allocate its string buffer using `json.reserve(512)` to prevent Arduino `String` fragmentation. Added buffer bounding to `gsmSendAT()` using `resp.reserve(128)` and `.substring(256)` to prevent runaway heap expansion on unsolicited data dumps.
- **Web Auth Security**: Added `if (!isAuthenticated) { server.send(401, ...); return; }` across protected endpoints (`/set`, `/restart`, `/ota`, `/rtctime`, `/livedata`, `/gsmstatus`).
- **NVS Input Safety**: Enforced string length bounds on `device_id`, `gsmip`, `gsmport` and boundary constraints on `data_interval` and `fan_prestart` inside the `/set` handler. Resolved an unvalidated secondary assignment path.
- **RTC UI Update**: Added "Fetch System Time" button via a non-disruptive script inside `main.cpp` HTML literal, preserving original CSS and layout without immediately committing time to hardware.
- **Scheduler**: Addressed timer rollover bug in `timer.cpp` to correctly maintain offsets natively (`0ULL - (diff % THRESHOLD)`) without zeroing them arbitrarily causing delta skew.

## Stage 3 Updates
- **Sensor Data Validity**: To prevent the transmission of stale sensor readings indefinitely during a localized UART/I2C drop, added the `SensorValidFlags sensor_valid` global struct.
- **JSON Null Injection**: Modified `buildGSM_JSON()` to perform a ternary check against `sensor_valid.X` for each output key. If invalid, it explicitly transmits the JSON unquoted literal `null` instead of the persistently cached memory value from `weather_data`. Legitimate zeroes (e.g. `0.0f` UV) remain numeric.
- **Sensor Drivers**: Explicitly added `sensor_valid.X = true` on functional parses and `sensor_valid.X = false` when timeouts or CRC check failures execute inside `veml7700.cpp`, `ltr390.cpp`, `sds011.cpp`, `gas_sensor.cpp`, `bme680.cpp`, and `s300e.cpp`. Modifying one flag isolates to its own sensor telemetry without bringing down unrelated arrays.
