/*
This library facilitate communication with the RTC IC DS3231 via the I2C bus, enabling the retrieval and setting of time and date information as needed.
Note:This code does not rely on any DS3231 library

Author : Savan Jivani
Date   : 11 December 2024
*/

#ifndef __RTC_S_H
#define __RTC_S_H

#include <Arduino.h>
// #include <Wire.h>
#include "i2c_20k.h"
#include "data_structure.h"

// Pin definitions
// #define SDA_PIN             47
// #define SCL_PIN             21

// Device I2C Address
#define DS3231_ADDRESS      0x68


void initRTC();
void setRTCDateTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t day, uint8_t date, uint8_t month, uint8_t year);
void readRTCDateTimeToStruct(weather_station_global_structure_t *);



#endif