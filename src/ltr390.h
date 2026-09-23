#ifndef LTR390_H
#define LTR390_H

#include <Arduino.h>
#include "i2c_20k.h"
//#include <Wire.h>

// #define SDA_PIN                   47
// #define SCL_PIN                   21
#define LTR390_ADDR               0x53      // I2C address

// Register addresses
#define LTR390_MAIN_CTRL          0x00
#define LTR390_MEAS_RATE          0x04
#define LTR390_GAIN               0x05
#define LTR390_PART_ID            0x06
#define LTR390_STATUS             0x07
#define LTR390_UVS_DATA_0         0x10

// Modes
#define LTR390_MODE_ALS           0x02      // Enable + ALS
#define LTR390_MODE_UVS           0x0A      // Enable + UVS

// Constants from datasheet
#define UV_SENSITIVITY            2300.0    // counts per UVI (gain=18, 20-bit)
#define UVI_TO_IRRADIANCE         2.5       // µW/cm² per UVI

// I2C error status
typedef enum {
    LTR390_I2C_OK = 0,
    LTR390_I2C_ERR_BEGIN,
    LTR390_I2C_ERR_WRITE,
    LTR390_I2C_ERR_READ_LEN
} ltr390_i2c_status_t;



void init_LTR390 ();

void read_LTR390 ();


#endif // LTR390_H