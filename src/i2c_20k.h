#ifndef __I2C_20K_H__
#define __I2C_20K_H__

#include <Arduino.h>
#include "driver/i2c.h"

// ============================================================
// I2C0 - NEW SENSOR BUS
// ============================================================
#define I2C0_PORT        I2C_NUM_0
#define I2C0_SDA_PIN     47
#define I2C0_SCL_PIN     21

// ============================================================
// I2C1 - EXISTING SENSOR BUS
// ============================================================
#define I2C1_PORT        I2C_NUM_1
#define I2C1_SDA_PIN     18
#define I2C1_SCL_PIN     48

// ============================================================
// I2C FREQUENCY
// ============================================================
#define I2C_FREQUENCY    20000


// ============================================================
// INITIALIZATION
// ============================================================
// Initializes BOTH I2C0 and I2C1
void I2C_HAL_Init(void);


// ============================================================
// EXISTING I2C1 FUNCTIONS
// ============================================================
// These keep your old API unchanged.
// Existing sensors continue using I2C1.
// ============================================================

esp_err_t I2C_Write(
    uint8_t devAddr,
    uint8_t reg,
    const uint8_t *data,
    size_t len);

esp_err_t I2C_Read(
    uint8_t devAddr,
    uint8_t reg,
    uint8_t *data,
    size_t len);

esp_err_t I2C_WriteRaw(
    uint8_t devAddr,
    const uint8_t *data,
    size_t len);

esp_err_t I2C_ReadRaw(
    uint8_t devAddr,
    uint8_t *data,
    size_t len);


// ============================================================
// NEW I2C0 FUNCTIONS
// ============================================================
// Use these only for sensors connected to I2C0.
// ============================================================

esp_err_t I2C0_Write(
    uint8_t devAddr,
    uint8_t reg,
    const uint8_t *data,
    size_t len);

esp_err_t I2C0_Read(
    uint8_t devAddr,
    uint8_t reg,
    uint8_t *data,
    size_t len);

esp_err_t I2C0_WriteRaw(
    uint8_t devAddr,
    const uint8_t *data,
    size_t len);

esp_err_t I2C0_ReadRaw(
    uint8_t devAddr,
    uint8_t *data,
    size_t len);

#endif