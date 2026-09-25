
/******************************************************************************
 *  Copyright (c) 2025-2026 Immensystech Private Limited. All rights reserved.
 *
 *  This software is confidential and proprietary to Immensystech Private Limited.
 *  Unauthorized copying, reproduction, modification, distribution, disclosure,
 *  or use of this software, in whole or in part, is strictly prohibited without
 *  prior written permission from Immensystech Private Limited.
 *
 *  Project     : Air Quality Monitoring (AQI) Weather Station Firmware
 *  Module      : Main Application
 *  File        : main.cpp
 *
 *  Description :
 *    Main application firmware for the ESP32-based Air Quality Monitoring
 *    System. This module initializes hardware peripherals, environmental
 *    sensors, GSM, Ethernet, Wi-Fi, OTA services, RTC, web server, and
 *    periodic task scheduling. It acquires sensor data, manages network
 *    communication, hosts the configuration web interface, and transmits
 *    environmental data to remote servers over GSM and Ethernet.
 *
 *  Authors:
 *    - YASH YADAV (Firmware Development)
 *
 *  Created     : 2025
 *  Last Updated: 2026
 *
 ******************************************************************************/


//==============================================================================
// INCLUDES & LIBRARIES
//==============================================================================
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>                                    //RGB Light controll
#include "debug_serial.h"                                         // Debug serial configuration
#include <timer.h>                                                // Hardware timer operation
#include "esp_task_wdt.h"                                         // ESP Task Watchdog Timer
#include <esp_intr_alloc.h>
#include <driver/uart.h>
#include "soc/rtc.h"                                              // APB/AHB clock frequency operations
#include "driver/i2c.h"                                           // I2C driver interface
#include <Preferences.h>                                          // NVS Non-Volatile Storage support
#include <Ethernet_Generic.h>                                     // W5500 Ethernet communication
#include <WebServer.h>                                            // Local configuration web server
#include <WiFi.h>                                                 // ESP32 Wi-Fi interface
#include <HardwareSerial.h>                                       // Hardware UART interface
#include <rtc_s.h>                                                // Real-Time Clock (RTC) library
#include "i2c_20k.h"                                              // Custom I2C communication library
#include <ota.h>                                                  // Firmware Over-The-Air (OTA) support
#include "tb600b.h"                                               // TB600B Gas sensor driver(EC SENSE)
#include <gas_sensor.h>                                           // UART Multiplexer gas sensor interface
#include <veml7700.h>                                             // VEML7700 Ambient light sensor
#include <ltr390.h>                                               // LTR390 UV radiation sensor
#include <sds011.h>                                               // SDS011 Particulate Matter (PM) sensor
#include <s300e.h>                                                // S300E Carbon Dioxide (CO2) sensor
#include <bme680.h>                                               // BME680 Environmental sensor (Temp, Hum, Press, Gas)
#include "device_restart_info.h"

//==============================================================================
// PIN DEFINITIONS & HARDWARE CONSTANTS
//==============================================================================
// GSM (M66/EC200U/Quectel LTE) UART Pins
#define GSM_RX_PIN              43
#define GSM_TX_PIN              44

// W5500 Ethernet SPI Control Pins
#define W5500_CS                10
#define W5500_RST               7

// Device Identification
//#define DEVICE_ID               "AQI_011"

// Access Point / Wi-Fi Credentials for Web Management
#define WIFI_SSID               "Weather_Station_IOT"
#define WIFI_PASSWORD           "Weather_Station_IOT"

// Default GSM Network APN
#define GSM_APN                 "airtelgprs.com"

// Fan configuration
#define FAN_PIN 42

// OTA Configuration
const unsigned long OTA_TIMEOUT_PERIOD = 90000U;                  // 90 seconds timeout before reverting to normal mode

//==============================================================================
// DATA STRUCTURES & ENUMS
//==============================================================================
/**
 * @brief Communication interface modes supported by the system.
 */
enum CommunicationMode
{
    COMM_GSM      = 0,  // Cellular GSM / LTE Mode
    COMM_ETHERNET = 1   // Hardwired Ethernet (W5500) Mode
};

/**
 * @brief Tracking structure for GSM modem runtime status and diagnostic telemetry.
 */
typedef struct
{
    bool    power_on;
    bool    sim_ready;
    bool    network_registered;
    bool    gprs_attached;
    bool    tcp_connected;
    int     rssi;
    String  last_error;
    String  last_tx_time;
} gsm_status_t;

//==============================================================================
// GLOBAL OBJECTS & VARIABLES
//==============================================================================
String DEVICE_ID = "AQI_1";
HardwareSerial                      gsm(0);                                 // Hardware Serial for GSM Module
EthernetClient                      ethernetclient;                         // Ethernet Client instance
WebServer                           server(80);                             // HTTP Web Server on port 80
extern Preferences                  prefs;                                  // NVS Preferences instance
ssl_timer_t                         ssl_timer;                              // Software interval timer structure
hw_timer_t *timer                   = NULL;                                 // Hardware Timer pointer
Preferences                         prefs;                                  // NVS storage
weather_station_global_structure_t  weather_data;                           // Aggregated weather and sensor metrics
gsm_status_t                        gsm_status;                             // GSM diagnostic status

// Network IP & Port Configurations (Default fallback values)
IPAddress   serverIP(192, 168, 1, 100);                                     // TCP Server Destination IP (Ethernet)
uint16_t    serverPort = 1234;                                              // TCP Server Destination Port (Ethernet)
//String    gsmServerIP   = "103.49.103.72";                                //(Davangere Server)
String      gsmServerIP   = "182.237.15.204";                               // Remote Server IP (GSM)
String      gsmServerPort = "5059";                                         // Remote Server Port (GSM)

// Default Static Ethernet Configuration
IPAddress   ip(192, 168, 1, 222);
IPAddress   gateway(192, 168, 1, 1);
IPAddress   subnet(255, 255, 255, 0);
// EC200U Hardware Reset Pin
#define GSM_RESET_PIN 15

// Hardware MAC Buffer
byte        mac[6];
String      ESP_MAC;

// Application Control Flags & Variables
CommunicationMode communicationMode = COMM_GSM;                             // Active Communication Mode
bool        isWiFiConnected         = false;                                // Wi-Fi Connection state
bool        webServerStarted        = false;                                // Web Server running state
bool        OTA_ENABLE_FLAG         = false;                                // OTA Process active trigger flag
bool        isAuthenticated         = false;                                // User authentication state
int         DATA_INTERVAL           = 1;                                    // Telemetry transmission interval (in minutes)
int         COUNTER_30SEC           = 0;                                    // 30-second interval cycle counter
// GSM Recovery Diagnostic Counters
uint32_t    gsm_diag_tcp_failures   = 0;
uint32_t    gsm_diag_pdp_recoveries = 0;
uint32_t    gsm_diag_soft_resets    = 0;
uint32_t    gsm_diag_hard_resets    = 0;
uint32_t    gsm_diag_success_after_recovery = 0;
String      gsm_diag_last_recovery  = "NONE";
String      gsm_diag_last_result    = "OK";
// Sensor Validity Mask
struct SensorValidFlags {
    bool bme680;
    bool s300e;
    bool sds011;
    bool ltr390;
    bool veml7700;
    bool gas;
};
SensorValidFlags sensor_valid = {false, false, false, false, false, false};
int         FAN_PRE_START_TIME      = 5;                                    // Fan ON time before data send, in seconds
bool        fanScheduled            = false;
bool        fanIsOn                 = false;
unsigned long fanStartMillis        = 0;

//==============================================================================
// FORWARD FUNCTION DECLARATIONS
//==============================================================================
void WiFi_Mac_to_Byte_Array();
void Ethernet_MAC_Address();
bool tryWiFiConnection(unsigned long timeout_ms);
void startWebServer();
void cleanupClients();

// NVS Persistent Storage Handlers
void save_ethernet_config_NVS(IPAddress ip, IPAddress gateway, IPAddress subnet);
bool load_ethernet_config_NVS(IPAddress &ip, IPAddress &gateway, IPAddress &subnet);
void save_server_config_NVS(IPAddress ip, uint16_t port);
bool load_server_config_NVS(IPAddress &ip, uint16_t &port);
void save_gsm_config_NVS(String ip, String port);
bool load_gsm_config_NVS(String &ip, String &port);
void save_data_interval_NVS(int interval);
int  load_data_interval_NVS();
void save_fan_pre_start_NVS(int seconds);
int  load_fan_pre_start_NVS();
void save_device_id_NVS(String deviceID);
String load_device_id_NVS();
void save_communication_mode_NVS(CommunicationMode mode);
void load_communication_mode_NVS();

