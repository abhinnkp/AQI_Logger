#include"data_structure.h"
#include<Arduino.h>


//-------------------------------------------------------------------------------------------------
//Function : For Initialize Weather station structure variables
//Argument : Pointer of weather_station_global_structure_t
//Return   : void
void init_weather_station_global_structure(weather_station_global_structure_t *temp)
{

    if(temp == NULL)
    {
        return;
    }

    // Gases sensor
    temp->CO_mg                          = 0.0f;
    temp->CO_ppm                         = 0.0f;
    temp->CO_T                           = 0.0f;
    temp->CO_H                           = 0.0f;

    temp->O3_mg                          = 0.0f;
    temp->O3_ppm                         = 0.0f;
    temp->O3_T                           = 0.0f;
    temp->O3_H                           = 0.0f;

    temp->NO2_mg                         = 0.0f;
    temp->NO2_ppm                        = 0.0f;
    temp->NO2_T                          = 0.0f;
    temp->NO2_H                          = 0.0f;

    temp->H2S_mg                         = 0.0f;
    temp->H2S_ppm                        = 0.0f;
    temp->H2S_T                          = 0.0f;
    temp->H2S_H                          = 0.0f;

    temp->SO2_mg                         = 0.0f;
    temp->SO2_ppm                        = 0.0f;
    temp->SO2_T                          = 0.0f;
    temp->SO2_H                          = 0.0f;

    temp->NH3_mg                         = 0.0f;
    temp->NH3_ppm                        = 0.0f;
    temp->NH3_T                          = 0.0f;
    temp->NH3_H                          = 0.0f;
    temp->NS_db                          = 0.0f;

    // CO2 sensor
    temp->CO2_ppm                        = 0;

    // Ambient Light
    temp->lux                            = 0.0f;

    // UV radiation
    temp->uv_raw                         = 0;
    temp->uvi                            = 0.0f;
    temp->uv_irradiance                  = 0.0f;

    // BME680 sensor
    temp->BME680_Temperature             = 0.0f;
    temp->BME680_Humidity                = 0.0f;
    temp->BME680_Pressure                = 0.0f;

    // PM sensor
    temp->pm1                            = 0.0f;
    temp->pm2_5                          = 0.0f;
    temp->pm4                            = 0.0f;
    temp->pm10                           = 0.0f;

    temp->nc0_5                          = 0.0f;
    temp->nc1_0                          = 0.0f;
    temp->nc2_5                          = 0.0f;
    temp->nc4_0                          = 0.0f;
    temp->nc10                           = 0.0f;

    temp->typicalSize                    = 0.0f;

    // RTC
    temp->rtc_sec                        = 0;
    temp->rtc_min                        = 0;
    temp->rtc_hour                       = 0;
    temp->rtc_day                        = 0;
    temp->rtc_date                       = 0;
    temp->rtc_month                      = 0;
    temp->rtc_year                       = 0;

}
//-------------------------------------------------------------------------------------------------
