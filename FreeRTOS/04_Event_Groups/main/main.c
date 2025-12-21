#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

static const char *TAG = "ROCKET_LAUNCH";

/* Event Group Handle */
EventGroupHandle_t rocket_event_group;

/* Bit Definitions (Flags) */
#define BIT_FUEL     (1 << 0) // 001 (Decimal 1)
#define BIT_WEATHER  (1 << 1) // 010 (Decimal 2)
#define BIT_SYSTEMS  (1 << 2) // 100 (Decimal 4)

/* Target: All bits must be 1 (OR Logic to combine them) */
#define ALL_SYSTEMS_GO (BIT_FUEL | BIT_WEATHER | BIT_SYSTEMS)

void launch_control_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[CONTROL_CENTER]: Waiting for ALL systems to be READY...");

    /* CRITICAL POINT: xEventGroupWaitBits
     * 3rd Param (pdTRUE): Clear bits on exit (Reset flags after launch)
     * 4th Param (pdTRUE): Wait for ALL bits (AND Logic). 
     */
    EventBits_t bits = xEventGroupWaitBits(
                            rocket_event_group,  // The Event Group
                            ALL_SYSTEMS_GO,      // Bits to wait for
                            pdTRUE,              // Clear bits after return? -> YES
                            pdTRUE,              // Wait for ALL bits? -> YES
                            portMAX_DELAY        // Wait indefinitely
                        );

    if ((bits & ALL_SYSTEMS_GO) == ALL_SYSTEMS_GO)
    {
        ESP_LOGI(TAG, "****************************************");
        ESP_LOGI(TAG, "🚀 3... 2... 1... LIFTOFF! ROCKET LAUNCHED! 🚀");
        ESP_LOGI(TAG, "****************************************");
    }
    
    vTaskDelete(NULL);
}

void fuel_check_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[FUEL_TEAM]: Refueling in progress...");
    vTaskDelay(pdMS_TO_TICKS(2000)); // Takes 2 seconds
    
    ESP_LOGI(TAG, "[FUEL_TEAM]: Tank Full. READY. (Setting Bit 0)");
    xEventGroupSetBits(rocket_event_group, BIT_FUEL);
    vTaskDelete(NULL);
}

void weather_check_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[WEATHER_TEAM]: Checking wind speed...");
    vTaskDelay(pdMS_TO_TICKS(4000)); // Takes 4 seconds
    
    ESP_LOGI(TAG, "[WEATHER_TEAM]: Sky is clear. READY. (Setting Bit 1)");
    xEventGroupSetBits(rocket_event_group, BIT_WEATHER);
    vTaskDelete(NULL);
}

void system_check_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[SYSTEM_ENG]: Running diagnostics...");
    vTaskDelay(pdMS_TO_TICKS(6000)); // Takes 6 seconds
    
    ESP_LOGI(TAG, "[SYSTEM_ENG]: All circuits GREEN. READY. (Setting Bit 2)");
    xEventGroupSetBits(rocket_event_group, BIT_SYSTEMS);
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "--- MISSION START ---");
    rocket_event_group = xEventGroupCreate();

    xTaskCreate(launch_control_task, "Launch_Control", 4096, NULL, 5, NULL);
    xTaskCreate(fuel_check_task,     "Fuel_Team",      2048, NULL, 5, NULL);
    xTaskCreate(weather_check_task,  "Weather_Team",   2048, NULL, 5, NULL);
    xTaskCreate(system_check_task,   "Systems_Eng",    2048, NULL, 5, NULL);
}