// Hardware & Transmission Operations
void    applyCommunicationMode();
bool    gsmSendAT(const String &cmd, const String &expect, uint32_t timeout);
bool    gsm_Init();
void    gsm_DeInit();
String  buildGSM_JSON();
void    gsmSendJSON();
void    Send_Data_If_Ethernet_Connected();
uint8_t xor_checksum(const char *data, uint16_t length);

// HTTP Endpoint Handlers
void handleLoginPage();
void handleDoLogin();
void handleRoot();
void handleSet();
void handleRestart();
void handleOTAEnable();
void handleLiveData();
void handleGSMStatus();
void handleRTCTime();

//==============================================================================
// GSM UTILITY & COMMUNICATION FUNCTIONS
//==============================================================================
/**
 * @brief Sends an AT command to the GSM modem and awaits an expected response string.
 *
 * @param cmd The AT command string to transmit.
 * @param expect The expected response substring confirming success (e.g. "OK", "READY").
 * @param timeout Maximum duration to wait in milliseconds.
 * @return true if the expected substring was received within the timeout period; otherwise false.
 */

bool gsmSendAT(const String &cmd, const String &expect, uint32_t timeout)
{
  gsm.println(cmd);
  uint32_t start = millis();
  String resp;
  resp.reserve(128);

  while (millis() - start < timeout)
  {
    while (gsm.available())
    {
      resp += (char)gsm.read();
      if (resp.length() > 512) {
          resp = resp.substring(resp.length() - 256);
      }
      if (resp.indexOf(expect) >= 0) return true;
      if (resp.indexOf("ERROR") >= 0) break;
    }
    esp_task_wdt_reset();
  }

  gsm_status.last_error = resp;
  return false;
}

/**
 * @brief Initializes the GSM/LTE modem, verifies SIM presence, locks LTE scan mode,
 *        checks network registration, and activates the PDP GPRS packet data context.
 *
 * @return true if modem initialized and PDP context activated successfully; false on error.
 */
void gsm_Hardware_Reset()
{
    USBSerial.println("[GSM] Performing HARDWARE RESET via GPIO15...");
    digitalWrite(GSM_RESET_PIN, LOW);
    delay(200); // Wait >100ms
    digitalWrite(GSM_RESET_PIN, HIGH);
    delay(5000); // Wait for modem to boot up
}

bool gsm_Init()
{
    gsm.begin(115200, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);

    if (!gsmSendAT("AT", "OK", 1000))
        return false;

    gsm_status.power_on = true;

    gsmSendAT("ATE0", "OK", 1000);
    gsmSendAT("AT+CMEE=2", "OK", 1000);

    // Check SIM
    gsm_status.sim_ready = gsmSendAT("AT+CPIN?", "READY", 5000);
    if (!gsm_status.sim_ready)
        return false;

    //Check current Network Preference
    USBSerial.println("===== Checking Network Preference =====");
    gsm.print("AT+QCFG=\"nwscanmode\"");
    delay(500);

    String resp = "";
    unsigned long start = millis();
    while (millis() - start < 3000)
    {
        while (gsm.available())
        {
            char c = gsm.read();
            resp += c;
        }
    }
    USBSerial.println(resp);

    //If not LTE -> Set LTE Only
    if (resp.indexOf("\"nwscanmode\",3") == -1)
    {
        USBSerial.println("Current Mode is NOT LTE Only.");
        USBSerial.println("Setting LTE ONLY (AT+QCFG=\"nwscanmode\",3,1)...");
        if (gsmSendAT("AT+QCFG=\"nwscanmode\",3,1", "OK", 5000))
        {
            USBSerial.println("LTE Only configured successfully.");
        }
        else
        {
            USBSerial.println("Failed to set LTE Only.");
        }
    }
    else
    {
        USBSerial.println("Module already configured as LTE ONLY.");
    }

    //Check LTE Registration
    gsm_status.network_registered = gsmSendAT("AT+CEREG?", "0,1", 5000) || gsmSendAT("AT+CEREG?", "0,5", 5000);

    if (!gsm_status.network_registered)
    {
        USBSerial.println("LTE Network Registration Failed.");
        return false;
    }

    //Print Current Network Type
    USBSerial.println("===== Current Network =====");

    gsm.print("AT+QNWINFO");
    delay(500);

    start = millis();
    resp = "";
    while (millis() - start < 3000)
    {
        while (gsm.available())
        {
            resp += (char)gsm.read();
        }
    }
    USBSerial.println(resp);

    //Signal Strength
    gsmSendAT("AT+CSQ", "OK", 2000);
    gsm_status.rssi = -1;

    //Configure APN
    gsmSendAT("AT+QICSGP=1,1,\"" GSM_APN "\",\"\",\"\",0", "OK", 3000);

    //Activate PDP Context
    if (!gsmSendAT("AT+QIACT=1", "OK", 15000))
        return false;

    gsm_status.gprs_attached = true;
    gsmSendAT("AT+QIACT?", "OK", 3000);
    USBSerial.println("===== GSM Initialization Complete =====");

    return true;
}

/**
 * @brief Deinitializes the GSM modem by terminating open TCP sockets,
 *        deactivating PDP context, and closing the hardware serial bus.
 */
void gsm_DeInit()
{
    USBSerial.println("[COMM] Disabling GSM...");

    // Close TCP session (ignore failure)
    gsmSendAT("AT+QICLOSE=0", "OK", 1000);

    // Detach GPRS (optional)
    gsmSendAT("AT+QIDEACT", "OK", 2000);

    // Stop UART
    gsm.end();

    // Update status
    gsm_status.power_on           = false;
    gsm_status.sim_ready          = false;
    gsm_status.network_registered = false;
    gsm_status.gprs_attached      = false;
    gsm_status.tcp_connected      = false;

    USBSerial.println("[COMM] GSM Disabled");
}

/**
 * @brief Serializes the latest aggregated weather, gas, particulate, and RTC timestamp
 *        data into a compact JSON string formatted for remote server ingestion.
 *
 * @return A serialized JSON string containing all sensor metrics.
 */
String buildGSM_JSON()
{
  String json;
  json.reserve(512);
  json = "{";
  json += "\"device_id\":\"" + String(DEVICE_ID) + "\",";

  json += "\"CO_ppm\":"  + (sensor_valid.gas ? String(weather_data.CO_ppm, 2) : "null") + ",";
  json += "\"O3_ppm\":"  + (sensor_valid.gas ? String(weather_data.O3_ppm, 3) : "null") + ",";
  json += "\"NO2_ppm\":" + (sensor_valid.gas ? String(weather_data.NO2_ppm, 3) : "null") + ",";
  json += "\"SO2_ppm\":" + (sensor_valid.gas ? String(weather_data.SO2_ppm, 3) : "null") + ",";
  json += "\"NS_db\":"   + (sensor_valid.gas ? String(weather_data.NS_db, 2) : "null") + ",";

  json += "\"CO2_ppm\":" + (sensor_valid.s300e ? String(weather_data.CO2_ppm) : "null") + ",";

  json += "\"PM2_5\":"   + (sensor_valid.sds011 ? String(weather_data.pm2_5, 2) : "null") + ",";
  json += "\"PM10\":"    + (sensor_valid.sds011 ? String(weather_data.pm10, 2) : "null") + ",";

  json += "\"lux\":"     + (sensor_valid.veml7700 ? String(weather_data.lux, 2) : "null") + ",";

  json += "\"uvi\":"     + (sensor_valid.ltr390 ? String(weather_data.uvi, 2) : "null") + ",";

  json += "\"temp\":"    + (sensor_valid.bme680 ? String(weather_data.BME680_Temperature, 2) : "null") + ",";
  json += "\"hum\":"     + (sensor_valid.bme680 ? String(weather_data.BME680_Humidity, 2) : "null") + ",";
  json += "\"press\":"   + (sensor_valid.bme680 ? String(weather_data.BME680_Pressure, 2) : "null") + ",";

  json += "\"date\":"    + String(weather_data.rtc_date) + ",";
  json += "\"month\":"   + String(weather_data.rtc_month) + ",";
  json += "\"year\":"    + String(weather_data.rtc_year) + ",";
  json += "\"hour\":"    + String(weather_data.rtc_hour) + ",";
  json += "\"min\":"     + String(weather_data.rtc_min) + ",";
  json += "\"sec\":"     + String(weather_data.rtc_sec);
  json += "}";

  return json;
}

/**
 * @brief Opens a direct TCP socket connection to the remote GSM server via AT commands,
 *        transmits the JSON payload, records transmission diagnostics, and closes the socket.
 */
