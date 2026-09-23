#include "debug_serial.h"
#include "gsm.h"
#include "esp_task_wdt.h"           // Watchdog reset handling

// ----------------------------------------------------------------
// External weather data
// ----------------------------------------------------------------
extern weather_station_global_structure_t weather_data;

// ----------------------------------------------------------------
// UART2 GSM Serial
// ----------------------------------------------------------------
static HardwareSerial GSM_Serial(2);

// ----------------------------------------------------------------
static String device_id = "AQI_001";        // Device ID


// ----------------------------------------------------------------
// Flush GSM RX buffer
// Ensures old responses do not interfere with new AT commands

static void flushGsmRx()
{
    while (GSM_Serial.available())
        GSM_Serial.read();
}

// ----------------------------------------------------------------
// AT command helper function
// Sends AT command and waits for expected response
// ----------------------------------------------------------------
bool sendAT(String cmd, String expected, uint32_t timeout)
{
    flushGsmRx();

    // Send AT command only if command string is not empty
    if (cmd.length()){
        GSM_Serial.println(cmd);
    }

    uint32_t start = millis();
    String resp;
    esp_task_wdt_reset();

    // Wait for response until timeout
    while (millis() - start < timeout)
    {
        while (GSM_Serial.available())
        {
            char c = GSM_Serial.read();
            resp += c;

            // Check for expected response
            if (resp.indexOf(expected) >= 0)
            {
                USBSerial.println(resp);
                return true;
            }
        }
        esp_task_wdt_reset();
    }


    USBSerial.println(resp);       // Print full response on timeout
    return false;
}

// ----------------------------------------------------------------
// GSM + GPRS INITIALIZATION
// ----------------------------------------------------------------
bool gsmInit()
{
    // Basic AT communication check
    if (!sendAT("AT", "OK", 1000)) {
        return false;
    }

    sendAT("ATE0", "OK", 1000);             //  Disable echo
    sendAT("AT+CMEE=2", "OK", 1000);        // Enable verbose error messages
    sendAT("AT+CFUN=1", "OK", 2000);        // Set full functionality

    // SIM ready check
    if (!sendAT("AT+CPIN?", "READY", 5000)) {
        return false;
    }

    // Network registration check
    if (!sendAT("AT+CREG?", "0,1", 5000) && !sendAT("AT+CREG?", "0,5", 5000)) {
        return false;
    }

    sendAT("AT+CSQ", "OK", 2000);                                       // Signal quality
    sendAT("AT+CGATT=1", "OK", 5000);                                   // Attach to GPRS
    sendAT("AT+CGDCONT=1,\"IP\",\"" + String(APN) + "\"", "OK", 2000);  // Set APN
    sendAT("AT+QIREGAPP", "OK", 2000);                                  // Register GPRS application

    // Activate PDP context
    if (!sendAT("AT+QIACT", "OK", 10000)) {
        return false;
    }

    sendAT("AT+QILOCIP", ".", 3000);        // Query local IP

    return true;
}

// ----------------------------------------------------------------
// CHECK TCP CONNECTION STATUS
// ----------------------------------------------------------------
bool isTcpConnected()
{
    flushGsmRx();

    GSM_Serial.println("AT+QISTAT");

    uint32_t start = millis();
    String resp;

    while (millis() - start < 2000)
    {
        while (GSM_Serial.available())
        {
            resp += (char)GSM_Serial.read();

            if (resp.indexOf("CONNECT") >= 0 ||
                resp.indexOf("CONNECTED") >= 0)
            {
                USBSerial.println("TCP already connected");
                return true;
            }
        }
    }

    USBSerial.println(resp);
    return false;
}

