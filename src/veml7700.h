#ifndef __VEML7700_H
#define __VEML7700_H

#include <Arduino.h>
//#include <Wire.h>
#include "i2c_20k.h"


#define VEML7700_ADDR       0x10    // I2C address of VEML7700
#define ALS_CONF_REG        0x00    // Register addresses
#define ALS_DATA_REG        0x04

// #define SDA_PIN             47
// #define SCL_PIN             21

// I2C Error handler
typedef enum
{
  VEML7700_I2C_OK = 0,
  VEML7700_I2C_ERR_BEGIN,
  VEML7700_I2C_ERR_WRITE,
  VEML7700_I2C_ERR_READ_LEN
} veml7700_i2c_status_t;


void veml7700_init(void);
void veml7700_task(void);

#endif // VEML7700_H
