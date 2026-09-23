#ifndef __BME680_H
#define __BME680_H

#include <Arduino.h>
#include "bme68x.h"
#include "data_structure.h"
#include "i2c_20k.h"

/* -------- USER CONFIG -------- */
#define BME68X_I2C_ADDR   0x76
// #define I2C_SDA_PIN      47
// #define I2C_SCL_PIN      21
/* ----------------------------- */

void BME680_Init(void);
void BME680_Process(void);

#endif
