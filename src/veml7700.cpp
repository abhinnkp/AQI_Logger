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
#include "veml7700.h"//Ambiant Light
#include "data_structure.h"

extern weather_station_global_structure_t weather_data;

//-------------------------------------------------------------------------------------------------
/* Write 16-bit register */
static veml7700_i2c_status_t write16(uint8_t reg, uint16_t value)
{
    uint8_t data[2];

    data[0] = value & 0xFF;         // LSB
    data[1] = (value >> 8) & 0xFF;  // MSB

    if (I2C_Write(VEML7700_ADDR, reg, data, 2) != ESP_OK)
    {
        USBSerial.println("I2C write error");
        return VEML7700_I2C_ERR_WRITE;
    }

    return VEML7700_I2C_OK;
}

//-------------------------------------------------------------------------------------------------
/* Read 16-bit register */
static veml7700_i2c_status_t read16(uint8_t reg, uint16_t *value)
{
    uint8_t data[2];

    if (I2C_Read(VEML7700_ADDR, reg, data, 2) != ESP_OK)
    {
        USBSerial.println("I2C read error");
        return VEML7700_I2C_ERR_READ_LEN;
    }

    *value = ((uint16_t)data[1] << 8) | data[0];

    return VEML7700_I2C_OK;
}

//-------------------------------------------------------------------------------------------------
/* Initialize VEML7700 */
void veml7700_init(void)
{
    //Wire.begin(SDA_PIN, SCL_PIN);
    veml7700_i2c_status_t status = write16(ALS_CONF_REG, 0x0000); // Gain=1/4, IT=100ms
    if (status != VEML7700_I2C_OK) {
        USBSerial.println("VEML7700 config failed!");
    } else {
        USBSerial.println("VEML7700 initialized successfully");
    }
}

//-------------------------------------------------------------------------------------------------
void veml7700_task(void)
{
    uint16_t als_raw;
    veml7700_i2c_status_t status = read16(ALS_DATA_REG, &als_raw);

    if (status == VEML7700_I2C_OK)
    {
        float lux = als_raw * 0.0576f;      // default conversion factor
        weather_data.lux = lux;             // store into global structure
        sensor_valid.veml7700 = true;
        USBSerial.print("Ambient Light: ");
        USBSerial.print(weather_data.lux);
        USBSerial.println(" lx");
    } else {
        USBSerial.println("Failed to read ALS data");
        sensor_valid.veml7700 = false;
    }
}
