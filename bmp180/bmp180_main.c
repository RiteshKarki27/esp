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


void app_main() {
	xTaskCreate(temperature_task, "temperature task", 2048, NULL,
			tskIDLE_PRIORITY, NULL);
}
