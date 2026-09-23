//Structure Body - Collection of variables
#ifndef __DATA_STRUCTURE_H
#define __DATA_STRUCTURE_H

#include<Arduino.h>
#include<stdint.h>
#include<string.h>

//-------------------------------------------------------------------------------------------------
//Collection of all varaibles - (Note: It is possible different sensors may collect different types of data)
typedef struct
{
    // Gas reading
    float                       CO_mg;
    float                       CO_ppm;
    float                       CO_T;
    float                       CO_H;

    float                       O3_mg;
    float                       O3_ppm;
    float                       O3_T;
    float                       O3_H;

    float                       NO2_mg;
    float                       NO2_ppm;
    float                       NO2_T;
    float                       NO2_H;

    float                       H2S_mg;
    float                       H2S_ppm;
    float                       H2S_T;
    float                       H2S_H;

    float                       SO2_mg;
    float                       SO2_ppm;
    float                       SO2_T;
    float                       SO2_H;

    float                       NH3_mg;
    float                       NH3_ppm;
    float                       NH3_T;
    float                       NH3_H;
    float                       NS_db;

    uint16_t                    CO2_ppm;             // CO2 data in ppm

    float                       lux;                 // Ambient light in lux

    uint32_t                    uv_raw;              // Raw UV sensor counts
    float                       uvi;                 // UV Index
    float                       uv_irradiance;       // UV irradiance in µW/cm²

    float                       pm1;                 // SPS30 PM1.0 (µg/m³)
    float                       pm2_5;               // SPS30 PM2.5 (µg/m³)
    float                       pm4;                 // SPS30 PM4.0 (µg/m³)
    float                       pm10;                // SPS30 PM10 (µg/m³)

    float                       nc0_5;               // SPS30 Number concentration 0.5µm (#/cm³)
    float                       nc1_0;               // SPS30 Number concentration 1.0µm (#/cm³)
    float                       nc2_5;               // SPS30 Number concentration 2.5µm (#/cm³)
    float                       nc4_0;               // SPS30 Number concentration 4.0µm (#/cm³)
    float                       nc10;                // SPS30 Number concentration 10µm (#/cm³)

    float                      typicalSize;          // Typical particle size (µm)

    // BME680 sensor
    float                       BME680_Temperature;  // Temperature read by BME sensor - in Celc
    float                       BME680_Humidity;     //RH % read by BME sensor
    float                       BME680_Pressure;     //Air Pressure - hPA read by BME sensor


    // RTC fields
    uint8_t                     rtc_sec;
    uint8_t                     rtc_min;
    uint8_t                     rtc_hour;
    uint8_t                     rtc_day;
    uint8_t                     rtc_date;
    uint8_t                     rtc_month;
    uint8_t                     rtc_year;



}weather_station_global_structure_t;
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//Function : For Initialize Weather station structure variables
//Argument : Pointer of weather_station_global_structure_t
//Return   : void
void init_weather_station_global_structure(weather_station_global_structure_t *);
//-------------------------------------------------------------------------------------------------


#endif