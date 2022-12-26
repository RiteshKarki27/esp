#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include <dht/dht.h>
#include <string.h>
#include <driver/gpio.h>
#include <rom/ets_sys.h>

#define DHT_TIMER_INTERVAL 2
#define DHT_DATA_BITS 40

#ifdef DEBUG_DHT
#define debug(fmt, ...) printf("%s" fmt "\n", "dht: ", ## __VA_ARGS__);
#else
#define debug(fmt, ...) /* (do nothing) */
#endif


static bool dht_await_pin_state(uint8_t pin, uint32_t timeout, bool expected_pin_state, uint32_t *duration)
{
	for(uint32_t i = 0; i < timeout; i += DHT_TIMER_INTERVAL)
	{
		os_delay_us(DHT_TIMER_INTERVAL);
		if(gpio_get_level(pin) == expected_pin_state)
		{
			if(duration)
			{
				*duration = i;
			}
			return true;
		}
	}
	return false;
}

static inline bool dht_fetch_data(dht_sensor_type_t sensor_type, uint8_t pin, bool bits[DHT_DATA_BITS])
{
	uint32_t low_duration;
	uint32_t high_duration;

	gpio_set_level(pin, 0);
	os_delay_us(sensor_type = DHT_TYPE_S17021 ? 500 : 20000); // sensor_type defined in dht.h
	gpio_set_level(pin,1);

	if(!dht_await_pin_state(pin, 40, false, NULL))
	{
		debug("Initializatin error, problem in phase B\n");
		return false;
	}

	if(!dht_await_pin_state(pin, 88, true, NULL))
	{
		debug("Initialization error, problem in phase C\n");
		return false;
	}

	if(!dht_await_pin_state(pin, 88, false, NULL))
	{
		debug("Initialization error, problem in phase D\n");
		return false;
	}

	//read 40 bits of data individually
	for(int i = 0; i < DHT_DATA_BITS; i++)
	{
		if(!dht_await_pin_state(pin, 65, true, &low_duration))
		{
			debug("low bit timeout\n");
			return false;
		}
		if (!dht_await_pin_state(pin, 75, false, &high_duration)) {
			debug("HIGHT bit timeout\n");
			return false;
		}
		bits[i] = high_duration > low_duration;
	}
	return true;
}

static inline int16_t dht_convert_data(dht_sensor_type_t sensor_type, uint8_t msb, uint8_t lsb)
{
	int16_t data;

	if(sensor_type == DHT_TYPE_DHT22)
	{
		data = msb & 0x7F;
		data <<= 8;
		data |= lsb;
		if(msb & BIT(7))
		{
			data = 0 - data;
		}
	}
	else
	{
		data = msb * 10;
	}

	return data;
}

esp_err_t dht_read_data(dht_sensor_type_t sensor_type, gpio_num_t pin, int16_t *humidity, int16 *temperature)
{
	bool bits[DHT_DATA_BITS];
	uint8_t data[DHT_DATA_BITS / 8] = {0};
	bool result;

	taskENTER_CRITICAL();
	result = dht_fetch_data(sensor_type, pin, bits);
	taskEXIT_CRITICAL();

	if(!result)
	{
		return ESP_FAIL;
	}

	for(uint8_t i = 0; i < DHT_DATA_BITS; i++)
	{
		data[i/8] <<= 1;
		data[i/8] |= bits[i];
	}
	if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
		debug("Checksum failed, invalid data received from sensor\n");
		return ESP_FAIL;
	}
	*humidity = dht_convert_data(sensor_type, data[0], data[1]);
	*temperature = dht_convert_data(sensor_type, data[2], data[3]);

	debug("Sensor data: humidity=%d, temp=%d\n", *humidity, *temperature);

	return ESP_OK;
}

esp_err_t dht_read_float_data(dht_sensor_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature)
{
    int16_t i_humidity, i_temp;

    if (dht_read_data(sensor_type, pin, &i_humidity, &i_temp) == ESP_OK) {
        *humidity = (float)i_humidity / 10;
        *temperature = (float)i_temp / 10;
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t dht_init(gpio_num_t pin, bool pull_up) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pin_bit_mask = 1ULL << pin,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = pull_up ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE
    };
    esp_err_t result = gpio_config(&io_conf);
    if (result == ESP_OK) {
        gpio_set_level(pin, 1);
        return ESP_OK;
    }
    return result;
}


void app_main(void)
{
	int i = 0;
	while (1) {
		printf("[%d] Hello world!\n", i);
		i++;
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}