void gsmSendJSON()
{
  static int consecutive_failures = 0;
  static bool recovering = false;

  if (!gsmSendAT(
      "AT+QIOPEN=1,0,\"TCP\",\"" + gsmServerIP + "\",\"" + gsmServerPort,
      "+QIOPEN: 0,0", 15000))
  {
    gsm_status.tcp_connected = false;
    consecutive_failures++;
    USBSerial.print("GSM TX Failed. Consecutive: ");
    USBSerial.println(consecutive_failures);
  }
  else
  {
    gsm_status.tcp_connected = true;

    if(gsmSendAT("AT+QISEND=0", ">", 3000))
    {
      String payload = buildGSM_JSON();
      gsm.print(payload);
      gsm.write(0x1A);

      if (gsmSendAT("", "SEND OK", 10000))
      {
        gsm_status.last_tx_time = String(weather_data.rtc_hour) + ":" + String(weather_data.rtc_min) + ":" + String(weather_data.rtc_sec);
        consecutive_failures = 0;
        gsm_diag_last_result = "SUCCESS";
        if (recovering) {
            gsm_diag_success_after_recovery++;
            recovering = false;
        }
      }
      else
      {
        consecutive_failures++;
        gsm_diag_last_result = "QISEND_NO_ACK";
      }
    }
    else
    {
      consecutive_failures++;
      gsm_diag_last_result = "QISEND_PROMPT_TIMEOUT";
    }
  }

  // TCP Level Recovery (Level 1)
  if (consecutive_failures > 0) {
      gsm_diag_tcp_failures++;
  }
  gsmSendAT("AT+QICLOSE=0", "OK", 5000);
  gsm_status.tcp_connected = false;

  // Escalate Recovery
  if (consecutive_failures == 3) {
      // Level 2: PDP Context Recovery
      USBSerial.println("[COMM RECOVERY LEVEL 2] 3 failures. PDP Deactivate.");
      gsmSendAT("AT+QIDEACT=1", "OK", 5000);
      gsm_diag_pdp_recoveries++;
      gsm_diag_last_recovery = "PDP_DEACT";
      recovering = true;
  } else if (consecutive_failures == 5) {
      // Level 3: Software Re-initialization
      USBSerial.println("[COMM RECOVERY LEVEL 3] 5 failures. Software gsm_Init.");
      gsm_Init();
      gsm_diag_soft_resets++;
      gsm_diag_last_recovery = "SOFT_INIT";
      recovering = true;
  } else if (consecutive_failures == 7) {
      // Level 4: Hardware RESET_N
      USBSerial.println("[COMM RECOVERY LEVEL 4] 7 failures. Hardware RESET_N.");
      gsm_Hardware_Reset();
      gsm_Init(); // Re-establish UART logic and network state
      gsm_diag_hard_resets++;
      gsm_diag_last_recovery = "HARD_RESET";
      recovering = true;
  } else if (consecutive_failures > 7) {
      // Modulo logic to repeatedly attempt recovery progressively, without causing an immediate boot-loop.
      if ((consecutive_failures - 7) % 10 == 0) {
          USBSerial.println("[COMM RECOVERY LEVEL 4] Throttled Hardware RESET_N.");
          gsm_Hardware_Reset();
          gsm_Init();
          gsm_diag_hard_resets++;
          gsm_diag_last_recovery = "HARD_RESET_THROTTLED";
      } else if ((consecutive_failures - 7) % 5 == 0) {
          USBSerial.println("[COMM RECOVERY LEVEL 3] Throttled Software gsm_Init.");
          gsm_Init();
          gsm_diag_soft_resets++;
          gsm_diag_last_recovery = "SOFT_INIT_THROTTLED";
      } else if ((consecutive_failures - 7) % 3 == 0) {
          USBSerial.println("[COMM RECOVERY LEVEL 2] Throttled PDP Deactivate.");
          gsmSendAT("AT+QIDEACT=1", "OK", 5000);
          gsm_diag_pdp_recoveries++;
          gsm_diag_last_recovery = "PDP_DEACT_THROTTLED";
      }
  }
}


//==============================================================================
// NETWORK UTILITIES & HARDWARE MANAGEMENT
//==============================================================================
/**
 * @brief Reads the ESP32 Wi-Fi interface MAC address string and parses it into a 6-byte array
 *        for assigning as the hardware MAC address of the W5500 Ethernet controller.
 */
void WiFi_Mac_to_Byte_Array() {
  ESP_MAC = WiFi.macAddress();
  int macVals[6];
  sscanf(ESP_MAC.c_str(), "%x:%x:%x:%x:%x:%x", &macVals[0], &macVals[1], &macVals[2], &macVals[3], &macVals[4], &macVals[5]);
  for (int i = 0; i < 6; i++) {
    mac[i] = (uint8_t)macVals[i];
  }
}

/**
 * @brief Prints the assigned 6-byte Ethernet MAC address to the USB serial console.
 */
void Ethernet_MAC_Address()
{
  USBSerial.print(F("Ethernet MAC: "));
  for (int i = 0; i < 6; i++) {
    USBSerial.print(mac[i], HEX);
    if (i < 5) USBSerial.print(":");
  }
  USBSerial.println();
}

/**
 * @brief Attempts to connect to the predefined Wi-Fi station network within a timeout window.
 *
 * @param timeout_ms Maximum time to attempt connection in milliseconds.
 * @return true if connected successfully; false if connection timed out or failed.
 */
bool tryWiFiConnection(unsigned long timeout_ms) {
  WiFi.mode(WIFI_STA);  // Ensure STA mode
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  USBSerial.print(F("Trying to connect to WiFi"));

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout_ms) {
    delay(100);
    //Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    USBSerial.println(F("\nWiFi connected!"));
    USBSerial.println("IP Address: " + WiFi.localIP().toString());
    isWiFiConnected = true;
    return true;
  } else
  {
    USBSerial.println(F("\nWiFi connection failed."));
    isWiFiConnected = false;
    return false;
  }
}

/**
 * @brief Applies the selected communication medium (GSM or Ethernet), initializing
 *        the chosen hardware interface and powering down/deinitializing the inactive one.
 */
void applyCommunicationMode()
{
    if (communicationMode == COMM_GSM)
    {
        USBSerial.println();
        USBSerial.println("================================");
        USBSerial.println(" COMMUNICATION MODE : GSM");
        USBSerial.println("================================");

        // Stop Ethernet TCP connection
        ethernetclient.stop();

        // Disable W5500
        digitalWrite(W5500_RST, LOW);
        delay(100);
        // Initialize GSM
        gsm_Init();

        USBSerial.println("[COMM] GSM ACTIVE");
    }
    else
    {
        USBSerial.println();
        USBSerial.println("================================");
        USBSerial.println(" COMMUNICATION MODE : ETHERNET");
        USBSerial.println("================================");

        gsm_DeInit();
        // Disable W5500 first
        digitalWrite(W5500_RST, LOW);
        delay(100);

        // Keep GSM communication inactive.
        // Actual modem power-off depends on your GSM hardware.

        // Start W5500
        digitalWrite(W5500_RST, HIGH);
        delay(200);

        Ethernet.init(W5500_CS);
        Ethernet.begin(mac, ip, gateway, gateway, subnet);

        USBSerial.print("[ETH] Local IP : ");
        USBSerial.println(Ethernet.localIP());

        USBSerial.println("[COMM] Ethernet ACTIVE");
    }
}

/**
 * @brief Connects to the designated TCP telemetry server over Ethernet and transmits the JSON payload.
 */
void Send_Data_If_Ethernet_Connected()
{
    USBSerial.println("[ETH] Connecting...");

    if (!ethernetclient.connect(serverIP, serverPort))
    {
        USBSerial.println("[ETH] Connection Failed");
        ethernetclient.stop();
        return;
    }

    USBSerial.println("[ETH] Connected");

    String msg = buildGSM_JSON();      // Reuse same JSON payload

    ethernetclient.println(msg);

    USBSerial.println("[ETH] Sent:");
    USBSerial.println(msg);

    ethernetclient.stop();
}

/**
 * @brief Calculates the 8-bit XOR checksum over a sequence of bytes.
 *
 * @param data Pointer to the character/byte buffer.
 * @param length Total length of the buffer.
 * @return Computed 8-bit XOR checksum.
 */
