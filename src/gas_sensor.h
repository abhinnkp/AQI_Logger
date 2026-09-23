#ifndef __GAS_SENSOR_H
#define __GAS_SENSOR_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "data_structure.h"

// ---------------- UART ----------------
#define GAS_RX_PIN          38
#define GAS_TX_PIN          39


// ---------------- MUX PINS ----------------
#define S0                  40
#define S1                  41
#define EN1                 36
#define EN2                 37

// ---------------- CONFIG ----------------
#define RESPONSE_TIMEOUT    3000

// ---------------- FUNCTION ----------------
void gas_sensor_init(void);
void gas_sensor_task();
void gas_sensor_print();
bool readNoiseFrame(void);

#endif
