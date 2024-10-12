#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/hw_timer.h"
#include "driver/uart.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOSConfig.h"
#include "esp_log.h"
#include "esp_system.h"



#define BUF_SIZE        1024
#define BUF_SIZE_2      2048
#define ZERO            0
#define MUTEX_WAIT_TIME 100
#define Second_Delay    1000
#define Delay_500MS     500
#define Logic_High      1
#define Logic_Low       0
#define configUSE_TIME_SLICING 1
/* Global mutex handle */
SemaphoreHandle_t g_Mutex;



static const char *TAG = "IdleHook";

// Idle hook function
void vApplicationIdleHook(void) {
    // This function will be called when the system is idle
    ESP_LOGI(TAG, "Idle hook is running...");
    // Perform low-power tasks, cleanup, etc.
    esp_light_sleep_start();

}


/* Function to configure the UART */
void ConfigureSerial(void)
{
    uint8_t p_data = (uint8_t *) malloc(BUF_SIZE); / Allocate buffer for UART data */
    
    uart_config_t uart_settings={

    .baud_rate = 74880,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_0, &uart_settings); /* Configure UART with specified settings */
    uart_driver_install(UART_NUM_0, BUF_SIZE_2, ZERO, ZERO, NULL, ZERO);
}

/* Function to configure GPIO */
void ConfigureGPIO(void)
{
    gpio_config_t gpio_settings={

    .mode         = GPIO_MODE_DEF_OUTPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE,
    .pin_bit_mask = (1ULL << GPIO_NUM_2) /* Configure GPIO2 */
    };
    gpio_config(&gpio_settings); /* Apply GPIO configuration */
}

/* Task 1: Turn LED on */
void Task1(void *pv_parameters)
{
    while (1)
    {   

        if (xSemaphoreTake(g_Mutex, (TickType_t) MUTEX_WAIT_TIME) == pdTRUE)
        {
            /* Set GPIO2 high to turn LED on */
            gpio_set_level(GPIO_NUM_2, Logic_High);
            xSemaphoreGive(g_Mutex);

            vTaskDelay(Delay_500MS / portTICK_PERIOD_MS); /* Delay for 500ms */
            taskYIELD(); /* Yield to other tasks */
        }
        else
        {
            printf("No MUTEX Received\n"); /* Print message if mutex is not available */
        }
    }

    /* Task should delete itself if exiting */
    vTaskDelete(NULL);
}

/* Task 2: Turn LED off */
void Task2(void *pv_parameters)
{  

    while (1)
    {
        if (xSemaphoreTake(g_Mutex, (TickType_t) MUTEX_WAIT_TIME) == pdTRUE)
        {
            /* Set GPIO2 low to turn LED off */
            gpio_set_level(GPIO_NUM_2, Logic_Low);

            xSemaphoreGive(g_Mutex);

            vTaskDelay(Second_Delay / portTICK_PERIOD_MS); /* Delay for 1 second */
        }
        else
        {
            printf("No Mutex Received\n");
        }
    }

    /* Task should delete itself if exiting */
    vTaskDelete(NULL);
}

/* Task 3: Print a status message */
void Task3(void *pv_parameters)
{

    while (1)
    {
        /* Print status message via UART */
        printf("Task 3: Status message\n");

        vTaskDelay(Second_Delay / portTICK_PERIOD_MS); /* Delay for 1 second */
    }

    /* Task should delete itself if exiting */
    vTaskDelete(NULL);
}

/* Main application entry point */
void app_main(void)
{
    ConfigureGPIO(); /* Configure GPIO */
    ConfigureSerial(); /* Configure UART */
    
    const int Priority_task_1=1; //Task one priority
    const int Priority_task_2=2; //Task two priority
    const int Priority_task_3=3; //Task two priority


    g_Mutex = xSemaphoreCreateMutex(); /* Create a mutex */

    if (g_Mutex != NULL)
    {
        /* Create tasks with different priorities */
        xTaskCreate(Task1, "Task 1", BUF_SIZE_2, NULL, 1, NULL);
        xTaskCreate(Task2, "Task 2", BUF_SIZE_2, NULL, 2, NULL);
        xTaskCreate(Task3, "Task 3", BUF_SIZE_2, NULL, 3, NULL);
    }
}