uint8_t xor_checksum(const char *data, uint16_t length)
{
    uint8_t checksum = 0x00;
    for (uint16_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief Terminates disconnected Wi-Fi HTTP client handles to prevent connection leakage.
 */
void cleanupClients()
{
    WiFiClient client = server.client();
    if (client && !client.connected())
    {
        client.stop();
    }
}

//==============================================================================
// NVS STORAGE (PREFERENCES) HANDLERS
//==============================================================================
/**
 * @brief Saves Ethernet static IP network configuration into NVS flash memory.
 *
 * @param ip Static device IP address.
 * @param gateway Network Gateway IP address.
 * @param subnet Network Subnet Mask.
 */
void sava_ethernet_config_NVS(IPAddress ip, IPAddress gateway, IPAddress subnet) {
  prefs.begin("ethconfig", false);
  prefs.putString("ip", ip.toString());
  prefs.putString("gateway", gateway.toString());
  prefs.putString("subnet", subnet.toString());
  prefs.end();
}

/**
 * @brief Loads Ethernet static IP network configuration from NVS flash memory.
 *
 * @param[out] ip Output reference for device IP address.
 * @param[out] gateway Output reference for Gateway IP address.
 * @param[out] subnet Output reference for Subnet Mask.
 * @return true if valid configuration was loaded; false otherwise.
 */
bool load_ethernet_config_NVS(IPAddress &ip, IPAddress &gateway, IPAddress &subnet)
{
    if (!prefs.begin("ethconfig", false))
    {
        USBSerial.println("[NVS] Ethernet config not found - using defaults");
        return false;
    }

    String ipStr = prefs.getString("ip", "");
    String gwStr = prefs.getString("gateway", "");
    String snStr = prefs.getString("subnet", "");

    prefs.end();

    if (ipStr != "" && gwStr != "" && snStr != "")
    {
        if (ip.fromString(ipStr) &&
            gateway.fromString(gwStr) &&
            subnet.fromString(snStr))
        {
            USBSerial.println("[NVS] Ethernet config loaded");
            return true;
        }
    }

    USBSerial.println("[NVS] Ethernet config invalid/not found - using defaults");
    return false;
}

/**
 * @brief Saves Ethernet TCP Server IP address and port into NVS flash memory.
 *
 * @param ip Remote TCP server IP address.
 * @param port Remote TCP server port.
 */
void save_server_config_NVS(IPAddress ip, uint16_t port) {
  prefs.begin("tcpconfig", false);
  prefs.putString("server_ip", ip.toString());
  prefs.putUShort("server_port", port);
  prefs.end();
}

/**
 * @brief Loads Ethernet TCP Server IP address and port from NVS flash memory.
 *
 * @param[out] ip Output reference for TCP server IP address.
 * @param[out] port Output reference for TCP server port.
 * @return true if valid configuration was loaded; false otherwise.
 */
bool load_server_config_NVS(IPAddress &ip, uint16_t &port)
{
    if (!prefs.begin("tcpconfig", false))
    {
        USBSerial.println("[NVS] TCP server config not found - using defaults");
        return false;
    }

    String ipStr = prefs.getString("server_ip", "");
    port = prefs.getUShort("server_port", 0);

    prefs.end();

    if (ipStr != "" && ip.fromString(ipStr) && port != 0)
    {
        USBSerial.println("[NVS] TCP server config loaded");
        return true;
    }

    USBSerial.println("[NVS] TCP server config invalid/not found - using defaults");
    return false;
}


/**
 * @brief Saves GSM remote telemetry server IP address and port into NVS flash memory.
 *
 * @param ip Remote GSM server IP address string.
 * @param port Remote GSM server port string.
 */
void save_gsm_config_NVS(String ip, String port)
{
  prefs.begin("gsmconfig", false);
  prefs.putString("gsm_ip", ip);
  prefs.putString("gsm_port", port);
  prefs.end();
}

/**
 * @brief Loads GSM remote telemetry server IP address and port from NVS flash memory.
 *
 * @param[out] ip Output reference for GSM server IP address.
 * @param[out] port Output reference for GSM server port.
 * @return true if valid configuration was loaded; false otherwise.
 */
bool load_gsm_config_NVS(String &ip, String &port)
{
    if (!prefs.begin("gsmconfig", false))
    {
        USBSerial.println("[NVS] GSM config not found - using defaults");
        return false;
    }

    String savedIP   = prefs.getString("gsm_ip", "");
    String savedPort = prefs.getString("gsm_port", "");

    prefs.end();

    if (savedIP.length() > 0)
        ip = savedIP;

    if (savedPort.length() > 0)
        port = savedPort;

    if (ip.length() > 0 && port.length() > 0)
    {
        USBSerial.println("[NVS] GSM config loaded");
        return true;
    }

    USBSerial.println("[NVS] GSM config invalid/not found - using defaults");
    return false;
}

/**
 * @brief Saves telemetry transmission interval (in minutes) into NVS flash memory.
 *
 * @param interval Data interval in minutes.
 */
void save_data_interval_NVS(int interval) {
  Preferences prefs;
  prefs.begin("interval", false);
  prefs.putInt("data_interval", interval);
  prefs.end();
}

void save_device_id_NVS(String deviceID)
{
    Preferences prefs;

    prefs.begin("deviceconfig", false);
    prefs.putString("device_id", deviceID);
    prefs.end();

    USBSerial.print("[NVS] Device ID saved: ");
    USBSerial.println(deviceID);
}


String load_device_id_NVS()
{
    Preferences prefs;

    if (!prefs.begin("deviceconfig", true))
    {
        USBSerial.println("[NVS] Device ID not found - using default");
        return "YOUR_DEFAULT_ID";
    }

    String deviceID = prefs.getString("device_id", "YOUR_DEFAULT_ID");

    prefs.end();

    USBSerial.print("[NVS] Device ID loaded: ");
    USBSerial.println(deviceID);

    return deviceID;
}

/**
 * @brief Saves telemetry transmission FAN CONTROL (in SECONDS) into NVS flash memory.
 *
 * @param interval FAN CONTROL IN SECONDS.
 */
void save_fan_pre_start_NVS(int seconds)
{
    Preferences prefs;
    prefs.begin("fanconfig", false);
    prefs.putInt("prestart", seconds);
    prefs.end();

    USBSerial.print("[NVS] Fan pre-start saved: ");
    USBSerial.print(seconds);
    USBSerial.println(" seconds");
}

/**
 * @brief Loads telemetry transmission FAN CONTROL (in SECONDS) into NVS flash memory.
 *
 * @param interval Stroed FAN CONTROL IN SECONDS.
 */
int load_fan_pre_start_NVS()
{
    if (!prefs.begin("fanconfig", false))
    {
        USBSerial.println("[NVS] Fan config not found - using default 10 sec");
        return 10;
    }

    int seconds = prefs.getInt("prestart", 10);

    prefs.end();

    if (seconds < 1)
        seconds = 1;

    if (seconds > 300)
        seconds = 300;

    USBSerial.print("[NVS] Fan pre-start loaded: ");
    USBSerial.print(seconds);
    USBSerial.println(" seconds");

    return seconds;
}

/**
 * @brief Loads telemetry transmission interval (in minutes) from NVS flash memory.
 *
 * @return Stored interval in minutes (minimum 1).
 */
int load_data_interval_NVS()
{
    if (!prefs.begin("interval", false))
    {
        USBSerial.println("[NVS] Data interval not found - using default 1 minute");
        return 1;
    }

    int interval = prefs.getInt("data_interval", 1);

    prefs.end();

    if (interval <= 0)
        interval = 1;

    USBSerial.print("[NVS] Data interval loaded: ");
    USBSerial.print(interval);
    USBSerial.println(" minute(s)");

    return interval;
}


/**
 * @brief Saves selected communication mode (GSM / Ethernet) into NVS flash memory.
 *
 * @param mode The CommunicationMode enum value to persist.
 */
void save_communication_mode_NVS(CommunicationMode mode)
{
    prefs.begin("commconfig", false);

    prefs.putUChar("mode", (uint8_t)mode);

    prefs.end();

    USBSerial.print("Communication mode saved: ");

    if (mode == COMM_GSM)
        USBSerial.println("GSM");
    else
        USBSerial.println("ETHERNET");
}


/**
 * @brief Loads the selected communication mode from NVS flash memory.
 */
void load_communication_mode_NVS()
{
    if (!prefs.begin("commconfig", false))
    {
        USBSerial.println("[COMM] NVS open failed!");

        // Safe production default
        communicationMode = COMM_GSM;
        return;
    }

    uint8_t mode = prefs.getUChar("mode", COMM_GSM);

    prefs.end();

    if (mode == COMM_ETHERNET)
        communicationMode = COMM_ETHERNET;
    else
        communicationMode = COMM_GSM;

    USBSerial.print("Communication mode loaded: ");

    if (communicationMode == COMM_GSM)
        USBSerial.println("GSM");
    else
        USBSerial.println("ETHERNET");
}


//==============================================================================
// HTTP WEB SERVER ENDPOINT HANDLERS
//==============================================================================
/**
 * @brief Renders and serves the HTML authentication/login page.
 */
void handleLoginPage() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Login</title>
      <style>
        body {
          margin: 0;
          height: 100vh;
          display: flex;
          justify-content: center;
          align-items: center;
          font-family: 'Segoe UI', Roboto, sans-serif;
          background: linear-gradient(135deg, #1e3c72, #2a5298);
        }

        .card {
          background: rgba(255,255,255,0.1);
          backdrop-filter: blur(12px);
          border-radius: 16px;
          padding: 30px;
          width: 300px;
          text-align: center;
          box-shadow: 0 8px 25px rgba(0,0,0,0.3);
          color: white;
        }

        h2 {
          margin-bottom: 20px;
        }

        input {
          width: 100%;
          padding: 10px;
          margin: 8px 0;
          border-radius: 8px;
          border: none;
          outline: none;
        }

        .btn {
          width: 100%;
          margin-top: 10px;
          background: #00c6ff;
          border: none;
          padding: 10px;
          border-radius: 8px;
          color: black;
          font-weight: bold;
          cursor: pointer;
          transition: 0.3s;
        }

        .btn:hover {
          background: #00a2d4;
        }
      </style>
    </head>

    <body>
      <div class="card">
        <h2>Login</h2>
        <form action="/" method="get">
          <input type="text" name="username" placeholder="Username" required>
          <input type="password" name="password" placeholder="Password" required>
          <input class="btn" type="submit" value="Login">
        </form>
      </div>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

/**
 * @brief Processes credentials submitted via POST to /dologin and redirects on success.
 */
void handleDoLogin() {
  if (server.hasArg("username") && server.hasArg("password")) {
    String user = server.arg("username");
    String pass = server.arg("password");
    if (user == "admin" && pass == "admin") {
      isAuthenticated = true;
      isAuthenticated = true;
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "Redirecting...");
      return;
    }
  }
  server.send(401, "text/html", "<h3>Login Failed. <a href=\"/login\">Try Again</a></h3>");
}

/**
 * @brief Serves real-time sensor metrics HTML snippet for dynamic AJAX dashboard updates.
 */
void handleLiveData() {
  if (!isAuthenticated) { server.send(401, "text/plain", "Unauthorized"); return; }

  String html = R"rawliteral(
    <style>
      .grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
        gap: 12px;
      }

      .card {
        background: rgba(255,255,255,0.1);
        backdrop-filter: blur(8px);
        border-radius: 12px;
        padding: 12px;
        color: #fff;
        font-size: 14px;
        box-shadow: 0 4px 10px rgba(0,0,0,0.2);
      }

      .value {
        font-size: 18px;
        font-weight: bold;
      }
    </style>

    <div class="grid">
      <div class="card">🧪 CO <div class="value">%CO% ppm</div></div>
      <div class="card">🧪 O₃<div class="value">%O3% ppm</div></div>
      <div class="card">🧪 NO₂<div class="value">%NO2% ppm</div></div>
      <div class="card">🧪 SO₂<div class="value">%SO2% ppm</div></div>
      <div class="card">🔊 NOISE<div class="value">%NS% db</div></div>
      <div class="card">🌍 CO₂<div class="value">%CO2% ppm</div></div>

      <div class="card">☁️ PM2.5<div class="value">%PM25% µg/m³</div></div>
      <div class="card">☁️ PM10<div class="value">%PM10% µg/m³</div></div>

      <div class="card">☀ Ambient Light (Lux)<div class="value">%LUX%</div></div>


      <div class="card">🌞 UV Raw<div class="value">%UVRAW%</div></div>
      <div class="card">🌞 UV Index<div class="value">%UV%</div></div>
      <div class="card">🌞 UV Irradiance<div class="value">%UVIRR%</div></div>

      <div class="card">🌡 Temp<div class="value">%BMETEMP% °C</div></div>
      <div class="card">💧 Humidity<div class="value">%BMEHUM% %</div></div>
      <div class="card">💨 Pressure<div class="value">%BMEPRESS%</div></div>

      <div class="card">🌧 Rain<div class="value">%RAIN%</div></div>
    </div>
  )rawliteral";

  html.replace("%LUX%",  String(weather_data.lux, 2));
  html.replace("%PM25%", String(weather_data.pm2_5, 2));
  html.replace("%PM10%", String(weather_data.pm10, 2));
  html.replace("%CO%",   String(weather_data.CO_ppm,2));
  html.replace("%CO2%",  String(weather_data.CO2_ppm));
  html.replace("%NS%",   String(weather_data.NS_db,2));
  html.replace("%NO2%",  String(weather_data.NO2_ppm,3));
  html.replace("%O3%",   String(weather_data.O3_ppm,3));
  html.replace("%SO2%",  String(weather_data.SO2_ppm,3));
  html.replace("%RAIN%", "0.0");
  html.replace("%UVRAW%", String(weather_data.uv_raw));
  html.replace("%UV%", String(weather_data.uvi, 2));
  html.replace("%UVIRR%", String(weather_data.uv_irradiance, 2));
  html.replace("%BMETEMP%",  String(weather_data.BME680_Temperature, 2));
  html.replace("%BMEHUM%",   String(weather_data.BME680_Humidity, 2));
  html.replace("%BMEPRESS%", String(weather_data.BME680_Pressure, 2));


  server.send(200, "text/html", html);
}