// ----------------------------------------------------------------
// TCP CONNECTION SETUP
// ----------------------------------------------------------------
bool tcpConnect()
{
    // Check existing connection
    if (isTcpConnected())
    {
        return true;
    }

    // Ensure clean state
    sendAT("AT+QICLOSE", "OK", 3000);
    sendAT("AT+QIDEACT", "OK", 5000);

    // Build TCP open command
    String cmd = "AT+QIOPEN=\"TCP\",\"" +
                 String(SERVER_IP) + "\"," +
                 String(SERVER_PORT);

    if (sendAT(cmd, "CONNECT OK", 15000)){
        return true;
    }

    if (sendAT("", "ALREADY CONNECT", 3000)){
        return true;
    }

    return false;
}

// ----------------------------------------------------------------
// TCP DATA SEND
// ----------------------------------------------------------------
bool tcpSend(String payload)
{
    // Enter send mode
    if (!sendAT("AT+QISEND", ">", 3000)){
        return false;
    }

    // Send payload
    GSM_Serial.print(payload);
    GSM_Serial.write(0x1A);     // CTRL+Z indicates end of data

    return sendAT("", "SEND OK", 10000);
}

// ----------------------------------------------------------------
// TCP CLOSE
// ----------------------------------------------------------------
void tcpClose()
{
    sendAT("AT+QICLOSE", "OK", 5000);
    sendAT("AT+QIDEACT", "OK", 5000);
}

// ----------------------------------------------------------------
// Build JSON payload
// ----------------------------------------------------------------
String buildJSON()
{
    String json = "{";
    json += "\"device_id\":\"" + device_id + "\",";

    json += "\"CO_ppm\":"  + String(weather_data.CO_ppm, 2) + ",";
    json += "\"O3_ppm\":"  + String(weather_data.O3_ppm, 3) + ",";
    json += "\"NO2_ppm\":" + String(weather_data.NO2_ppm, 3) + ",";
    json += "\"H2S_ppm\":" + String(weather_data.H2S_ppm, 3) + ",";
    json += "\"SO2_ppm\":" + String(weather_data.SO2_ppm, 3) + ",";
    json += "\"NH3_ppm\":" + String(weather_data.NH3_ppm, 2) + ",";
    json += "\"CO2_ppm\":" + String(weather_data.CO2_ppm) + ",";

    json += "\"PM2_5\":"   + String(weather_data.pm2_5, 2) + ",";
    json += "\"PM10\":"    + String(weather_data.pm10, 2) + ",";

    json += "\"lux\":"     + String(weather_data.lux, 2) + ",";
    json += "\"uvi\":"     + String(weather_data.uvi, 2) + ",";

    json += "\"temp\":"    + String(weather_data.BME680_Temperature, 2) + ",";
    json += "\"hum\":"     + String(weather_data.BME680_Humidity, 2) + ",";
    json += "\"press\":"   + String(weather_data.BME680_Pressure, 2) + ",";

    json += "\"date\":"    + String(weather_data.rtc_date) + ",";
    json += "\"month\":"   + String(weather_data.rtc_month) + ",";
    json += "\"year\":"    + String(weather_data.rtc_year) + ",";
    json += "\"hour\":"    + String(weather_data.rtc_hour) + ",";
    json += "\"min\":"     + String(weather_data.rtc_min) + ",";
    json += "\"sec\":"     + String(weather_data.rtc_sec);

    json += "}";
    return json;
}

// ----------------------------------------------------------------
// GSM setup
// ----------------------------------------------------------------
void gsmSetup()
{
    GSM_Serial.begin(115200, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
    delay(3000);

    USBSerial.println("Initializing GSM...");
    if (!gsmInit()){
        USBSerial.println("GSM Init Failed");
    }
    else {
        USBSerial.println("GSM Ready");
    }
}

// ----------------------------------------------------------------
// GSM process loop
// ----------------------------------------------------------------
void gsmProcessLoop()
{
    String payload = buildJSON();

    USBSerial.println("Connecting TCP...");
    if (tcpConnect())
    {
        USBSerial.println("Sending Data:");
        USBSerial.println(payload);

        if (tcpSend(payload))
        {
            USBSerial.println("Upload Success");
        } else {
            USBSerial.println("Send Failed");
        }
        tcpClose();
    }
    else
    {
        USBSerial.println("TCP Connect Failed");
    }
}
