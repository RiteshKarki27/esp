#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "driver/i2c.h"

static const char *TAG = "main";

/*Pin assignment:
 *
 * - master:
 *    GPIO14 is assigned as the data signal of i2c master port
 *    GPIO2 is assigned as the clock signal of i2c master port
 * - read the sensor data, if connected.*/

#define I2C_MASTER_SCL_IO 			2
#define I2C_MASTER_SDA_IO			14
#define I2C_MASTER_NUM				I2C_NUM_0
#define I2C_MASTER_TX_BUF_DISABLE	0
#define I2C_MASTER_RX_BUF_DISABLE	0

#define BMP180_ADDR 				0x77
#define WRITE_BIT					I2C_MASTER_WRITE
#define READ_BIT					I2C_MASTER_READ
#define ACK_CHECK_EN				0X1
#define ACK_CHECK_DIS				0X0
#define ACK_VAL						0X0
#define NACK_VAL					0X1
#define LAST_NACK_VAL				0X2

/*Define BMP180 register addresses*/
#define OUT_XLSB					0xF8
#define OUT_LSB						0xF7
#define OUT_MSB						0xF6
#define CTRL_MEAS					0xF4
#define SOFT_RESET					0xE0
#define ID							0xD0
/*calib21 to calib 0                  0xBF down to 0xAA*/
#define AC1_MSB 					0xAA
#define AC1_LSB						0xAB	
#define AC2_MSB						0xAC
#define AC2_LSB 					0xAD 						

static esp_err_t i2c_master_init()
{
	int i2c_master_port = I2C_MASTER_NUM;
	i2c_config_t conf;
	conf.mode - I2C_MODE_MASTER;
	conf.sda_io_num = I2C_MASTER_SDA_IO;
	conf.sda_pullup_en = 1;
	conf.scl_io_num = I2C_MASTER_SCL_IO;
	conf.scl_pullup_en = 1;
	conf.clk_stretch_tick = 300; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.
	ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode));
	ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
	return ESP_OK;
}

//code to write to bmp180

static esp_err_t i2c_master_bmp180_write(i2c_port_t i2c_num, uint8_t reg_address, uint8_t *data, size_t data_len)
{
	int ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, BMP180_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg_address, ACK_CHECK_EN);
	i2c_master_write(cmd, data, data_len, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return ret;
}

static esp_err_t i2c_master_bmp180_read(i2c_port_t i2c_num, uint8_t reg_address, uint8_t *data, size_t data_len)
{
	int ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, BMP180_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg_address, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

	if (ret != ESP_OK)
	{
		return ret;
	}
	md = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, BMP180_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, data, data_len, LAST_NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

	return ret;
}

static esp_err_t i2c_master_bmp180_init(i2c_port_t i2c_num)
{
	uint8_t cmd_data;
	uint8_t Callib_Data[22] = {0};
	vTaskDelay(100 / portTICK_PERIOD_MS);
	i2c_master_init();
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC1_MSB, &Callib_Data[0], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC1_LSB, &Callib_Data[1], 1));
}
void app_main() {
	xTaskCreate(temperature_task, "temperature task", 2048, NULL,
			tskIDLE_PRIORITY, NULL);
}
