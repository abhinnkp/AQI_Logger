#include "debug_serial.h"
#include <Arduino.h>
#include <rtc_s.h>
#include <data_structure.h>
//#include <Wire.h>
#include "i2c_20k.h"
//-------------------------------------------------------------------------------------------------
void initRTC() {
//    Wire.begin(SDA_PIN, SCL_PIN); // SDA, SCL
}

//-------------------------------------------------------------------------------------------------
uint8_t decToBcd(uint8_t val) {
    return ((val / 10 * 16) + (val % 10));
}

//-------------------------------------------------------------------------------------------------
uint8_t bcdToDec(uint8_t val) {
    return ((val / 16 * 10) + (val % 16));
}

//-------------------------------------------------------------------------------------------------
void setRTCDateTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t day, uint8_t date, uint8_t month, uint8_t year) {
    uint8_t buffer[7];

    buffer[0] = decToBcd(sec);
    buffer[1] = decToBcd(min);
    buffer[2] = decToBcd(hour);
    buffer[3] = decToBcd(day);
    buffer[4] = decToBcd(date);
    buffer[5] = decToBcd(month);
    buffer[6] = decToBcd(year);
    I2C_Write(DS3231_ADDRESS, 0x00, buffer, 7);
}

//-------------------------------------------------------------------------------------------------
void readRTCDateTimeToStruct(weather_station_global_structure_t *weather_data) {
    if (weather_data == nullptr)
        return;

    uint8_t rtcData[7];

    if(I2C_Read(DS3231_ADDRESS,0x00,rtcData,7)!=ESP_OK)
    {
        USBSerial.println("RTC Read Failed");

        weather_data->rtc_sec   = 0;
        weather_data->rtc_min   = 0;
        weather_data->rtc_hour  = 0;
        weather_data->rtc_day   = 0;
        weather_data->rtc_date  = 0;
        weather_data->rtc_month = 0;
        weather_data->rtc_year  = 0;

        return;
    }

    uint8_t sec   = bcdToDec(rtcData[0]);
    uint8_t min   = bcdToDec(rtcData[1]);
    uint8_t hour  = bcdToDec(rtcData[2]);
    uint8_t day   = bcdToDec(rtcData[3]);
    uint8_t date  = bcdToDec(rtcData[4]);
    uint8_t month = bcdToDec(rtcData[5]);
    uint8_t year  = bcdToDec(rtcData[6]);

    weather_data->rtc_sec = sec;
    weather_data->rtc_min = min;
    weather_data->rtc_hour = hour;
    weather_data->rtc_day = day;
    weather_data->rtc_date = date;
    weather_data->rtc_month = month;
    weather_data->rtc_year = year;

    // Print the date and time to Serial Monitor
    USBSerial.print(F("Date: "));
    USBSerial.print(date);
    USBSerial.print(F("/"));
    USBSerial.print(month);
    USBSerial.print(F("/20"));
    USBSerial.print(year);
    USBSerial.print(F("  Time: "));
    USBSerial.print(hour);
    USBSerial.print(F(":"));
    USBSerial.print(min);
    USBSerial.print(F(":"));
    USBSerial.println(sec);

}
