#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"



// Log tag for debug output
static const char *TAG = "DEBUG";

// BOOT button on ESP32-C6 DevKit (GPIO 9)
#define BOOT_BUTTON_PIN GPIO_NUM_9

// Global Semaphore Handle (Bridge between ISR and Task)
SemaphoreHandle_t xBinarySemaphore = NULL;

// -------------------------------------------------------------------------
// ISR (Interrupt Service Routine) Function
// NOTE: This must be placed in IRAM_ATTR to execute very fast.
// Never use printf or vTaskDelay inside an ISR!
// -------------------------------------------------------------------------
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    // Variable to check if a context switch is needed
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Give the "Wake Up" signal to the Task.
    // CAUTION: We must use the "FromISR" version here!
    xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);

    // If the semaphore woke up a higher priority task, 
    // force a context switch immediately upon exiting the ISR.
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// -------------------------------------------------------------------------
// Button Handler Task (The Worker)
// -------------------------------------------------------------------------
void button_handler_task(void *pvParameters)
{
    while(1)
    {
        // Wait for the Semaphore indefinitely (portMAX_DELAY)
        // The CPU sleeps here until the semaphore is given by the ISR.
        if(xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE)
        {
            // If we are here, the button was pressed and ISR signaled us.
            ESP_LOGI("----------------------------------------\n");
            ESP_LOGI("🔥 INTERRUPT DETECTED! Button Pressed.\n");
            ESP_LOGI("   Heavy processing can be done here...\n");
            ESP_LOGI("----------------------------------------\n");

            // Simple software debounce delay
            // In real projects, hardware debounce is preferred.
            vTaskDelay(pdMS_TO_TICKS(50)); 
        }
    }
}

// -------------------------------------------------------------------------
// Main Application
// -------------------------------------------------------------------------
void app_main(void)
{
    // 1. Create Binary Semaphore
    xBinarySemaphore = xSemaphoreCreateBinary();

    if(xBinarySemaphore == NULL) {
        ESP_LOGI("Failed to create semaphore!\n");
        return;
    }

    // 2. Button Configuration (GPIO Settings)
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE; // Trigger on Falling Edge (Press)
    io_conf.pin_bit_mask = (1ULL << BOOT_BUTTON_PIN);
    io_conf.mode = GPIO_MODE_INPUT;        // Input mode
    io_conf.pull_up_en = 1;                // Enable internal pull-up
    gpio_config(&io_conf);

    // 3. Create the Handler Task
    // We give it high priority (10) to respond immediately.
    xTaskCreate(button_handler_task, "Button_Task", 2048, NULL, 10, NULL);

    // 4. Install ISR Service and Add Handler
    // Must install the service before adding any specific handlers
    gpio_install_isr_service(0);
    
    // Attach our 'gpio_isr_handler' function to the specific pin
    gpio_isr_handler_add(BOOT_BUTTON_PIN, gpio_isr_handler, NULL);

    ESP_LOGI("System Ready! Waiting for BOOT button press...\n");
}