/**
 * @brief Serves the current RTC date and time formatted in JSON for live UI clock updates.
 */
void handleRTCTime()
{
    String json = "{";
    json += "\"date\":\"20" + String(weather_data.rtc_year) + "-";

    if(weather_data.rtc_month < 10) json += "0";
    json += String(weather_data.rtc_month) + "-";

    if(weather_data.rtc_date < 10) json += "0";
    json += String(weather_data.rtc_date) + "\",";

    json += "\"time\":\"";

    if(weather_data.rtc_hour < 10) json += "0";
    json += String(weather_data.rtc_hour) + ":";

    if(weather_data.rtc_min < 10) json += "0";
    json += String(weather_data.rtc_min) + ":";

    if(weather_data.rtc_sec < 10) json += "0";
    json += String(weather_data.rtc_sec);

    json += "\"}";

    server.send(200, "application/json", json);
}

/**
 * @brief Serves the GSM modem connection status and diagnostics in JSON format.
 */
void handleGSMStatus()
{
  String json = "{";
  json += "\"power\":" + String(gsm_status.power_on) + ",";
  json += "\"sim\":" + String(gsm_status.sim_ready) + ",";
  json += "\"network\":" + String(gsm_status.network_registered) + ",";
  json += "\"gprs\":" + String(gsm_status.gprs_attached) + ",";
  json += "\"tcp\":" + String(gsm_status.tcp_connected) + ",";
  json += "\"last_tx\":\"" + gsm_status.last_tx_time + "\",";
  json += "\"error\":\"" + gsm_status.last_error + "\"";
  json += "}";

  server.send(200, "application/json", json);
}


/**
 * @brief Renders the primary device management dashboard and configuration web form.
 */
