struct SensorValidFlags { bool bme680; bool s300e; bool sds011; bool ltr390; bool veml7700; bool gas; };
extern SensorValidFlags sensor_valid;
#include "tb600b.h"

#define REG_REALTIME  0xD1
#define REG_PARAMETER 0xD2
#define SENSOR_COUNT   4


TB600B_t co  = {0x20};
TB600B_t o3  = {0x21};
TB600B_t no2 = {0x22};
TB600B_t so2 = {0x23};

static bool sensor_connected[SENSOR_COUNT] = {false};

extern weather_station_global_structure_t weather_data;


static bool TB600B_checkChecksum(uint8_t *buf, uint8_t len)
{
    uint8_t sum = 0;

    for (int i = 0; i < len - 1; i++)
        sum += buf[i];

    return ((sum & 0xFF) == buf[len - 1]);
}


bool TB600B_begin(TB600B_t *sensor)
{
    uint8_t rx[6];

    return (I2C0_Read(sensor->addr, REG_PARAMETER, rx, 6) == ESP_OK);
}


bool TB600B_readParameter(TB600B_t *sensor, SensorParameter *param)
{
    uint8_t rx[6];

    if (I2C0_Read(sensor->addr, REG_PARAMETER, rx, 6) != ESP_OK)
        return false;

    if (!TB600B_checkChecksum(rx, 6))
        return false;

    param->decimal    = rx[0];
    param->sensorType = rx[1];
    param->unit       = rx[2];
    param->range      = ((uint16_t)rx[3] << 8) | rx[4];

    return true;
}

bool TB600B_readData(TB600B_t *sensor, SensorData *data)
{
    uint8_t rx[7];

    if (I2C0_Read(sensor->addr, REG_REALTIME, rx, 7) != ESP_OK)
        return false;

    uint8_t sum = 0;

    for (int i = 0; i < 6; i++)
        sum += rx[i];

    if ((sum & 0xFF) != rx[6])
        return false;

    SensorParameter param;

    if (!TB600B_readParameter(sensor, &param))
        return false;

    uint16_t rawConc = ((uint16_t)rx[0] << 8) | rx[1];

    float divisor = 1.0f;

    for (int i = 0; i < param.decimal; i++)
        divisor *= 10.0f;

    data->ppm = rawConc / divisor;

    int16_t  rawTemp = ((int16_t)rx[2] << 8) | rx[3];
    uint16_t rawRH   = ((uint16_t)rx[4] << 8) | rx[5];

    data->temperature = rawTemp / 100.0f;
    data->humidity    = rawRH / 100.0f;

    return true;
}

void I2C_gas_sensor_init(void)
{
    TB600B_t *sensors[] = {&co, &o3, &no2, &so2};
    const char *names[] = {"CO", "O3", "NO2", "SO2"};

    for (int i = 0; i < 4; i++)
    {
        if (TB600B_begin(sensors[i]))
            USBSerial.printf("%s Connected\n", names[i]);
        else
            USBSerial.printf("%s Not Found\n", names[i]);
    }
    USBSerial.println();
}

void I2C_gas_sensor_read(void)
{
    SensorData d;
    bool any_success = false;

    if (TB600B_readData(&co, &d))
    {
        weather_data.CO_ppm = d.ppm;
        weather_data.CO_mg  = d.ppm;
        weather_data.CO_T   = d.temperature;
        weather_data.CO_H   = d.humidity;
        any_success = true;
    }
    if (TB600B_readData(&o3, &d))
    {
        weather_data.O3_ppm = d.ppm;
        weather_data.O3_mg  = d.ppm;
        weather_data.O3_T   = d.temperature;
        weather_data.O3_H   = d.humidity;
        any_success = true;
    }
    if (TB600B_readData(&no2, &d))
    {
        weather_data.NO2_ppm = d.ppm;
        weather_data.NO2_mg  = d.ppm;
        weather_data.NO2_T   = d.temperature;
        weather_data.NO2_H   = d.humidity;
        any_success = true;
    }
    if (TB600B_readData(&so2, &d))
    {
        weather_data.SO2_ppm = d.ppm;
        weather_data.SO2_mg  = d.ppm;
        weather_data.SO2_T   = d.temperature;
        weather_data.SO2_H   = d.humidity;
        any_success = true;
    }

    if (any_success) {
        sensor_valid.gas = true;
    } else {
        sensor_valid.gas = false;
    }
}

void I2C_gas_sensor_print(void)
{
    USBSerial.println("=============================================================");
    USBSerial.printf("CO  : %.3f ppm   T=%.2f°C  H=%.2f%%\n", weather_data.CO_ppm, weather_data.CO_T, weather_data.CO_H);
    USBSerial.printf("O3  : %.3f ppm   T=%.2f°C  H=%.2f%%\n", weather_data.O3_ppm, weather_data.O3_T, weather_data.O3_H);
    USBSerial.printf("NO2 : %.3f ppm   T=%.2f°C  H=%.2f%%\n", weather_data.NO2_ppm, weather_data.NO2_T, weather_data.NO2_H);
    USBSerial.printf("SO2 : %.3f ppm   T=%.2f°C  H=%.2f%%\n", weather_data.SO2_ppm, weather_data.SO2_T, weather_data.SO2_H);
    USBSerial.println("=============================================================\n");
}