#ifndef __TB600B_H__
#define __TB600B_H__

#include <Arduino.h>
#include "debug_serial.h"
#include "data_structure.h"
#include "i2c_20k.h"

/* ── Raw sensor parameter & data structs ──────────── */

struct SensorParameter
{
    uint8_t  decimal;
    uint8_t  sensorType;
    uint8_t  unit;
    uint16_t range;
};

struct SensorData
{
    float ppm;          // concentration in ppm (from sensor)
    float temperature;  // in °C
    float humidity;     // in %RH
};

struct TB600B_t
{
    uint8_t addr;
};

bool TB600B_begin(TB600B_t *sensor);
bool TB600B_readParameter(TB600B_t *sensor, SensorParameter *param);
bool TB600B_readData(TB600B_t *sensor, SensorData *data);


void I2C_gas_sensor_init(void);
void I2C_gas_sensor_read(void);
void I2C_gas_sensor_print(void);

#endif