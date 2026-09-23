#ifndef __GSM_H
#define __GSM_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "data_structure.h"

// ---------------- UART PIN CONFIGURATION ----------------
#define GSM_RX_PIN          17       // ESP32S3 RX ← GSM TX
#define GSM_TX_PIN          18       // ESP32S3 TX → GSM RX


// Server & APN
#define SERVER_IP     "182.237.15.204"      // Remote server IP
#define SERVER_PORT   "5059"                // Remote server port
#define APN           "airtelgprs.com"      // SIM APN (change as per operator)



//-------------------------------------------------------------------------------------------------
// Function : For initializing GSM UART and perform GSM/GPRS initialization
// Argument :
// Return   : void
void gsmSetup();

//-------------------------------------------------------------------------------------------------
// Function : For GSM processing task
// Argument :
// Return   : void
void gsmProcessLoop();

//-------------------------------------------------------------------------------------------------
// Function : For sending AT command and wait for expected response
// Argument : AT command string, Expected response substring, Timeout in milliseconds
// Return   : true if expected response received
bool sendAT(String cmd, String expected, uint32_t timeout);

//-------------------------------------------------------------------------------------------------
// Function : Initialize GSM network and GPRS
// Argument :
// Return   : true if initialization successful
bool gsmInit();

//-------------------------------------------------------------------------------------------------
// Function : Establish TCP connection with server
// Argument :
// Return   : true if connected
bool tcpConnect();

//-------------------------------------------------------------------------------------------------
// Function : Send payload over TCP connection
// Argument : payload JSON string
// Return   : true if data sent successfully
bool tcpSend(String payload);

//-------------------------------------------------------------------------------------------------
// Function : Close TCP connection and deactivate PDP context
// Argument :
// Return   :
void tcpClose();

//-------------------------------------------------------------------------------------------------
// Function : Build JSON payload from sensor data
// Argument :
// Return   : JSON formatted string
String buildJSON();



#endif