void handleRoot() {
  if (!server.hasArg("username") || !server.hasArg("password")) {
    handleLoginPage();
    return;
  }

  String user = server.arg("username");
  String pass = server.arg("password");

  if (user == "admin" && pass == "admin") {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta charset="utf-8">
        <title>AQI LOGGER</title>
        <style>

        *{
          margin:0;
          padding:0;
          box-sizing:border-box;
        }

        .rtc-card{
          text-align:center;
          background:rgba(255,255,255,0.10);
          backdrop-filter:blur(12px);
          border-radius:18px;
          padding:25px;
          box-shadow:0 8px 25px rgba(0,0,0,.25);
        }

        #rtc-time{
          font-size:56px;
          font-weight:700;
          letter-spacing:4px;
          font-family:'Courier New', monospace;

          background:linear-gradient(90deg,#00ff88,#00c6ff);
          -webkit-background-clip:text;
          -webkit-text-fill-color:transparent;

          margin-top:15px;
        }

        #rtc-date{
          margin-top:10px;
          font-size:20px;
          color:#dbeafe;
          letter-spacing:1px;
        }
        body{
          font-family:'Segoe UI',sans-serif;
          background:linear-gradient(135deg,#1e3c72,#2a5298);
          color:white;
          min-height:100vh;
          padding:20px;
        }

        .container{
          max-width:1400px;
          margin:auto;
        }

        .header{
          text-align:center;
          margin-bottom:25px;
        }

        .header h1{
          font-size:34px;
        }

        .card{
          background:rgba(255,255,255,0.10);
          backdrop-filter:blur(12px);
          border-radius:18px;
          padding:20px;
          margin-bottom:20px;
          box-shadow:0 8px 25px rgba(0,0,0,.25);
        }

        .card h2{
          margin-bottom:15px;
        }

        .note{
          color:#ffffff;
          margin-top:10px;
          display:block;
        }

        #gsm-status{
          background:#0f172a;
          color:#22c55e;
          border-radius:10px;
          padding:15px;
          font-family:monospace;
          font-size:14px;
          white-space:pre-wrap;
        }

        .config-grid{
          display:grid;
          grid-template-columns:repeat(auto-fit,minmax(350px,1fr));
          gap:20px;
        }

        .group{
          background:rgba(255,255,255,.05);
          border-radius:12px;
          padding:15px;
        }

        .group h3{
          margin-bottom:15px;
          color:#7dd3fc;
        }

        .row{
          margin-bottom:12px;
        }

        label{
          display:block;
          margin-bottom:5px;
        }

        input[type=text],
        input[type=number],
        input[type=date],
        input[type=time]{
          width:100%;
          padding:10px;
          border:none;
          border-radius:8px;
          outline:none;
        }

        .btn{
          background:#00c6ff;
          color:black;
          border:none;
          padding:12px 20px;
          border-radius:10px;
          font-weight:bold;
          cursor:pointer;
        }

        .btn:hover{
          background:#00a2d4;
        }

        .save-row{
          margin-top:20px;
          text-align:center;
        }

        %RESTART_CSS%

        </style>
      </head>
      <body>
        <h1>☁️ AQI Weather Station V1.1 🍃</h1>

        <div class="card rtc-card">
          <h2> Real Time </h2>

          <div id="rtc-time">
            Loading...
          </div>

          <div id="rtc-date">
            Loading...
          </div>
        </div>


        <div class="card">
          <h2>Live Sensor Data</h2>
          <div id="live-data">Loading...</div>
          <small style="color: white;">Values refresh every 5 seconds. Lux updates live, the rest are dummy placeholders.</small>
        </div>

        <div class="card">
          <h2>GSM Status</h2>
          <pre id="gsm-status">Loading...</pre>
        </div>

        <!-- ===== DEVICE RESTART INFORMATION ===== -->

        %RESTART_HTML%

        <div class="card">

        <h2>⚙ Configuration</h2>

        <form action="/set" method="get">

        <div class="config-grid">

        <div class="group">
        <h3>Ethernet Configuration</h3>

        <div class="row">
        <label>IP Address</label>
        <input type="text" name="ip" value="%IP%">
        </div>

        <div class="row">
        <label>Gateway</label>
        <input type="text" name="gateway" value="%GATEWAY%">
        </div>

        <div class="row">
        <label>Subnet</label>
        <input type="text" name="subnet" value="%SUBNET%">
        </div>
        </div>

        <div class="group">
        <h3>TCP Server</h3>

        <div class="row">
        <label>Server IP</label>
        <input type="text" name="serverip" value="%SERVERIP%">
        </div>

        <div class="row">
        <label>Server Port</label>
        <input type="number" name="serverport" value="%SERVERPORT%">
        </div>
        </div>

        <div class="group">
        <h3>RTC Configuration</h3>

        <div class="row">
        <label>Date</label>
        <input type="date" id="dateInput" name="date">
        </div>

        <div class="row">
        <label>Time</label>
        <input type="time" id="timeInput" step="1" name="time">
        </div>

        <div class="row" style="margin-top: 10px;">
        <button type="button" onclick="fetchSystemTime()" style="background-color: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; font-size: 14px;">Fetch System Time</button>
        </div>
        </div>
        <script>
        function fetchSystemTime() {
          const now = new Date();
          const year = now.getFullYear();
          const month = String(now.getMonth() + 1).padStart(2, '0');
          const day = String(now.getDate()).padStart(2, '0');
          const hours = String(now.getHours()).padStart(2, '0');
          const minutes = String(now.getMinutes()).padStart(2, '0');
          const seconds = String(now.getSeconds()).padStart(2, '0');

          document.getElementById('dateInput').value = `${year}-${month}-${day}`;
          document.getElementById('timeInput').value = `${hours}:${minutes}:${seconds}`;
        }
        </script>

        <div class="group">
        <h3>GSM Server</h3>

        <div class="row">
        <label>GSM Server IP</label>
        <input type="text" name="gsmip" value="%GSMIP%">
        </div>

        <div class="row">
        <label>GSM Server Port</label>
        <input type="text" name="gsmport" value="%GSMPORT%">
        </div>
        </div>

        <div class="group">
        <h3>Communication Mode</h3>

        <div class="row">
        <label>Active Communication</label>

        <select name="commmode">
          <option value="gsm" %GSM_SELECTED%>GSM</option>
          <option value="ethernet" %ETHERNET_SELECTED%>Ethernet</option>
        </select>

        </div>
        </div>

        <div class="row">
        <label>Interval (minutes)</label>
        <input type="number" name="datainterval" value="%DATAINTERVAL%">
        <label>Fan Pre-Start Time (seconds)</label>
        <input type="number" name="fanprestart" value="%FANPRESTART%" min="1" max="300">
        <label>Device ID</label>
        <input type="text" name="deviceid" value="%DEVICEID%" maxlength="50">
        </div>
        </div>

        </div>

        <div class="save-row">
        <input class="btn" type="submit" value="💾 Save Configuration">
        </div>

        </form>

        </div>

        <hr>
        <div class="card">
          <form action="/ota" method="get">
            <input class="btn" type="submit" value="Enable OTA">
          </form>
        </div>

        <script>
          function fetchLiveData() {
            fetch('/livedata')
              .then(response => response.text())
              .then(html => { document.getElementById("live-data").innerHTML = html; })
              .catch(err => console.error("Error fetching live data:", err));
          }
          setInterval(fetchLiveData, 5000);
          fetchLiveData();

          /* -------- GSM STATUS -------- */
          function fetchGSM() {
            fetch('/gsmstatus')
              .then(response => response.text())
              .then(html => { document.getElementById("gsm-status").innerHTML = html; })
              .catch(err => console.error("Error fetching live data:", err));
              }
            setInterval(fetchGSM, 5000);
            fetchGSM();

            function fetchRTCTime()
            {
                fetch('/rtctime')
                .then(response => response.json())
                .then(data =>
                {
                    document.getElementById("rtc-date").innerHTML = data.date;
                    document.getElementById("rtc-time").innerHTML = data.time;
                })
                .catch(error =>
                {
                    console.log("RTC Error:", error);
                });
            }

            setInterval(fetchRTCTime, 10000);
            fetchRTCTime();
            setInterval(fetchRTCTime, 10000);
            fetchRTCTime();
            %RESTART_JS%
        </script>
      </body>
      </html>
    )rawliteral";

    html.replace("%RESTART_CSS%", deviceRestartInfo.getCss());
    html.replace("%RESTART_HTML%", deviceRestartInfo.getHtml());
    html.replace("%RESTART_JS%", deviceRestartInfo.getJavaScript());
    html.replace("%IP%", ip.toString());
    html.replace("%GATEWAY%", gateway.toString());
    html.replace("%SUBNET%", subnet.toString());
    html.replace("%SERVERIP%", serverIP.toString());
    html.replace("%SERVERPORT%", String(serverPort));
    html.replace("%GSMIP%", gsmServerIP);
    html.replace("%GSMPORT%", gsmServerPort);
    html.replace("%DATAINTERVAL%", String(DATA_INTERVAL));
    html.replace("%FANPRESTART%", String(FAN_PRE_START_TIME));
    html.replace("%DEVICEID%", DEVICE_ID);

    if (communicationMode == COMM_GSM)
    {
        html.replace("%GSM_SELECTED%", "selected");
        html.replace("%ETHERNET_SELECTED%", "");
    }
    else
    {
        html.replace("%GSM_SELECTED%", "");
        html.replace("%ETHERNET_SELECTED%", "selected");
    }

    server.send(200, "text/html", html);
  } else {
    server.send(401, "text/html", "<h3>Invalid login. <a href=\"/\">Try Again</a></h3>");
  }
}

/**
 * @brief Processes and validates configuration form submissions, saves updated parameters
 *        to NVS flash memory, and reconfigures active network interfaces.
 */
