#include "i2c_20k.h"


// ============================================================
// INTERNAL I2C INITIALIZATION
// ============================================================

static esp_err_t I2C_InitPort(
    i2c_port_t port,
    int sdaPin,
    int sclPin)
{
    i2c_config_t conf = {};

    conf.mode = I2C_MODE_MASTER;

    conf.sda_io_num = (gpio_num_t)sdaPin;
    conf.scl_io_num = (gpio_num_t)sclPin;

    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;

    conf.master.clk_speed = I2C_FREQUENCY;

    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;


    esp_err_t err = i2c_param_config(
        port,
        &conf);

    if (err != ESP_OK)
    {
        return err;
    }


    err = i2c_driver_install(
        port,
        I2C_MODE_MASTER,
        0,
        0,
        0);

    return err;
}


// ============================================================
// I2C HAL INIT
// ============================================================

void I2C_HAL_Init(void)
{
    esp_err_t err;


    // --------------------------------------------------------
    // I2C0
    // --------------------------------------------------------

    err = I2C_InitPort(
        I2C0_PORT,
        I2C0_SDA_PIN,
        I2C0_SCL_PIN);

    if (err != ESP_OK)
    {
        Serial.printf(
            "I2C0 Init Failed: %s\n",
            esp_err_to_name(err));
    }
    else
    {
        Serial.println("I2C0 Initialized");
    }


    // --------------------------------------------------------
    // I2C1
    // --------------------------------------------------------

    err = I2C_InitPort(
        I2C1_PORT,
        I2C1_SDA_PIN,
        I2C1_SCL_PIN);

    if (err != ESP_OK)
    {
        Serial.printf(
            "I2C1 Init Failed: %s\n",
            esp_err_to_name(err));
    }
    else
    {
        Serial.println("I2C1 Initialized");
    }
}


// ============================================================
// I2C WRITE - INTERNAL COMMON FUNCTION
// ============================================================

static esp_err_t I2C_WritePort(
    i2c_port_t port,
    uint8_t devAddr,
    uint8_t reg,
    const uint8_t *data,
    size_t len)
{
    i2c_cmd_handle_t cmd =
        i2c_cmd_link_create();

    if (cmd == NULL)
    {
        return ESP_ERR_NO_MEM;
    }


    i2c_master_start(cmd);


    i2c_master_write_byte(
        cmd,
        (devAddr << 1) | I2C_MASTER_WRITE,
        true);


    i2c_master_write_byte(
        cmd,
        reg,
        true);


    if (len)
    {
        i2c_master_write(
            cmd,
            (uint8_t *)data,
            len,
            true);
    }


    i2c_master_stop(cmd);


    esp_err_t err =
        i2c_master_cmd_begin(
            port,
            cmd,
            pdMS_TO_TICKS(100));


    i2c_cmd_link_delete(cmd);


    return err;
}


// ============================================================
// I2C READ - INTERNAL COMMON FUNCTION
// ============================================================

static esp_err_t I2C_ReadPort(
    i2c_port_t port,
    uint8_t devAddr,
    uint8_t reg,
    uint8_t *data,
    size_t len)
{
    if (data == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }


    i2c_cmd_handle_t cmd =
        i2c_cmd_link_create();

    if (cmd == NULL)
    {
        return ESP_ERR_NO_MEM;
    }


    // --------------------------------------------------------
    // Write register address
    // --------------------------------------------------------

    i2c_master_start(cmd);


    i2c_master_write_byte(
        cmd,
        (devAddr << 1) | I2C_MASTER_WRITE,
        true);


    i2c_master_write_byte(
        cmd,
        reg,
        true);


    // --------------------------------------------------------
    // Repeated START
    // --------------------------------------------------------

    i2c_master_start(cmd);


    i2c_master_write_byte(
        cmd,
        (devAddr << 1) | I2C_MASTER_READ,
        true);


    // --------------------------------------------------------
    // Read data
    // --------------------------------------------------------

    if (len > 1)
    {
        i2c_master_read(
            cmd,
            data,
            len - 1,
            I2C_MASTER_ACK);
    }


    i2c_master_read_byte(
        cmd,
        &data[len - 1],
        I2C_MASTER_NACK);


    i2c_master_stop(cmd);


    esp_err_t err =
        i2c_master_cmd_begin(
            port,
            cmd,
            pdMS_TO_TICKS(100));


    i2c_cmd_link_delete(cmd);


    return err;
}


