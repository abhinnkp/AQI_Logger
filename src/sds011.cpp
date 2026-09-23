struct SensorValidFlags {
    bool bme680;
    bool s300e;
    bool sds011;
    bool ltr390;
    bool veml7700;
    bool gas;
};
extern SensorValidFlags sensor_valid;
#include "debug_serial.h"
#include "sds011.h"
#include "data_structure.h"
#include "esp_task_wdt.h"

#define SDS011_SENSOR_TIMEOUT_MS     10000UL
#define SDS011_MAX_FAIL_STREAK       5
#define SDS011_WARMUP_TIME_MS        30000UL

extern weather_station_global_structure_t weather_data;

// ============================================================
// UART OBJECT
// ============================================================
static HardwareSerial PM_Serial(1);

// ============================================================
// SDS011 STATUS
// ============================================================

// Total number of invalid/no-data checks
uint32_t readFails = 0;

// Consecutive invalid packet count
uint8_t failStreak = 0;

// Time when the last VALID SDS011 packet was received
static uint32_t lastValidPacketTime = 0;

// Sensor status
static bool sds011SensorOK = false;

// Last time we printed sensor status
static uint32_t lastStatusPrint = 0;

// ============================================================
// SEND COMMAND
// ============================================================
void sendCommand(uint8_t *cmd, uint8_t len)
{
    PM_Serial.write(cmd, len);
}

// ============================================================
// WAKEUP
// ============================================================
bool startMeasurement()
{
    uint8_t cmd[] = {
        0xAA, 0xB4, 0x06, 0x01, 0x01,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF, 0xFF, 0x06, 0xAB
    };

    size_t sent = PM_Serial.write(cmd, sizeof(cmd));

    return (sent == sizeof(cmd));
}

// ============================================================
// SLEEP
// ============================================================
bool stopMeasurement()
{
    uint8_t cmd[] = {
        0xAA, 0xB4, 0x06, 0x01, 0x00,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF, 0xFF, 0x05, 0xAB
    };

    size_t sent = PM_Serial.write(cmd, sizeof(cmd));

    return (sent == sizeof(cmd));
}

// ============================================================
// RESET / RECOVERY
// ============================================================
static void restartSDS011()
{
    USBSerial.println();
    USBSerial.println(F("================================"));
    USBSerial.println(F("SDS011 RECOVERY"));
    USBSerial.println(F("Restarting PM sensor..."));
    USBSerial.println(F("================================"));

    // Stop measurement
    stopMeasurement();
    delay(200);
    // Clear old UART data
    while (PM_Serial.available())
    {
        PM_Serial.read();
    }

    // Start measurement again
    if (startMeasurement())
    {
        USBSerial.println(F("SDS011 wake-up command sent."));
    }
    else
    {
        USBSerial.println(F("SDS011 wake-up command FAILED."));
    }

    // Do NOT delay 20 seconds here.
    //
    // Sensor will continue warming/measuring while
    // the main loop continues running.

    failStreak = 0;

    // We don't immediately call it OK.
    sds011SensorOK = false;

    // Reset timeout timer
    lastValidPacketTime = millis();

    USBSerial.println(F("SDS011 recovery complete."));
}

// ============================================================
// READ DATA
// ============================================================
bool readMeasurement()
{
    static uint8_t buffer[10];

    // --------------------------------------------------------
    // Process every complete 10-byte packet available
    // --------------------------------------------------------
    while (PM_Serial.available() >= 10)
    {
        // ----------------------------------------------------
        // Synchronize to 0xAA
        // ----------------------------------------------------
        if (PM_Serial.peek() != 0xAA)
        {
            PM_Serial.read();
            continue;
        }

        // ----------------------------------------------------
        // Read complete packet
        // ----------------------------------------------------
        size_t received = PM_Serial.readBytes(buffer, 10);

        if (received != 10)
        {
            return false;
        }

        // ----------------------------------------------------
        // Packet format validation
        //
        // [0]  AA
        // [1]  C0
        // [2]  PM2.5 low
        // [3]  PM2.5 high
        // [4]  PM10 low
        // [5]  PM10 high
        // [6]  reserved
        // [7]  reserved
        // [8]  checksum
        // [9]  AB
        // ----------------------------------------------------
        if (buffer[0] != 0xAA ||
            buffer[1] != 0xC0 ||
            buffer[9] != 0xAB)
        {
            failStreak++;

            continue;
        }

        // ----------------------------------------------------
        // Checksum
        // ----------------------------------------------------
        uint8_t checksum = 0;

        for (int i = 2; i <= 7; i++)
        {
            checksum += buffer[i];
        }

        if (checksum != buffer[8])
        {
            failStreak++;
            continue;
        }

        // ----------------------------------------------------
        // VALID PACKET
        // ----------------------------------------------------
        uint16_t pm25_raw =
            ((uint16_t)buffer[3] << 8) |
            buffer[2];

        uint16_t pm10_raw =
            ((uint16_t)buffer[5] << 8) |
            buffer[4];

        // ----------------------------------------------------
        // Update weather data
        // ----------------------------------------------------
        weather_data.pm1   = 0;
        weather_data.pm2_5 = pm25_raw / 10.0f;
        weather_data.pm4   = 0;
        weather_data.pm10  = pm10_raw / 10.0f;
        sensor_valid.sds011 = true;

        weather_data.nc0_5       = 0;
        weather_data.nc1_0       = 0;
        weather_data.nc2_5       = 0;
        weather_data.nc4_0       = 0;
        weather_data.nc10        = 0;
        weather_data.typicalSize = 0;

        // ----------------------------------------------------
        // Update status
        // ----------------------------------------------------
        lastValidPacketTime = millis();

        sds011SensorOK = true;

        failStreak = 0;

        return true;
    }

    return false;
}