void handleSet() {
  if (!isAuthenticated) { server.send(401, "text/plain", "Unauthorized"); return; }

  if (server.hasArg("ip") && server.hasArg("gateway") && server.hasArg("subnet")) {
    IPAddress newIP, newGateway, newSubnet;

    if (newIP.fromString(server.arg("ip")) &&
        newGateway.fromString(server.arg("gateway")) &&
        newSubnet.fromString(server.arg("subnet"))) {

      ip = newIP;
      gateway = newGateway;
      subnet = newSubnet;

      sava_ethernet_config_NVS(ip, gateway, subnet);

      // Ensure bounds
      if (server.hasArg("device_id") && server.arg("device_id").length() < 32) {
          DEVICE_ID = server.arg("device_id");
          save_device_id_NVS(DEVICE_ID);
      }
      if (server.hasArg("gsmip") && server.arg("gsmip").length() < 64 &&
          server.hasArg("gsmport") && server.arg("gsmport").length() < 10) {
          gsmServerIP = server.arg("gsmip");
          gsmServerPort = server.arg("gsmport");
          save_gsm_config_NVS(gsmServerIP, gsmServerPort);
      }
      if (server.hasArg("data_interval")) {
          int di = server.arg("data_interval").toInt();
          if (di >= 1 && di <= 1440) {
              DATA_INTERVAL = di;
              save_data_interval_NVS(DATA_INTERVAL);
          }
      }
      if (server.hasArg("fan_prestart")) {
          int fpt = server.arg("fan_prestart").toInt();
          if (fpt >= 0 && fpt <= 600) {
              FAN_PRE_START_TIME = fpt;
              save_fan_pre_start_NVS(FAN_PRE_START_TIME);
          }
      }

      // TCP Server config
      if (server.hasArg("serverip") && server.hasArg("serverport")) {
        IPAddress newServerIP;
        uint16_t newServerPort = server.arg("serverport").toInt();
        if (newServerIP.fromString(server.arg("serverip"))) {
          serverIP = newServerIP;
          serverPort = newServerPort;
          save_server_config_NVS(serverIP, serverPort);
        }
      }

      // RTC Config
      if (server.hasArg("date") && server.hasArg("time"))
      {
        String dateStr = server.arg("date");                  // Format: YYYY-MM-DD
        String timeStr = server.arg("time");                  // Format: HH:MM:SS

        uint16_t fullYear = dateStr.substring(0, 4).toInt();  // Full year
        uint8_t year = fullYear % 100;                        // RTC accepts only two digits
        uint8_t month = dateStr.substring(5, 7).toInt();
        uint8_t date = dateStr.substring(8, 10).toInt();

        uint8_t hour = timeStr.substring(0, 2).toInt();
        uint8_t min  = timeStr.substring(3, 5).toInt();
        uint8_t sec  = timeStr.substring(6, 8).toInt();

        //Serial.printf("Parsed Date/Time: %02d/%02d/20%02d  %02d:%02d:%02d\n", date, month, year, hour, min, sec);

        uint8_t day = 1;  // Optional: could calculate actual weekday if needed
        setRTCDateTime(sec, min, hour, day, date, month, year);
      }

      // DATA_INTERVAL handling
      if (server.hasArg("datainterval")) {
        DATA_INTERVAL = server.arg("datainterval").toInt();
        save_data_interval_NVS(DATA_INTERVAL);
      }

      // Device_ID handling
      if (server.hasArg("deviceid"))
      {
          String newDeviceID = server.arg("deviceid");

          newDeviceID.trim();

          if (newDeviceID.length() > 0)
          {
              DEVICE_ID = newDeviceID;

              save_device_id_NVS(DEVICE_ID);

              USBSerial.print("[CONFIG] Device ID changed to: ");
              USBSerial.println(DEVICE_ID);
          }
      }

      // FAN_CONTROL handling
      if (server.hasArg("fanprestart"))
      {
          FAN_PRE_START_TIME = server.arg("fanprestart").toInt();

          if (FAN_PRE_START_TIME < 1)
              FAN_PRE_START_TIME = 1;

          if (FAN_PRE_START_TIME > 300)
              FAN_PRE_START_TIME = 300;

          save_fan_pre_start_NVS(FAN_PRE_START_TIME);
      }

      // Communication Mode
      if (server.hasArg("commmode"))
      {
          String mode = server.arg("commmode");

          if (mode == "ethernet")
          {
              communicationMode = COMM_ETHERNET;
          }
          else
          {
              communicationMode = COMM_GSM;
          }

          save_communication_mode_NVS(communicationMode);
      }

      // Apply selected communication mode
      applyCommunicationMode();

      String msg = "<h3>Configuration Saved Successfully!</h3>";
      msg += "<p><b>IP:</b> " + ip.toString() + "</p>";
      msg += "<p><b>Gateway:</b> " + gateway.toString() + "</p>";
      msg += "<p><b>Subnet:</b> " + subnet.toString() + "</p>";
      msg += "<p><b>Server IP:</b> " + serverIP.toString() + "</p>";
      msg += "<p><b>Server Port:</b> " + String(serverPort) + "</p>";
      msg += "<p><b>GSM Server IP:</b> " + gsmServerIP + "</p>";
      msg += "<p><b>GSM Server Port:</b> " + gsmServerPort + "</p>";
      msg += "<p><b>Communication:</b> ";
      if (communicationMode == COMM_GSM)
          msg += "GSM";
      else
          msg += "ETHERNET";

      msg += "</p>";
      msg += "<form action=\"/restart\" method=\"get\">";
      msg += "<input type=\"submit\" value=\"Restart ESP32\">";
      msg += "</form>";

      server.send(200, "text/html", msg);
      return;
    }
  }

  server.send(400, "text/plain", "Invalid parameters");
}

/**
 * @brief Handles remote restart request by sending an acknowledgment and triggering ESP.restart().
 */
void handleRestart() {
  if (!isAuthenticated) { server.send(401, "text/plain", "Unauthorized"); return; }
  server.send(200, "text/html", "<h3>Restarting ESP32...</h3>");
  delay(500);
  ESP.restart();
}

/**
 * @brief Activates the Over-The-Air (OTA) firmware upgrade listener mode.
 */
void handleOTAEnable() {
  if (!isAuthenticated) { server.send(401, "text/plain", "Unauthorized"); return; }
  OTA_ENABLE_FLAG = true;
  server.send(200, "text/html", "<h3>OTA Enabled. You can now start the OTA process.</h3>");
}

/**
 * @brief Registers all HTTP request routes and begins hosting the Web Server.
 */
void startWebServer()
{
  if (!webServerStarted)
  {
    server.on("/", handleRoot);
    server.on("/login", handleLoginPage);
    server.on("/dologin", HTTP_POST, handleDoLogin);
    server.on("/set", handleSet);
    server.on("/restart", handleRestart);
    server.on("/ota", handleOTAEnable);       // OTA Route
    server.on("/livedata", handleLiveData);  // << NEW
    server.on("/gsmstatus", handleGSMStatus);
    server.on("/rtctime", handleRTCTime);
    server.begin();
    webServerStarted = true;
    if (WiFi.status() == WL_CONNECTED)
    {
      USBSerial.println("Web server started at: http://" + WiFi.localIP().toString());
    }
  }
}

//==============================================================================
// SETUP & INITIALIZATION ROUTINE
//==============================================================================
/**
 * @brief Main hardware setup function. Initializes serial buses, hardware timers,
 *        I2C buses, environmental sensors, NVS storage configurations, network
 *        interfaces, watchdog timer, and web services.
 */
void setup()
{
  USBSerial.begin(115200);

  USBSerial.print(F("APB CLK SET = "));               // To know our ESP32 operates on defined frequency
  USBSerial.println(rtc_clk_apb_freq_get());

  delay(3000);                                        // Initialization delay

  // Initialize EC200U Reset Pin
  pinMode(GSM_RESET_PIN, OUTPUT);
  digitalWrite(GSM_RESET_PIN, HIGH); // Active LOW

  // Initialize FAN pin
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  // Initialize W5500 hardware reset pin
  pinMode(W5500_RST, OUTPUT);
  digitalWrite(W5500_RST, LOW);

  gsm_Init();                                         // GSM initialization
  I2C_HAL_Init();                                     // I2C initialization
  BME680_Init();                                      // BME680 initialization

  tryWiFiConnection(3000);                            // WiFi initalization
   if (isWiFiConnected) {
    startWebServer();
  }

  WiFi_Mac_to_Byte_Array();                           // Converting ESP MAC Address in Byte Array for Ethernet Module
  Ethernet_MAC_Address();                             // Printing ESP MAC Address on Serial monitor
  gas_sensor_init();                                  // UART Multiplexer initialization for noise sensor on ch 6
  I2C_gas_sensor_init();                              // GAS Sensor initialization
  veml7700_init();                                    // Ambient Light initialization
  init_LTR390();                                      // UV radiation sensor LTR390 initialization
  sds011_init();                                      // PM sensor initialization
  S300E_Init();                                       // CO2 sensor initialization
  initRTC();                                          // RTC initialization
  deviceRestartInfo.begin();

  // Load persisted settings from NVS Flash Memory
  USBSerial.println("[NVS] Loading Ethernet config");
  load_ethernet_config_NVS(ip, gateway, subnet);      // Saving Ethernet Configuration to NVS
  USBSerial.println("[NVS] Loading Server config");
  load_server_config_NVS(serverIP, serverPort);       // Saving TCP Server Configuration to NVS
  USBSerial.println("[NVS] Loading GSM config");
  load_gsm_config_NVS(gsmServerIP, gsmServerPort);
  USBSerial.println("[NVS] Loading Data interval");
  DATA_INTERVAL = load_data_interval_NVS();           // Saving Data Interval to NVS
  USBSerial.println("[NVS] Loading Communication mode");
  FAN_PRE_START_TIME = load_fan_pre_start_NVS();
  DEVICE_ID = load_device_id_NVS();
  load_communication_mode_NVS();

  // Apply Active Communication Subsystem
  applyCommunicationMode();
  USBSerial.print("Server IP: "); USBSerial.println(serverIP);
  USBSerial.print("Server Port: "); USBSerial.println(serverPort);
  USBSerial.println("GSM IP: " + gsmServerIP);
  USBSerial.println("GSM Port: " + gsmServerPort);

  // Initializing Web Page Service
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/restart", handleRestart);
  server.on("/ota", handleOTAEnable);
  server.on("/livedata", handleLiveData);
  deviceRestartInfo.attach(server);
  server.begin();
  USBSerial.println("Web server started at: http://" + WiFi.localIP().toString());


  // Initialize OTA & Hardware Interval Timers
  init_ssl_ota();
  init_hw_ssl_timer(timer,&ssl_timer);                // After all init process complete, lets start Timer

  // Initialize the Task Watchdog Timer (WDT) 30 Second Timeout (To avoid controller's reboot during OTA process)
  esp_task_wdt_init(30, true);                        // Set timeout to 30 seconds, and enable panic handling
  esp_task_wdt_add(NULL);                             // Add the current task to WDT monitoring

}

