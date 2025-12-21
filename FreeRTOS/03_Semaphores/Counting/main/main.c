#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "PARKING_LOT";
SemaphoreHandle_t parking_sem;
#define MAX_SPOTS 3

void car_task(void *pvParameters)
{
    char *car_name = (char *)pvParameters;
    ESP_LOGI(TAG, "[%s]: Arrived at gate.", car_name);

    if(xSemaphoreTake(parking_sem, portMAX_DELAY) == pdTRUE) {
        int free_spots = uxSemaphoreGetCount(parking_sem);
        ESP_LOGI(TAG, "[%s]: ---> ENTERED! (Free Spots: %d)", car_name, free_spots);
        vTaskDelay(pdMS_TO_TICKS(3000)); 
        ESP_LOGI(TAG, "[%s]: <--- LEAVING...", car_name);
        xSemaphoreGive(parking_sem);
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Opening Parking Lot...");
    parking_sem = xSemaphoreCreateCounting(MAX_SPOTS, MAX_SPOTS);
    if(parking_sem != NULL) {
        xTaskCreate(car_task, "Car1", 2048, (void*)"Car_1", 5, NULL);
        xTaskCreate(car_task, "Car2", 2048, (void*)"Car_2", 5, NULL);
        xTaskCreate(car_task, "Car3", 2048, (void*)"Car_3", 5, NULL);
        xTaskCreate(car_task, "Car4", 2048, (void*)"Car_4", 5, NULL);
        xTaskCreate(car_task, "Car5", 2048, (void*)"Car_5", 5, NULL);
    }
}