// ============================================================
// I2C WRITE RAW - INTERNAL COMMON FUNCTION
// ============================================================

static esp_err_t I2C_WriteRawPort(
    i2c_port_t port,
    uint8_t devAddr,
    const uint8_t *data,
    size_t len)
{
    i2c_cmd_handle_t cmd =
        i2c_cmd_link_create();

    if (cmd == NULL)
    {
        return ESP_ERR_NO_MEM;
    }


    i2c_master_start(cmd);


    i2c_master_write_byte(
        cmd,
        (devAddr << 1) | I2C_MASTER_WRITE,
        true);


    if (len)
    {
        i2c_master_write(
            cmd,
            (uint8_t *)data,
            len,
            true);
    }


    i2c_master_stop(cmd);


    esp_err_t err =
        i2c_master_cmd_begin(
            port,
            cmd,
            pdMS_TO_TICKS(100));


    i2c_cmd_link_delete(cmd);


    return err;
}


// ============================================================
// I2C READ RAW - INTERNAL COMMON FUNCTION
// ============================================================

static esp_err_t I2C_ReadRawPort(
    i2c_port_t port,
    uint8_t devAddr,
    uint8_t *data,
    size_t len)
{
    if (data == NULL || len == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }


    i2c_cmd_handle_t cmd =
        i2c_cmd_link_create();

    if (cmd == NULL)
    {
        return ESP_ERR_NO_MEM;
    }


    i2c_master_start(cmd);


    i2c_master_write_byte(
        cmd,
        (devAddr << 1) | I2C_MASTER_READ,
        true);


    if (len > 1)
    {
        i2c_master_read(
            cmd,
            data,
            len - 1,
            I2C_MASTER_ACK);
    }


    i2c_master_read_byte(
        cmd,
        &data[len - 1],
        I2C_MASTER_NACK);


    i2c_master_stop(cmd);


    esp_err_t err =
        i2c_master_cmd_begin(
            port,
            cmd,
            pdMS_TO_TICKS(100));


    i2c_cmd_link_delete(cmd);


    return err;
}


// ============================================================
// EXISTING I2C1 API
// ============================================================
// IMPORTANT:
// All your existing sensor drivers continue to work unchanged.
// ============================================================

esp_err_t I2C_Write(
    uint8_t devAddr,
    uint8_t reg,
    const uint8_t *data,
    size_t len)
{
    return I2C_WritePort(
        I2C1_PORT,
        devAddr,
        reg,
        data,
        len);
}


esp_err_t I2C_Read(
    uint8_t devAddr,
    uint8_t reg,
    uint8_t *data,
    size_t len)
{
    return I2C_ReadPort(
        I2C1_PORT,
        devAddr,
        reg,
        data,
        len);
}


esp_err_t I2C_WriteRaw(
    uint8_t devAddr,
    const uint8_t *data,
    size_t len)
{
    return I2C_WriteRawPort(
        I2C1_PORT,
        devAddr,
        data,
        len);
}


esp_err_t I2C_ReadRaw(
    uint8_t devAddr,
    uint8_t *data,
    size_t len)
{
    return I2C_ReadRawPort(
        I2C1_PORT,
        devAddr,
        data,
        len);
}


// ============================================================
// NEW I2C0 API
// ============================================================
// Use these for your new I2C0 sensors.
// ============================================================

esp_err_t I2C0_Write(
    uint8_t devAddr,
    uint8_t reg,
    const uint8_t *data,
    size_t len)
{
    return I2C_WritePort(
        I2C0_PORT,
        devAddr,
        reg,
        data,
        len);
}


esp_err_t I2C0_Read(
    uint8_t devAddr,
    uint8_t reg,
    uint8_t *data,
    size_t len)
{
    return I2C_ReadPort(
        I2C0_PORT,
        devAddr,
        reg,
        data,
        len);
}


esp_err_t I2C0_WriteRaw(
    uint8_t devAddr,
    const uint8_t *data,
    size_t len)
{
    return I2C_WriteRawPort(
        I2C0_PORT,
        devAddr,
        data,
        len);
}


esp_err_t I2C0_ReadRaw(
    uint8_t devAddr,
    uint8_t *data,
    size_t len)
{
    return I2C_ReadRawPort(
        I2C0_PORT,
        devAddr,
        data,
        len);
}