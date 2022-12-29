#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_err.h>
#include <dht/dht.h>

#define DHT_GPIO 5 // D1 pin

void temperature_task(void *arg)
{
    ESP_ERROR_CHECK(dht_init(DHT_GPIO, false));
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    while (1)
    {
        int16_t humidity = 0;
        int16_t temperature = 0;
        float humidity1, temperature1;
        if (dht_read_data(DHT_TYPE_DHT22, DHT_GPIO, &humidity, &temperature) == ESP_OK) {
            // e.g. in dht22, 604 = 60.4%, 252 = 25.2 C
            // If you want to print float data, you should run `make menuconfig`
            // to enable full newlib and call dht_read_float_data() here instead
        	humidity1 = (float) (humidity / 100);
        	temperature1 = (float) (temperature / 100);
            printf("Humidity: %d\ Temperature: %d\n", humidity, temperature);
        } else 
		{
            printf("Fail to get dht temperature data\n");
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void app_main() {
    xTaskCreate(temperature_task, "temperature task", 2048, NULL, tskIDLE_PRIORITY, NULL);
}
