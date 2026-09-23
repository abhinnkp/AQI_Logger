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
#include "ltr390.h"//UV Radiation
#include "data_structure.h"

extern weather_station_global_structure_t weather_data;

//-------------------------------------------------------------------------------------------------
/* Write 8-bit register */
static ltr390_i2c_status_t writeRegister(uint8_t reg, uint8_t value)
{
  if (I2C_Write(LTR390_ADDR, reg, &value, 1) != ESP_OK)
  {
      USBSerial.print("I2C write error, reg 0x");
      USBSerial.println(reg, HEX);
      return LTR390_I2C_ERR_WRITE;
  }

  return LTR390_I2C_OK;
}


//-------------------------------------------------------------------------------------------------
/* Read 8-bit register */
static ltr390_i2c_status_t readRegister8(uint8_t reg, uint8_t *value)
{
  if (I2C_Read(LTR390_ADDR, reg, value, 1) != ESP_OK)
  {
      return LTR390_I2C_ERR_READ_LEN;
  }
  return LTR390_I2C_OK;
}

//-------------------------------------------------------------------------------------------------
/* Read 24-bit register */
static ltr390_i2c_status_t readRegister24(uint8_t reg, uint32_t *value)
{
  uint8_t data[3];

  if (I2C_Read(LTR390_ADDR, reg, data, 3) != ESP_OK)
  {
      return LTR390_I2C_ERR_READ_LEN;
  }

  *value  = (uint32_t)data[0];
  *value |= (uint32_t)data[1] << 8;
  *value |= (uint32_t)data[2] << 16;

  return LTR390_I2C_OK;
}

//-------------------------------------------------------------------------------------------------
void init_LTR390 (void)
{
  uint8_t partID;
    if (readRegister8(LTR390_PART_ID, &partID) != LTR390_I2C_OK)
    {
      USBSerial.println("ERROR: LTR390 not responding");
    }

    USBSerial.print("LTR390 Part ID: 0x");
    USBSerial.println(partID, HEX);

    if (partID != 0xB2)
    {
      USBSerial.println("ERROR: Invalid LTR390 ID");
    }


  // Enable in UV mode
  writeRegister(LTR390_MAIN_CTRL, LTR390_MODE_UVS);

  // Set resolution = 20-bit, measurement rate = 400ms
  writeRegister(LTR390_MEAS_RATE, 0x00);

  // Set gain = 18
  writeRegister(LTR390_GAIN, 0x04);

}

//-------------------------------------------------------------------------------------------------
void read_LTR390 (void)
{
    // Check if new data is ready
  uint8_t status;
  if (readRegister8(LTR390_STATUS, &status) != LTR390_I2C_OK)
  {
    USBSerial.println("Failed to read STATUS");
    sensor_valid.ltr390 = false;
    return;
  }

  if (status & 0x08)
  {
    uint32_t uv_raw;
    if (readRegister24(LTR390_UVS_DATA_0, &uv_raw) == LTR390_I2C_OK)
    {
      weather_data.uv_raw = uv_raw;

      // Convert to UV Index
      weather_data.uvi = weather_data.uv_raw / UV_SENSITIVITY;

      // Convert UVI to irradiance (µW/cm²)
      weather_data.uv_irradiance = weather_data.uvi * UVI_TO_IRRADIANCE;

      USBSerial.print(F("UV Raw: "));
      USBSerial.print(weather_data.uv_raw);
      USBSerial.print(F(" | UVI: "));
      USBSerial.print(weather_data.uvi, 2);
      USBSerial.print(F(" | Irradiance: "));
      USBSerial.print(weather_data.uv_irradiance, 2);
      USBSerial.println(F(" µW/cm²"));
      sensor_valid.ltr390 = true;
    }
    else {
      USBSerial.println("Failed to read UV data");
      sensor_valid.ltr390 = false;
    }
  }
}