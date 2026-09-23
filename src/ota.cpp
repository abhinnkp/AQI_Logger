/*
This library is specifically designed for the Over the Air (OTA) process, facilitating connections to an access point using a predefined ID and password.
We believe this matter has already been resolved.
*/
#include "debug_serial.h"
#include<Arduino.h>
#include "ota.h"


//-------------------------------------------------------------------------------------------------
//Function for initialize functions for OTA_PROCESS
//Argument : void
//Return   : void
void init_ssl_ota()
{
  //-----------------------------------------------------------------------------------------------
  // Setup OTA
  //Note : ArduinoOTA declared in ArduinoOTA.cpp file, member of ArduinoOTAClass
  //This is an event handler that triggers when the OTA update process starts
  //lambda function
  ArduinoOTA.onStart([]()
  {
    String type;

    //ArduinoOTA.getCommand() - Function checks what type of update is being initiated
    if(ArduinoOTA.getCommand() == U_FLASH)
    {
      //U_FLASH - Means OTA request for Firmware update
      type = "sketch";
    }
    else
    {
      //OTArequest for update File system SPIFFS
      // U_SPIFFS
      type = "filesystem";
    }

    //Print info
    USBSerial.println("Start updating " + type);
  });


  //-----------------------------------------------------------------------------------------------
  //Event Handler - Triggered when OTA end
  ArduinoOTA.onEnd([]()
  {
    USBSerial.println("\n OTA update End");
  });


  //-----------------------------------------------------------------------------------------------
  //Function called during OTA in progress
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    USBSerial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });


  //-----------------------------------------------------------------------------------------------
  //Function Triggered during OTA Error
  ArduinoOTA.onError([](ota_error_t error)
  {
    USBSerial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      USBSerial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR)
    {
      USBSerial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR)
    {
      USBSerial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR)
    {
      USBSerial.println("Receive Failed");
    } else if (error == OTA_END_ERROR)
    {
      USBSerial.println("End Failed");
    }
  });
}
//-------------------------------------------------------------------------------------------------


/*
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_ARDUINOOTA)
ArduinoOTAClass ArduinoOTA;
#endif
*/