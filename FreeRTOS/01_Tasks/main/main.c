#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Log tag for debug output
static const char *TAG = "MACHINE_SYSTEM";

// Machine Names (Passed as parameters to tasks)
static const char *press_machine_name    = "PRESS_MACHINE";
static const char *welding_machine_name  = "WELDING_MACHINE";
static const char *painting_machine_name = "PAINTING_MACHINE";

/**
 * @brief Generic task function to simulate machine operation.
 * * @param pvParameters Pointer to the machine name string.
 */
void machine_task(void *pvParameters)
{
    // Cast the void pointer back to a char pointer to retrieve the name
    const char *machine_name = (const char *)pvParameters;

    for (;;)
    {
        // Log the machine status
        ESP_LOGI(TAG, "%s is currently operating...", machine_name);
        
        // Block the task for 1 second (1000ms)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "System Initializing...");
    
    // 1. Create Press Machine Task
    BaseType_t ret_press = xTaskCreate(machine_task, "Task_Press", 2048, (void*)press_machine_name, 5, NULL);
    if (ret_press == pdPASS)
    {
        ESP_LOGI(TAG, "Press Machine Task started successfully.");
    }

    // 2. Create Welding Machine Task
    BaseType_t ret_weld = xTaskCreate(machine_task, "Task_Welding", 2048, (void*)welding_machine_name, 5, NULL);
    if (ret_weld == pdPASS)
    {
        ESP_LOGI(TAG, "Welding Machine Task started successfully.");
    }

    // 3. Create Painting Machine Task
    BaseType_t ret_paint = xTaskCreate(machine_task, "Task_Painting", 2048, (void*)painting_machine_name, 5, NULL);
    if (ret_paint == pdPASS)
    {
        ESP_LOGI(TAG, "Painting Machine Task started successfully.");
    }

    ESP_LOGI(TAG, "All systems go!");
}