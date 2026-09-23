# Sensor Validity Analysis

## 1. Current Failure Behavior
The global structure `weather_data` (`weather_station_global_structure_t` in `data_structure.h`) is initialized once to `0.0f` (or `0`) at boot via `init_weather_station_global_structure()`.

When the main loop's scheduler flags are triggered (5s, 10s, 30s), functions like `veml7700_task()`, `read_LTR390()`, `BME680_Process()`, `sds011_task()`, `gas_sensor_task()`, and `S300E_data()` attempt to read their respective physical interfaces (I2C/UART).

If a read fails (e.g. `LTR390_I2C_OK` is not returned, or the CRC fails on SDS011), the driver outputs a failure string to `USBSerial` (e.g., `"Failed to read ALS data"`) and **simply returns**.

## 2. Exact Source Causing the Problem
Because the driver merely returns early, it does not update the field inside `weather_data` corresponding to that sensor. The JSON builder (`buildGSM_JSON()`) unconditionally reads `weather_data`. Therefore:
1. If the sensor has *never* succeeded since boot, it transmits `0.0f`.
2. If the sensor succeeded previously but has disconnected, it transmits the **last known valid value infinitely**, creating completely stale data that the backend interprets as legitimate static ambient conditions.

## 3. Proposed Minimal Correction
Instead of restructuring `weather_data` to hold boolean `valid` flags per-sensor (which would require rewriting `buildGSM_JSON` entirely and potentially break backend assumptions if fields are omitted), we can use a **sentinel value array** or implement an "Age" counter per sensor.

However, the cleanest and most backward-compatible approach for telemetry drops is to **write `NAN` (Not A Number)** to the float fields inside `weather_data` if a read fails. `String(NAN)` yields `"nan"` in the JSON payload natively, which most REST backends naturally parse as a nullified float, or we can catch it in `buildGSM_JSON()` and format it specifically. Alternatively, setting them to `-999.0f` is an industry standard for invalid sensor data without breaking JSON integer parsers.

I propose modifying `buildGSM_JSON()` to check a parallel `sensor_timeout` struct. If a sensor hasn't been successfully updated within `X` cycles, we inject `null` or `-999.0` into the JSON string. Or, even simpler, the sensor drivers themselves overwrite `weather_data.xxx = -999.0f;` upon returning an error.

*Effect on weather_data:* Drivers update failure states into the variables.
*Effect on JSON:* The JSON transmits the explicit error bound.
*Effect on Backend:* Backend ignores `-999.0f` or handles it as missing data, instead of storing a perfectly flat `24.5 °C` for three days straight.


## 4. User Approval Wait
I will stop here and request the user's preference on how they want the JSON formatted on disconnect (`null`, `"nan"`, or `-999.0`). I lean heavily toward `-999.0` because it avoids needing any JSON builder logic changes (it simply passes natively through `String(float)`).