// ============================================================
// INIT
// ============================================================
void sds011_init()
{
    USBSerial.println(F("Starting SDS011..."));

    PM_Serial.begin(
        SDS011_BAUD,
        SERIAL_8N1,
        PM_RX_PIN,
        PM_TX_PIN
    );

    // Clear any garbage already present
    while (PM_Serial.available())
    {
        PM_Serial.read();
    }

    // Wake sensor
    if (startMeasurement())
    {
        USBSerial.println(F("SDS011 wake-up command sent."));
    }
    else
    {
        USBSerial.println(F("SDS011 wake-up command FAILED."));
    }

    USBSerial.println(F("Warm-up 30 sec..."));

    // --------------------------------------------------------
    // IMPORTANT:
    //
    // Your original code had delay(FIRST_WARMUP_TIME).
    // If FIRST_WARMUP_TIME = 30 sec, this blocks setup only.
    //
    // Keeping it here is acceptable if you want startup
    // warm-up before normal operation.
    // --------------------------------------------------------
    delay(SDS011_WARMUP_TIME_MS);

    // Clear any old bytes after warm-up
    while (PM_Serial.available())
    {
        PM_Serial.read();
    }

    // Start timeout from this point
    lastValidPacketTime = millis();

    sds011SensorOK = false;
    failStreak = 0;
    readFails = 0;

    USBSerial.println(F("SDS011 initialization complete."));
}

// ============================================================
// SDS011 TASK
// ============================================================
//
// Call this function continuously from loop().
//
// This function:
// 1. Reads UART
// 2. Updates last valid packet time
// 3. Detects sensor timeout
// 4. Performs recovery if necessary
//
// ============================================================
void sds011_task()
{
    // --------------------------------------------------------
    // Try to read a valid packet
    // --------------------------------------------------------
    bool packetReceived = readMeasurement();

    if (packetReceived)
    {
        // Valid packet already handled inside readMeasurement()
        return;
    }

    // --------------------------------------------------------
    // No valid packet received this call
    // --------------------------------------------------------

    // If UART contains data but it was invalid,
    // count it as a failure.
    if (PM_Serial.available() > 0)
    {
        readFails++;

        if (failStreak < 255)
        {
            failStreak++;
        }
    }

    // --------------------------------------------------------
    // SENSOR TIMEOUT
    // --------------------------------------------------------
    //
    // Do NOT declare failure immediately when there is no
    // packet. SDS011 sends data periodically.
    //
    // Only declare failure when NO VALID PACKET has arrived
    // for 10 seconds.
    // --------------------------------------------------------
    if (millis() - lastValidPacketTime >
        SDS011_SENSOR_TIMEOUT_MS)
    {
        if (sds011SensorOK)
        {
            USBSerial.println();
            USBSerial.println(F("SDS011: NO VALID DATA"));
            USBSerial.println(F("SDS011: Sensor may be disconnected."));

            sds011SensorOK = false;
        }

        // ----------------------------------------------------
        // Clear PM values
        //
        // This is important for your current problem.
        // Otherwise the previous value remains forever.
        // ----------------------------------------------------
        weather_data.pm1   = 0;
        weather_data.pm2_5 = 0;
        weather_data.pm4   = 0;
        weather_data.pm10  = 0;
        sensor_valid.sds011 = false;

        weather_data.nc0_5       = 0;
        weather_data.nc1_0       = 0;
        weather_data.nc2_5       = 0;
        weather_data.nc4_0       = 0;
        weather_data.nc10        = 0;
        weather_data.typicalSize = 0;
    }

    // --------------------------------------------------------
    // AUTO RECOVERY
    // --------------------------------------------------------
    //
    // Only recover if we have had repeated bad packets.
    //
    // No valid packet for 30 seconds -> restart sensor
    // --------------------------------------------------------
    if (!sds011SensorOK &&
        (millis() - lastValidPacketTime > 30000UL))
    {
        restartSDS011();
    }
}

// ============================================================
// GET SENSOR STATUS
// ============================================================
bool sds011_is_connected()
{
    return sds011SensorOK;
}

// ============================================================
// PRINT CURRENT DATA
// ============================================================
void sds011_print()
{
    USBSerial.println(F("----- SDS011 STATUS -----"));

    if (sds011SensorOK)
    {
        USBSerial.println(F("SDS011: CONNECTED"));

        USBSerial.print(F("PM2.5: "));
        USBSerial.print(weather_data.pm2_5, 2);
        USBSerial.println(F(" ug/m3"));

        USBSerial.print(F("PM10 : "));
        USBSerial.print(weather_data.pm10, 2);
        USBSerial.println(F(" ug/m3"));
    }
    else
    {
        USBSerial.println(F("SDS011: DISCONNECTED / NO VALID DATA"));

        USBSerial.println(F("PM2.5: 0.00 ug/m3"));
        USBSerial.println(F("PM10 : 0.00 ug/m3"));
    }

    USBSerial.print(F("Read fails: "));
    USBSerial.println(readFails);

    USBSerial.print(F("Fail streak: "));
    USBSerial.println(failStreak);

    USBSerial.println(F("-------------------------"));
}