#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "GPS_SYSTEM";

// GPS Data Structure
typedef struct {
    float latitude;
    float longitude;
    uint8_t satellite_cnt;
} gps_data_t;

#define QUEUE_LENGTH    10
#define ITEM_SIZE       sizeof(gps_data_t)


QueueHandle_t gps_queue;

void gps_producer_task(void *pvParameters)
{
    gps_data_t my_gps_data;
    my_gps_data.latitude = 41.0123;
    my_gps_data.longitude = 28.9876;
    my_gps_data.satellite_cnt = 4;
    float increment_val = 0.0005;

    for (;;)
    {
        my_gps_data.latitude += increment_val;
        my_gps_data.longitude += increment_val;

        if (xQueueSend(gps_queue, &my_gps_data, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "Sent Data -> Lat: %.4f, Lon: %.4f", my_gps_data.latitude, my_gps_data.longitude);
        }
        else
        {
            ESP_LOGE(TAG, "Queue is full! Data lost.");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void display_consumer_task(void *pvParameters)
{
    gps_data_t received_data;

    for (;;)
    {
        if (xQueueReceive(gps_queue, &received_data, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "Received -> Lat: %.4f, Lon: %.4f, Satellites: %d", 
                     received_data.latitude, 
                     received_data.longitude, 
                     received_data.satellite_cnt);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "System Initializing...");

    gps_queue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);
    if (gps_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create Queue!");
        return;
    }

    xTaskCreate(gps_producer_task, "GPS_Producer", 2048, NULL, 5, NULL);
    xTaskCreate(display_consumer_task, "Display_Consumer", 2048, NULL, 5, NULL);
}