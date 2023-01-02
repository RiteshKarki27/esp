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
#define ACK_CHECK_EN				0x1
#define ACK_CHECK_DIS				0x0
#define ACK_VAL						0x0
#define NACK_VAL					0x1
#define LAST_NACK_VAL				0x2

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
#define AC3_MSB						0xAE
#define AC3_LSB						0xAF
#define AC4_MSB						0xB0
#define AC4_LSB						0xB1
#define AC5_MSB						0xB2
#define AC5_LSB						0xB3
#define AC6_MSB						0xB4
#define AC6_LSB						0xB5
#define B1_MSB						0xB6
#define B1_LSB						0xB7
#define B2_MSB						0xB8
#define B2_LSB						0xB9
#define MB_MSB 						0xBA
#define MB_LSB 						0xBB
#define MC_MSB 						0xBC
#define MC_LSB 						0xBD
#define MD_MSB 						0xBE
#define MD_LSB 						0xBF
#define atm 						101325

static sint8_t Callib_Data[22] = {0};
static sint16_t AC1, AC2, AC3, AC4, AC5, AC6, B1, B2, MB, MC, MD;
static float X1, X2, X3, B4, B5. B6. B7, temp, press;

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
	// uint8_t Callib_Data[22] = {0};
	vTaskDelay(100 / portTICK_PERIOD_MS);
	i2c_master_init();
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC1_MSB, &Callib_Data[0], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC1_LSB, &Callib_Data[1], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC2_MSB, &Callib_Data[2], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC2_LSB, &Callib_Data[3], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC3_MSB, &Callib_Data[4], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC3_LSB, &Callib_Data[5], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC4_MSB, &Callib_Data[6], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC4_LSB, &Callib_Data[7], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC5_MSB, &Callib_Data[8], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC5_LSB, &Callib_Data[9], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC6_MSB, &Callib_Data[10], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, AC6_LSB, &Callib_Data[11], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, B1_MSB, &Callib_Data[12], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, B1_LSB, &Callib_Data[13], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, B2_MSB, &Callib_Data[14], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, B2_LSB, &Callib_Data[15], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, MB_MSB, &Callib_Data[16], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, MB_LSB, &Callib_Data[17], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, MC_MSB, &Callib_Data[18], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, MC_LSB, &Callib_Data[19], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, MD_MSB, &Callib_Data[20], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, MD_LSB, &Callib_Data[21], 1));
	return ESP_OK;
}

static void read_callibration_data (void)
{
	AC1 = ((Callib_Data[0] << 8) | Callib_Data[1]);
	AC2 = ((Callib_Data[2] << 8) | Callib_Data[3]);
	AC3 = ((Callib_Data[4] << 8) | Callib_Data[5]);
	AC4 = ((Callib_Data[6] << 8) | Callib_Data[7]);
	AC5 = ((Callib_Data[8] << 8) | Callib_Data[9]);
	AC6 = ((Callib_Data[10] << 8) | Callib_Data[11]);
	B1 = ((Callib_Data[12] << 8) | Callib_Data[13]);
	B2 = ((Callib_Data[14] << 8) | Callib_Data[15]);
	MB = ((Callib_Data[16] << 8) | Callib_Data[17]);
	MC = ((Callib_Data[18] << 8) | Callib_Data[19]);
	MD = ((Callib_Data[20] << 8) | Callib_Data[21]);
}

//Uncompensated temp
uint16_t Get_UT (void)
{
	uint8_t val_to_write = 0x2E;
	uint8_t Temp_RAW[2] = {0};
	ESP_ERROR_CHECK(i2c_master_bmp180_write(i2c_num, CTRL_MEAS, &val_to_write, 1));
	
	vTaskDelay(6 / portTICK_PERIOD_MS);  // wait 4.5 ms

	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, OUT_MSB, &Temp_RAW[0], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, OUT_LSB, &Temp_RAW[1], 1));	

	return ((Temp_RAW[0]<<8) + Temp_RAW[1]);
}

//get Actual Temperature
float BMP180_ActualTemp (void)
{
	uint16_t UT = Get_UT ();
	X1 = ((UT - AC6) * (AC5 / (pow(2, 15))));
	X2 = ((MC * (pow(2, 11))) / (X1 + MD));
	B5 = X1 + X2;
	temp = (B5 + 8) / (pow(2, 4));
	return temp / 10.0;
}

// Get uncompensated Pressure
uint32_t Get_UP (int oss)   // oversampling setting 0,1,2,3
{
	uint8_t val_to_write = 0x34+(oss<<6);
	uint8_t raw_pressure[3] = {0};
	ESP_ERROR_CHECK(i2c_example_master_mpu6050_write(i2c_num, CTRL_MEAS, &val_to_write, 1));
	switch (oss)
	{
		case (0):
			vTaskDelay(5 / portTICK_PERIOD_MS);
			break;
		case (1):
			vTaskDelay(8 / portTICK_PERIOD_MS);
			break;
		case (2):
			vTaskDelay(14 / portTICK_PERIOD_MS);
			break;
		case (3):
			vTaskDelay(26 / portTICK_PERIOD_MS);
			break;
	}
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, OUT_MSB, &raw_pressure[0], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, OUT_LSB, &raw_pressure[1], 1));
	ESP_ERROR_CHECK(i2c_master_bmp180_read(i2c_num, OUT_XLSB, &raw_pressure[2], 1));
	return (((raw_pressure[0]<<16)+(raw_pressure[1]<<8)+raw_pressure[2]) >> (8-oss));
}

float BMP180_ActualPress (int oss)
{
	UP = Get_UP(oss);
	X1 = ((UT - AC6) * (AC5 / (pow(2, 15))));
	X2 = ((MC * (pow(2, 11))) / (X1 + MD));
	B5 = X1 + X2;
	B6 = B5 - 4000;
	X1 = (B2 * (B6 * B6 / (pow(2, 12)))) / (pow(2, 11));
	X2 = AC2 * B6 / (pow(2, 11));
	X3 = X1 + X2;
	B3 = (((AC1 * 4 + X3) << oss) + 2) / 4;
	X1 = AC3 * B6 / pow(2, 13);
	X2 = (B1 * (B6 * B6 / (pow(2, 12)))) / (pow(2, 16));
	X3 = ((X1 + X2) + 2) / pow(2, 2);
	B4 = AC4 * (unsigned long)(X3 + 32768) / (pow(2, 15));
	B7 = ((unsigned long)UP - B3) * (50000 >> oss);
	if (B7<0x80000000) 
		press = (B7 * 2) / B4;
	else 
		press = (B7 / B4) * 2;

	X1 = (Press / (pow(2, 8))) * (Press / (pow(2, 8)));
	X1 = (X1 * 3038) / (pow(2, 16));
	X2 = (-7357 * Press) / (pow(2, 16));
	press = press + (X1 + X2 + 3791) / (pow(2, 4));

	return press;
}

float BMP180_Altitude (int oss)
{
	BMP180_ActualPress (oss);
	return 44330 * (1 - (pow(((float)Press / (float)atm), 0.19029495718)));
}


void app_main() {
	xTaskCreate(temperature_task, "temperature task", 2048, NULL,
			tskIDLE_PRIORITY, NULL);
}
