#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h" //ESP-IDF v5.x new ADC library
#include "driver/gpio.h"


// --- CONFIGURATION ---
// ESP32-c6 --> GPIO 2 --> ADC1 Channel 2
// ESP32-c6 --> GPIO 3 --> ADC1 Channel 3
// ESP32-c6 --> GPIO 4 --> Joystick Switch
#define JOYSTICK_X_PIN      ADC_CHANNEL_3 
#define JOYSTICK_Y_PIN      ADC_CHANNEL_2 
#define JOYSTICK_SW_PIN     GPIO_NUM_4

#define RIGHT_VALUE         3000
#define LEFT_VALUE          1000
#define UP_VALUE            3000
#define DOWN_VALUE          1000
#define QUEUE_LENGTH        50

// --- DEBUGGING ---
static const char *TAG =    "JOYSTICK_APP";


// --- DATA STRUCTURES ---
typedef struct {
    int  x_raw;
    int  y_raw;
    bool btn_pressed;
} joystick_data_t;


//Global Queue Handle
QueueHandle_t xJoystickQueue;


// -------------------------------------------------------------------------
// Producer Task --- Hardware Abstraction Layer -- We take raw data from ADC1 with this task
// -------------------------------------------------------------------------
void adc_reader_task(void *pvParameters)
{
    //.. Initialize ADC
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1, // We are using ADC1 because C6 doesn't have ADC2
    };

    //.. Create ADC Unit
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    // Configure Channel, for X and Y
    adc_oneshot_chan_cfg_t config = { 
        .bitwidth = ADC_BITWIDTH_12, // Use 12-bit (0-4095)
        .atten = ADC_ATTEN_DB_12,    // 12dB (to read between 0-3.3V)
    };

    //.. Set GPIO 2 and GPIO 3 for ADC1 channels
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOYSTICK_X_PIN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOYSTICK_Y_PIN, &config));

    //.. Configure GPIO
    //.. We are using GPIO 4 in INPUT_PULLUP mode because when it is pressed, it goes to GND
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << JOYSTICK_SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,    // PULLUP enable
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE      // Disable interrupt, we use polling method
    };
    gpio_config(&io_conf);

    joystick_data_t data_packet;

    while (1)
    {
        //.. Read RAW Data from ADC1.
        adc_oneshot_read(adc1_handle, JOYSTICK_X_PIN, &data_packet.x_raw);
        adc_oneshot_read(adc1_handle, JOYSTICK_Y_PIN, &data_packet.y_raw);

        //.. If the switch is pressed, it will be 0(Active Low) and we need to invert it
        data_packet.btn_pressed = !gpio_get_level(JOYSTICK_SW_PIN); 

        //.. Send data to Queue, if Queue is full, don't wait (0)
        if (xQueueSend(xJoystickQueue, &data_packet, 0) != pdTRUE) 
        {
            ESP_LOGI(TAG, "Queue is full! Data lost.");
        }

        //.. Sampling Rate -- 100ms is equal to 10Hz
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    //.. If the task is finished, delete it
    adc_oneshot_del_unit(adc1_handle);
    vTaskDelete(NULL);
}

// -------------------------------------------------------------------------
// Consumer Task
// -------------------------------------------------------------------------
void controller_task(void *pvParameters)
{
    //.. Clear Screen
    printf("\033[2J");
    joystick_data_t received_data;

    while (1)
    {
        //.. Wait for data from Queue
        if (xQueueReceive(xJoystickQueue, &received_data, portMAX_DELAY) == pdTRUE) {
            
            //.. Logic is written here
            //.. The joystick approximately gives a value of 2048 on the center.
            //.. Let's give a tolerance (deadzone): 1500 to 2500.
            const char* direction_x = "MIDDLE";
            const char* direction_y = "MIDDLE";
            const char* button_state = "RELEASED";

            // The decision is based on the raw data of X
            if (received_data.x_raw > RIGHT_VALUE) direction_x = "RIGHT";
            else if (received_data.x_raw < LEFT_VALUE) direction_x = "LEFT";

            // The decision is based on the raw data of Y
            if (received_data.y_raw > UP_VALUE) direction_y = "UP";
            else if (received_data.y_raw < DOWN_VALUE) direction_y = "DOWN";

            // Button State (Red/Green Effect)
            if (received_data.btn_pressed) {
                button_state = "\033[1;31mPRESSED\033[0m";
            }

            //.. Write to Terminal
            //.. Go Up in Terminal with \033[H
            printf("\033[H"); 
            printf("-----------------------------\n");
            printf("🕹️  JOYSTICK STATUS (FreeRTOS)\n");
            printf("-----------------------------\n");
            printf("RAW X: %4d  |  DIRECTION: %-12s\n", received_data.x_raw, direction_x);
            printf("RAW Y: %4d  |  DIRECTION: %-12s\n", received_data.y_raw, direction_y);
            printf("BUTTON   : %-20s\n", button_state);
            printf("-----------------------------\n");
        }
    }

}


void app_main(void)
{

    //.. Create Queue
    xJoystickQueue = xQueueCreate(QUEUE_LENGTH, sizeof(joystick_data_t));

    if (NULL == xJoystickQueue) 
    {
        ESP_LOGE(TAG, "Queue creation failed!");
        return;
    }

    //.. Create Tasks
    BaseType_t adc_read = xTaskCreate(adc_reader_task, "ADC_Reader", 2048, NULL, 5, NULL);
    if (pdFAIL == adc_read )
    {
        ESP_LOGE(TAG, "ADC Reader Task creation failed!");
    }
    BaseType_t controller = xTaskCreate(controller_task, "Controller", 2048, NULL, 5, NULL);
    if (pdFAIL == controller)
    {
        ESP_LOGE(TAG, "Controller Task creation failed!");
    }

    ESP_LOGI(TAG, "System Initializing...");
}
