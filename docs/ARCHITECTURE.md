# Architecture Document

## A. Firmware Architecture
```text
ESP32-S3
   |
   +-- Sensors
   |     |-- BME680 (I2C)
   |     |-- VEML7700 (I2C)
   |     |-- LTR390 (I2C)
   |     |-- S300E (UART/I2C bridge)
   |     |-- SDS011 (UART)
   |     +-- Multi-Gas (UART + GPIO Mux)
   |
   +-- GSM / EC200U-CN (UART + GPIO 15 RESET_N)
   |
   +-- W5500 Ethernet (SPI)
   |
   +-- Web UI (AsyncWebServer / HTML literals)
   |
   +-- RTC (DS3231 I2C)
   |
   +-- OTA (ArduinoOTA)
   |
   +-- Scheduler (Hardware Timer ISR)
   |
   +-- Watchdog (esp_task_wdt)
```

## B. GSM Recovery Architecture
```text
TCP failure
    ↓
recovery escalation
    ↓
PDP recovery (AT+QIDEACT=1)
    ↓
software GSM recovery (gsm_Init)
    ↓
EC200U RESET_N (GPIO 15)
```

## C. Sensor Validity Architecture
```text
sensor acquisition
    ↓
success / failure
    ↓
SensorValidFlags (true/false)
    ↓
buildGSM_JSON()
    ↓
numeric / null
```

## D. Data Flow
```text
Sensors
   ↓
weather_data
   +
SensorValidFlags
   ↓
JSON
   ↓
GSM / Ethernet
```
