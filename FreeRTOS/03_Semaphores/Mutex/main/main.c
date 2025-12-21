#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "PRINTER_SYSTEM";
SemaphoreHandle_t mutex_printer;

void printer_write(const char *message)
{
    if (xSemaphoreTake(mutex_printer, portMAX_DELAY) == pdTRUE)
    {
        printf("[%s] Printing: ", TAG);
        for(int i = 0; i < strlen(message); i++)
        {
            printf("%c", message[i]);
            fflush(stdout); 
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }
        printf("\n");
        xSemaphoreGive(mutex_printer);
    }
}

void task_doc_a(void *pvParameters)
{
    for (;;) { printer_write("DOC_AAAAA"); vTaskDelay(pdMS_TO_TICKS(1000)); }
}

void task_doc_b(void *pvParameters)
{
    for (;;) { printer_write("DOC_BBBBB"); vTaskDelay(pdMS_TO_TICKS(1000)); }
}

void app_main(void)
{
    mutex_printer = xSemaphoreCreateMutex();
    xTaskCreate(task_doc_a, "TaskA", 2048, NULL, 5, NULL);
    xTaskCreate(task_doc_b, "TaskB", 2048, NULL, 5, NULL);
}