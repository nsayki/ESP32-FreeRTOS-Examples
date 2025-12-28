/**
 * @file main.c
 * @brief ESP32-C6 Joystick Driver with FreeRTOS & ADC OneShot
 *
 * This driver implements a thread-safe, producer-consumer model for reading 
 * analog joysticks. It supports both 8-way directional logic and advanced 
 * 360-degree trigonometric calculations with auto-calibration.
 *
 * @author Nurullah SAYKI
 * @contact nurullahsayki52@gmail.com
 * @date 2025-12-28
 * @version 1.0
 *
 * @copyright Copyright (c) 2025
 * */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h" //ESP-IDF v5.x new ADC library
#include "driver/gpio.h"


// --- CONFIGURATION ---
//.. ENABLE_360_LOGIC == 0 --> Classic 8 Direction
//.. ENABLE_360_LOGIC ==1 --> Professional 360 Degree
#define ENABLE_360_LOGIC     1

// CALIBRATION NOTE:
// Theoretical ADC radius is 2048 (4095/2). However, due to
// mechanical limitations and hardware offset, the joystick
// physically maxes out around ~3800 raw value.
//
// Since our calibrated center is ~2400:
// Effective Range = Max(3800) - Center(2400) = ~1400.
//
// We use 1400.0f instead of 2048.0f to ensure the calculated
// power can reach 100% at full stick deflection.
#define JOYSTICK_MAX_RADIUS  1400.0f

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
// Consumer Task (Hybrid: 8-Way & 360-Degree Support)
// -------------------------------------------------------------------------
void controller_task(void *pvParameters)
{
    //.. Clear Screen
    printf("\033[2J");
    joystick_data_t received_data;

    // --- CALIBRATION VARIABLES ---
    static bool is_calibrated = false;
    static int origin_x = 2048; // Default theoretical center
    static int origin_y = 2048;

    while (1)
    {
        //.. Wait for data from Queue
        if (xQueueReceive(xJoystickQueue, &received_data, portMAX_DELAY) == pdTRUE) 
        {
            
            //..AUTO-CALIBRATION (Runs only once at startup)
            if (!is_calibrated) 
            {
                origin_x = received_data.x_raw;
                origin_y = received_data.y_raw;
                is_calibrated = true;
                ESP_LOGI("JOYSTICK", "Calibrated Center -> X:%d Y:%d", origin_x, origin_y);
            }

            //.. Take cursor to the top
            printf("\033[H"); 
            printf("-----------------------------\n");
            #if ENABLE_360_LOGIC
            printf("  JOYSTICK DRIVER (360 Mode)\n");
            printf("-----------------------------\n");

            //.. Centering, using the CALIBRATED origin (Not 2048)
            float x_centered = (float)received_data.x_raw - (float)origin_x;
            float y_centered = (float)received_data.y_raw - (float)origin_y;

            //.. NOTE: If the Y axis is inverted (The value decreases when going up), multiply by -1
            y_centered = -y_centered; 

            //. Calculate the angle, the atan2 function returns a value between -Pi and +Pi
            float angle_rad = atan2(y_centered, x_centered);
            float angle_deg = angle_rad * (180.0f / M_PI);

            //.. Convert negative angles to positive angles (0-360 degrees)
            if (angle_deg < 0) angle_deg += 360.0f;

            //.. Convert the direction to counter-clockwise
            angle_deg = 360.0f - angle_deg;

            //.. If the angle is greater than 360 degrees, set it to 0
            if (angle_deg >= 360.0f) angle_deg = 0.0f;

            //.. Calculate the magnitude, Pisagor: a^2 + b^2 = c^2
            float magnitude = sqrt(x_centered*x_centered + y_centered*y_centered);
            
            //.. Map the power to 0-100%, max radius ~JOYSTICK_MAX_RADIUS
            int power_percent = (int)((magnitude / JOYSTICK_MAX_RADIUS) * 100.0f);
            //.. Prevent overflow
            if (power_percent > 100) power_percent = 100;
            
            //.. Deadzone filter, if the power is less than 10%, set it to 0
            if (power_percent < 10) 
            {
                power_percent = 0;
                angle_deg = 0.0f;
            }

            //.. Show on screen(CLI)
            printf("RAW X: %4d  |  RAW Y: %4d\n", received_data.x_raw, received_data.y_raw);
            printf("ANGLE  : %-6.1f° |  POWER: %%%-3d\n", angle_deg, power_percent);
            
            //.. Visual progress bar
            printf("POWER BAR: [");
            for(int i=0; i<20; i++) {
                if (i < (power_percent/5)) printf("#");
                else printf(".");
            }
            printf("]\n");

            #else

            printf("  JOYSTICK DRIVER (8-Way Mode)\n");
            printf("-----------------------------\n");

            //.. Logic is written here
            //.. The joystick approximately gives a value of 2048 on the center.
            //.. Let's give a tolerance (deadzone): 1500 to 2500.
            const char* direction_x = "";
            const char* direction_y = "";

            // The decision is based on the raw data of X
            if (received_data.x_raw > RIGHT_VALUE) direction_x = "RIGHT";
            else if (received_data.x_raw < LEFT_VALUE) direction_x = "LEFT";

            // The decision is based on the raw data of Y
            if (received_data.y_raw > UP_VALUE) direction_y = "UP";
            else if (received_data.y_raw < DOWN_VALUE) direction_y = "DOWN";

            //.. Combine Directions. We use a buffer to combine "UP" and "RIGHT" -> "UP RIGHT"
            char combined_direction[32]; 

            if (direction_x[0] == '\0' && direction_y[0] == '\0') 
            {
                // Both are empty -> CENTER
                snprintf(combined_direction, sizeof(combined_direction), "CENTER");
            } 
            else 
            {
                // Combine string: e.g., "UP    RIGHT" or just "DOWN"
                snprintf(combined_direction, sizeof(combined_direction), "%s %s", direction_y, direction_x);
            }

            printf("RAW X: %4d  |\n", received_data.x_raw);
            printf("RAW Y: %4d  |\n", received_data.y_raw);
            printf("STATUS: %-25s\n", combined_direction);
            #endif

            const char* button_state = "\033[1;32mRELEASED\033[0m";
            // Button State (Red/Green Effect)
            if (received_data.btn_pressed) 
            {
                button_state = "\033[1;31mPRESSED\033[0m";
            }
            printf("BUTTON: %-25s\n", button_state);
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
