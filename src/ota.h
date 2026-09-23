/*
This library is specifically designed for the Over the Air (OTA) process, facilitating connections to an access point using a predefined ID and password.
We believe this matter has already been resolved.
*/



#ifndef __OTA_H
#define __OTA_H

#include <Arduino.h>                                                    //For platfrom.io IDE
#include <WiFi.h>                                                       //For manage Wi-Fi connectivity like scanning,connect Network, and create AP
#include <ArduinoOTA.h>                                                 //Allowing to wirelessly upload code to the ESP32 without needing a physical connection via USB


//-------------------------------------------------------------------------------------------------
//Function for initialize functions for OTA_PROCESS
//This Function is container of all OTA related callback functions...
//Argument : void
//Return   : void
void init_ssl_ota();

#endif