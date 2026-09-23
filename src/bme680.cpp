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
#include "bme680.h"
//#include <Wire.h>
#include "i2c_20k.h"

extern weather_station_global_structure_t weather_data;


/* ---------- BME680 Objects ---------- */
static struct bme68x_dev  bme;
static struct bme68x_conf conf;
static struct bme68x_data data;
static uint8_t n_fields;

/* ---------- I2C Low Level ---------- */
// static int8_t i2c_read(uint8_t reg, uint8_t *buf, uint32_t len, void *intf_ptr)
// {
//     uint8_t dev_addr = *(uint8_t *)intf_ptr;

//     Wire.beginTransmission(dev_addr);
//     Wire.write(reg);
//     if (Wire.endTransmission(false) != 0)
//         return BME68X_E_COM_FAIL;

//     Wire.requestFrom(dev_addr, (uint8_t)len);

//     for (uint32_t i = 0; i < len && Wire.available(); i++)
//         buf[i] = Wire.read();

//     return BME68X_OK;
// }

static int8_t i2c_read(uint8_t reg, uint8_t *buf, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *(uint8_t *)intf_ptr;

    if(I2C_Read(dev_addr, reg, buf, len) == ESP_OK)
        return BME68X_OK;

    return BME68X_E_COM_FAIL;
}

static int8_t i2c_write(uint8_t reg,
                        const uint8_t *buf,
                        uint32_t len,
                        void *intf_ptr)
{
    uint8_t dev_addr = *(uint8_t *)intf_ptr;

    if(I2C_Write(dev_addr, reg, buf, len) == ESP_OK)
        return BME68X_OK;

    return BME68X_E_COM_FAIL;
}

// static int8_t i2c_write(uint8_t reg, const uint8_t *buf, uint32_t len, void *intf_ptr)
// {
//     uint8_t dev_addr = *(uint8_t *)intf_ptr;

//     Wire.beginTransmission(dev_addr);
//     Wire.write(reg);

//     for (uint32_t i = 0; i < len; i++)
//         Wire.write(buf[i]);

//     return (Wire.endTransmission() == 0) ? BME68X_OK : BME68X_E_COM_FAIL;
// }

static void delay_us(uint32_t period, void *)
{
    delayMicroseconds(period);
}

/* ---------- Init ---------- */
void BME680_Init(void)
{
  //  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    static uint8_t dev_addr = BME68X_I2C_ADDR;

    bme.intf      = BME68X_I2C_INTF;
    bme.read      = i2c_read;
    bme.write     = i2c_write;
    bme.delay_us  = delay_us;
    bme.intf_ptr  = &dev_addr;

    if (bme68x_init(&bme) != BME68X_OK)
    {
        USBSerial.println("BME680 init failed");
        //while (1);
    }

    /* Oversampling + filter */
    conf.os_hum  = BME68X_OS_2X;
    conf.os_temp = BME68X_OS_4X;
    conf.os_pres = BME68X_OS_4X;
    conf.filter  = BME68X_FILTER_SIZE_7;
    conf.odr     = BME68X_ODR_NONE;

    bme68x_set_conf(&conf, &bme);

    /* Disable gas heater */
    bme68x_set_heatr_conf(BME68X_DISABLE, NULL, &bme);

    USBSerial.println("BME680 ready");
}

/* ---------- This is your old LOOP logic ---------- */
void BME680_Process()
{
    /* Trigger forced measurement */
    bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);

    //uint32_t meas_dur = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme);
    delay(1000);

    if (bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &bme) == BME68X_OK && n_fields)
    {
        weather_data.BME680_Temperature = data.temperature;
        weather_data.BME680_Humidity    = data.humidity;
        weather_data.BME680_Pressure    = data.pressure / 100.0f;   // Pa → hPa

        USBSerial.print("BME680 T: ");
        USBSerial.print(weather_data.BME680_Temperature, 2);
        USBSerial.print(" °C | H: ");
        USBSerial.print(weather_data.BME680_Humidity, 2);
        USBSerial.print(" % | P: ");
        USBSerial.print(weather_data.BME680_Pressure, 2);
        USBSerial.println(" hPa");
        sensor_valid.bme680 = true;
    }
    else
    {
        USBSerial.println("BME680 read error");
        sensor_valid.bme680 = false;
    }
}
