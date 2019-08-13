#include "tc74.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TC74 tc74;

esp_err_t i2c_init()
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = SDA;
    conf.scl_io_num = SCL;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(tc74.I2C_PORT_NUM, &conf);

    return i2c_driver_install(tc74.I2C_PORT_NUM, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}

void select_temperature_register(i2c_cmd_handle_t cmd)
{
    // Slave address
    i2c_master_write_byte(cmd, tc74.I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);

    // Select temperature register
    i2c_master_write_byte(cmd, TC74_TEMP_REG, ACK_CHECK_EN);
}

void select_config_register(i2c_cmd_handle_t cmd)
{
    // Slave address
    i2c_master_write_byte(cmd, tc74.I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);

    // Select temperature register
    i2c_master_write_byte(cmd, TC74_CFG_REG, ACK_CHECK_EN);
}

float extract_value_from_buffer(int temp, temp_unit unit)
{
    // Two's complement conversion
    if (temp & 0x80)
    {
        temp = -1 * ((temp ^ 255) + 1);
    }
    // ---------------------------

    if (unit == Celsius)
    {
        return temp;
    }
    else if (unit == Fahrenheit)
    {
        return (temp * 9.0) / 5.0 + 32;
    }

    return 128;
}

void TC74_init(int port_num, int variant)
{
    tc74.I2C_PORT_NUM = port_num;
    tc74.I2C_ADDRESS = variant;

    i2c_init();
    disable_standby();
}

float read_TC74(temp_unit unit)
{
    uint8_t data[1] = {0};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // START bit
    i2c_master_start(cmd);

    // Select temperature register
    select_temperature_register(cmd);

    // Start reading temperature data
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, tc74.I2C_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, data, 1, NACK_VAL);

    // STOP
    i2c_master_stop(cmd);

    // END transmission
    i2c_master_cmd_begin(tc74.I2C_PORT_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return extract_value_from_buffer(data[0], unit);
}

void enable_standby()
{
    /* Freeze the temperature register */

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // START bit
    i2c_master_start(cmd);

    // Select configuration register
    select_config_register(cmd);

    // Enable standby
    i2c_master_write_byte(cmd, PWRSAVE, ACK_CHECK_EN);

    // STOP
    i2c_master_stop(cmd);

    // END transmission
    i2c_master_cmd_begin(tc74.I2C_PORT_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void disable_standby()
{
    /* Unfreeze the temperature register */

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // START bit
    i2c_master_start(cmd);

    // Select configuration register
    select_config_register(cmd);

    // Disable standby
    i2c_master_write_byte(cmd, NOPWRSAVE, ACK_CHECK_EN);

    // STOP
    i2c_master_stop(cmd);

    // END transmission
    i2c_master_cmd_begin(tc74.I2C_PORT_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    vTaskDelay(250 / portTICK_PERIOD_MS);
}

int is_standby()
{
    uint8_t data[1] = {0};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // START bit
    i2c_master_start(cmd);

    // Select configuration register
    select_config_register(cmd);

    // Start reading data
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, tc74.I2C_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, data, 1, NACK_VAL);

    // STOP
    i2c_master_stop(cmd);

    // END transmission
    i2c_master_cmd_begin(tc74.I2C_PORT_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    switch (data[0])
    {
    case 0x40:
        return 0;
    case 0x80:
        return 1;
    default:
        return -1;
    }
}
