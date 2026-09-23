struct SensorValidFlags {
    bool bme680;
    bool s300e;
    bool sds011;
    bool ltr390;
    bool veml7700;
    bool gas;
};
extern SensorValidFlags sensor_valid;
#include "debug_serial.h"
#include "s300e.h"//CO2 SENSOR
#include "data_structure.h"

extern weather_station_global_structure_t weather_data;

/* ================= STORAGE ================= */
static s300e_frame_t frame;

/* ================= INIT ================= */
void S300E_Init(void)
{
//    Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
}

/* ================= READ ================= */
bool S300E_ReadCO2(uint16_t *ppm)
{
    if (ppm == nullptr)
    {
        return false;
    }

    /* Send Read Command */
    uint8_t cmd = CMD_READ;

    if (I2C_WriteRaw(S300E_ADDR, &cmd, 1) != ESP_OK)
    {
        return false;
    }

    delay(2);

    /* Read 16-byte frame */
    uint8_t raw[FRAME_LEN];

    if (I2C_ReadRaw(S300E_ADDR, raw, FRAME_LEN) != ESP_OK)
    {
        return false;
    }

    memcpy(&frame, raw, FRAME_LEN);

    /* Sanity check */
    if (frame.config != 0x08)
    {
        return false;
    }

    *ppm = BASE_CO2_PPM + frame.co2_delta;

    return true;
}


void S300E_data(void)
{
    uint16_t co2_ppm = 0;

    if (S300E_ReadCO2(&co2_ppm))
    {
        weather_data.CO2_ppm = co2_ppm;
        sensor_valid.s300e = true;

        USBSerial.print("CO2 = ");
        USBSerial.print(weather_data.CO2_ppm);
        USBSerial.println(" ppm");
    }
    else
    {
        USBSerial.println("CO2 read error");
        sensor_valid.s300e = false;
    }

    delay(SAMPLE_TIME_MS);
}