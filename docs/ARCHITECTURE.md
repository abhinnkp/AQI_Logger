# Architecture Document

## 1. Hardware Architecture
- Board: ESP32-S3-DevKitC-1
- Peripherals:
  - BME680 (Temperature, Humidity, Pressure)
  - VEML7700 (Ambient Light)
  - LTR390 (UV)
  - S300E (CO2 via I2C)
  - SDS011 (PM sensor via UART)
  - Multi-Gas Sensor (Multiplexed via UART/GPIO)
  - TB600B (Uncertain usage, possibly another gas sensor via UART)
  - W5500 Ethernet Module (SPI)
  - GSM Module (SIMxxx over UART)
  - DS3231 RTC (I2C)
  - Fan Control (GPIO)
  - WS2812 NeoPixel (Status indicator)

## 2. Software Architecture
- Framework: Arduino for ESP32
- Execution flow:
  - `setup()` initializes all peripherals, loads configurations from NVS, initializes communication links (GSM, Ethernet, WiFi), starts the Web Server, and configures the hardware timer.
  - `loop()` checks for timer flags, handles sensor acquisition in specific intervals, manages GSM/TCP transmission, serves Web Server client requests, and handles the Watchdog Timer (WDT) and OTA.
- Scheduler: Hardware timer interrupt triggers every 1ms. The ISR `onTimer()` maintains `RUNNING_TICK_COUNTER` and sets flags (`FIVE_SECOND_ELAPSED_FLAG`, `TEN_SECOND_ELAPSED_FLAG`, etc.). The main loop checks these flags and performs tasks. A fallback rollover mechanism at ~10 minutes resets counters.
- Global State: `weather_data` (`weather_station_global_structure_t`) stores all current sensor readings and RTC data.

## 3. Communication Flow
- Sensor Data to Server: Device creates JSON payload of `weather_data` and attempts to send it over Ethernet (W5500) and/or GSM. No apparent failover logic; both try to initialize independently.
- Local Configuration: Web Server running on WiFi (Access Point / Station mode depending on config). It exposes endpoints for configuring Network and general settings.
- OTA Update: Uses `ArduinoOTA` or a custom OTA component depending on configuration. Need to analyze `ota.cpp`.

## 4. Boot Sequence
1. UART debug initialized
2. Fan and W5500 Reset pins initialized
3. Sensors initialized via I2C and UART (GSM, BME, VEML, LTR, SDS011, Gas)
4. Network connections initialized
5. Timer started
6. Watchdog activated

## 5. Risk Areas (Initial View)
- Timer rollover implementation. ISR rollover sets `LAST_*_TIME_VALUE` to 0 but `RUNNING_TICK_COUNTER` also to 0. It may lose a tick or cause uneven scheduling during rollover.
- Global variable safety without mutexes/critical sections between main loop and ISR.
- String concatenation and buffer vulnerabilities when parsing GSM or creating JSON payloads.
- Web Server missing authentication on configuration modification endpoints (`/set`).

## 6. Communication Modules Audit
- `gsm.cpp` / `gsm.h`: Exists as a standalone module for GSM/TCP, but `main.cpp` has its own redundant implementation of `gsmSendJSON()`.
- Ethernet (`W5500` via SPI): Managed within `main.cpp` directly rather than a separate `ethernet.cpp`. Relies on `Ethernet_Generic.h`.
- No Failover: `communicationMode` variable selects between GSM and Ethernet based on configuration.
- AQI Calculation: Neither `data_structure.h` nor any apparent module performs an AQI calculation. PM data, Gas data, etc., are transmitted raw.

## 7. Sensors Audit
- Drivers for Gas, S300E, BME680, LTR390, VEML7700, SDS011 exist in `/app/src/`.
- Communication to sensors is scattered between UART (HardwareSerial) and I2C.
- `weather_data` variable is used to ferry this data.
- The fan is turned on for `FAN_PRE_START_TIME` seconds, then data is acquired, payload is sent, and fan is turned off.

## 8. Web UI Audit
- Hosted in `main.cpp`.
- Implements `WebServer server(80)`.
- Uses `index_html[]` generated via standard HTML/CSS string concatenation.
- Endpoints apply changes via POST params and use `Preferences` (NVS) for storage.
- UI styling must NOT be touched. We should only fix buffer handling inside endpoint handlers if unsafe `String` operations are found.

## 9. Hardware Validations Required
- `Ethernet_Generic.h`: W5500 via SPI requires actual hardware to verify network behavior.
- `gsm.cpp`: Uses UART `AT` commands.
- Sensors (BME680, S300E, VEML7700, LTR390, Gas): Some via I2C (`i2c_20k.h`), some via UART.
- DS3231 RTC: Uses standard I2C.
- Fan Control: Uses GPIO 42.
