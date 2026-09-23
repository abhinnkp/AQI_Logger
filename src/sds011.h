#ifndef SDS011_H
#define SDS011_H

#include <Arduino.h>

#define SDS011_BAUD 9600

// Use your existing pins here
// If these are already defined elsewhere,
// don't duplicate them.

// UART pins (change if needed)
#define PM_RX_PIN 17
#define PM_TX_PIN 16

void sds011_init();

bool readMeasurement();

void sds011_task();

void sds011_print();

bool sds011_is_connected();

void sendCommand(uint8_t *cmd, uint8_t len);

bool startMeasurement();

bool stopMeasurement();

#endif

// #ifndef __SPS30_H
// #define __SPS30_H


// #include <Arduino.h>
// #include <Wire.h>

// // SPS30 I2C address
// #define SPS30_ADDR          0x69


// #define SDA_PIN             8
// #define SCL_PIN             9


// bool startMeasurement();
// void stopMeasurement();
// bool readMeasurement();
// void sps_30_init();
// void sps_30_data();


// #endif
