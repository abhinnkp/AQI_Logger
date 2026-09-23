#ifndef __S300E_H
#define __S300E_H

#include <Arduino.h>
#include "i2c_20k.h"
//#include <Wire.h>
#include <stdint.h>

/* ================= CONFIG ================= */
// #define I2C_SDA             47
// #define I2C_SCL             21
#define S300E_ADDR          0x31
#define CMD_READ            'R'
//#define I2C_FREQ            100000
#define FRAME_LEN           16
#define BASE_CO2_PPM        400
#define SAMPLE_TIME_MS      5000

/* ================= FRAME ================= */
typedef struct
{
    uint8_t config;       // Byte 0
    uint8_t type;         // Byte 1
    uint8_t co2_delta;    // Byte 2
    uint8_t rsvd1[4];     // Byte 3–6
    uint8_t status;       // Byte 7
    uint8_t padding[8];   // Byte 8–15
} __attribute__((packed)) s300e_frame_t;

/* ================= API ================= */
void S300E_Init(void);
bool S300E_ReadCO2(uint16_t *ppm);
void S300E_data(void);

#endif