//==============================================================================
// MAIN APPLICATION LOOP
//==============================================================================
/**
 * @brief Main execution loop. Manages HTTP client requests, background sensor tasks,
 *        OTA update windows, periodic sensor sampling schedules (5s, 10s, 30s, 40s),
 *        and periodic telemetry transmissions over GSM/Ethernet.
 */
void loop()
{

  // Continuous tasks: SDS011 UART buffer & Web Server client handling
  {
    sds011_task();
    server.handleClient();
    esp_task_wdt_reset();
  }

  // -------------------------------------------------------------------------
  // OTA Update Execution Window
  // -------------------------------------------------------------------------
  if (OTA_ENABLE_FLAG == true)
  {
    esp_task_wdt_reset();  // Prevent WDT during OTA pause

    //OTA begin() - Initialization + Network listening begin
    ArduinoOTA.begin();

    USBSerial.println("ESP32 machine is ready for OTA");

    //Time stamp when device in OTA mode
    unsigned long OTA_START_TIME = millis();

    //Wait for certain time else back to normal mode
    while((millis() - OTA_START_TIME) < OTA_TIMEOUT_PERIOD)
    {
      ArduinoOTA.handle();
      esp_task_wdt_reset();
    }

    //Reset Flag & stop OTA listening - Timeout
    ArduinoOTA.end();
    OTA_ENABLE_FLAG = false;
    USBSerial.println("[OTA] OTA window expired. Resuming normal operation.");
    esp_task_wdt_reset();

  }

  esp_task_wdt_reset();                         //Clear Watchdog Frequently....

  // -------------------------------------------------------------------------
  // High Priority Task (5-Second Schedule): RTC Time Sampling
  // -------------------------------------------------------------------------
  if(ssl_timer.FIVE_SECOND_ELAPSED_FLAG == true)
  {
    //RESET FLAG
    ssl_timer.FIVE_SECOND_ELAPSED_FLAG = false;
    esp_task_wdt_reset();
    veml7700_task();                          // Ambient light data
    read_LTR390();                            // UV radiation data
    readRTCDateTimeToStruct(&weather_data);   // RTC data
    esp_task_wdt_reset();
  }

  // -------------------------------------------------------------------------
  // Medium Priority Task (10-Second Schedule): BME680 & Wi-Fi Keep-Alive
  // -------------------------------------------------------------------------
  if(ssl_timer.TEN_SECOND_ELAPSED_FLAG == true)
  {
    //RESET FLAG
    ssl_timer.TEN_SECOND_ELAPSED_FLAG = false;
    esp_task_wdt_reset();
    BME680_Process();                        // BME680 data
    USBSerial.println(DEVICE_ID);
    if((!isWiFiConnected) || ((WiFi.status() != WL_CONNECTED)))     // WiFi reconnection logic
    {
      isWiFiConnected = false;
      bool connected = tryWiFiConnection(3000);                     // Try for 3 seconds

      if (connected && (WiFi.status() == WL_CONNECTED))
      {
        startWebServer();
      }
      else
      {
        USBSerial.println(F("WiFi connection failed. WL_CONNECT_FAILED"));
      }

    }
    cleanupClients();
    esp_task_wdt_reset();
  }

  // -------------------------------------------------------------------------
  // Primary Telemetry Task (30-Second Schedule): Gas/CO2/PM Sampling & Transmission
  // -------------------------------------------------------------------------
  if(ssl_timer.THIRTY_SECOND_ELAPSED_FLAG == true)
  {
    //RESET FLAG
    ssl_timer.THIRTY_SECOND_ELAPSED_FLAG = false;
    esp_task_wdt_reset();
    // gas_sensor_task();                      // Noise sensor data
    // gas_sensor_print();
    // I2C_gas_sensor_read();                  // Gases sensor data
    // I2C_gas_sensor_print();
    // S300E_data();                           // CO2 data
    // sds011_print();                         // PM Data

    COUNTER_30SEC++;                        // Increase 30 sec counter
    if(COUNTER_30SEC >= (DATA_INTERVAL * 2))
    {
      // if (communicationMode == COMM_GSM)
      // {
      //     USBSerial.println("[COMM] Sending AQI data via GSM");
      //     gsmSendJSON();
      // }
      // else
      // {
      //     USBSerial.println("[COMM] Sending AQI data via Ethernet");
      //     Send_Data_If_Ethernet_Connected();
      // }
      // COUNTER_30SEC = 0;
        // Reset counter
      COUNTER_30SEC = 0;

      // Schedule FAN
      fanScheduled = true;

      USBSerial.print("[FAN] Data transmission scheduled. ");
      USBSerial.print(FAN_PRE_START_TIME);
      USBSerial.println(" sec pre-start.");
    }
    esp_task_wdt_reset();
  }
  // ---------------------------------------------------------
// FAN CONTROL + DATA TRANSMISSION
// ---------------------------------------------------------
if(fanScheduled)
{
    // FAN ON
    if(!fanIsOn)
    {
        digitalWrite(FAN_PIN, HIGH);

        fanIsOn = true;
        fanStartMillis = millis();

        USBSerial.println("[FAN] GPIO 42 ON");
    }

    // FAN PRE-START TIME COMPLETE
    if((millis() - fanStartMillis) >=
       ((unsigned long)FAN_PRE_START_TIME * 1000UL))
    {
        USBSerial.println("[FAN] Pre-start time completed");
        USBSerial.println("[SENSOR] Starting sensor readings");

        // ---------------------------------------------------------
        // SENSOR READINGS
        // ---------------------------------------------------------
        gas_sensor_task();
        gas_sensor_print();

        I2C_gas_sensor_read();
        I2C_gas_sensor_print();

        S300E_data();
        sds011_print();

        // -------------------------------------------------
        // SEND SENSOR DATA
        // -------------------------------------------------
        if(communicationMode == COMM_GSM)
        {
            USBSerial.println("[COMM] Sending AQI data via GSM");
            gsmSendJSON();
        }
        else
        {
            USBSerial.println("[COMM] Sending AQI data via Ethernet");
            Send_Data_If_Ethernet_Connected();
        }

        // -------------------------------------------------
        // DATA SEND COMPLETE -> FAN OFF
        // -------------------------------------------------
        digitalWrite(FAN_PIN, LOW);

        fanIsOn = false;
        fanScheduled = false;

        USBSerial.println("[FAN] GPIO 42 OFF");
        USBSerial.println("[TELEMETRY] Data transmission completed");
    }
}

  // -------------------------------------------------------------------------
  // Diagnostic Task (40-Second Schedule): Payload Verification & Heartbeat
  // -------------------------------------------------------------------------
  if(ssl_timer.FOURTY_SECOND_ELAPSED_FLAG == true)
  {
    //RESET FLAG
    ssl_timer.FOURTY_SECOND_ELAPSED_FLAG = false;
    esp_task_wdt_reset();
    USBSerial.println(buildGSM_JSON());
    USBSerial.println("Firmware version_1.1.1");
    esp_task_wdt_reset();
  }

  // -------------------------------------------------------------------------
  // Low Priority Task (50-Second Schedule): Spare Maintenance Window
  // -------------------------------------------------------------------------
  if(ssl_timer.FIFTY_SECOND_ELAPSED_FLAG == true)
  {
    //RESET FLAG
    ssl_timer.FIFTY_SECOND_ELAPSED_FLAG = false;
    esp_task_wdt_reset();
    esp_task_wdt_reset();
    //Serial.println(">>50SEC...");
  }

}