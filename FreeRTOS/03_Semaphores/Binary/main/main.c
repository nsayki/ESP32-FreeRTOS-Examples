#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "WORKFLOW";
SemaphoreHandle_t work_signal_sem;

void manager_task(void *pvParameters)
{
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000)); 
        ESP_LOGI(TAG, "[MANAGER]: New order received! Signaling worker...");
        xSemaphoreGive(work_signal_sem);
    }
}

void employee_task(void *pvParameters)
{
    for (;;) {
        if(xSemaphoreTake(work_signal_sem, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "[WORKER]: Yes Boss! Working on it...");
            vTaskDelay(pdMS_TO_TICKS(500));
            ESP_LOGI(TAG, "[WORKER]: Job done.");
        }
    }
}

void app_main(void)
{
    work_signal_sem = xSemaphoreCreateBinary();
    xTaskCreate(manager_task, "Manager", 2048, NULL, 5, NULL);
    xTaskCreate(employee_task, "Employee", 2048, NULL, 5, NULL);
}