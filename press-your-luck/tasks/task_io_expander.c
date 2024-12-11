/**
 * @file task_io_expander.c
 * @author Kenneth Oh (oh87@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2024-08-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "task_io_expander.h"
#include "ece353_tasks.h"

#if defined(HW05) || defined (HW06)
#include "main.h"
#include "press-your-luck/pyl.h"
#include "press-your-luck/tasks/task_console.h"

/* Event Groups */
extern EventGroupHandle_t eg_UI;

QueueHandle_t q_IO_Exp;

/**
 * @brief 
 * Displays the number of whammies that have been selected
 * @param param 
 * Not Used
 */
void task_io_expander(void *param)
{
    /* Suppress warning for unused parameter */
    (void)param;

    io_expander_set_configuration(0x80); // Set bit 7 as input, bits 6-0 as outputs
    io_expander_set_output_port(0x00);   // Turn OFF all LEDs

    /* Repeatedly running part of the task */
    for (;;)
    {
        uint32_t num_whammies = 0;
        BaseType_t status;

        status = xQueueReceive(q_IO_Exp, &num_whammies, portMAX_DELAY);

        if (status == pdPASS)
        {
            switch (num_whammies)
            {
                case 0:
                    // Turn OFF all LEDs
                    io_expander_set_output_port(0x00);
                    break;
                case 1:
                    // Turn ON D601
                    io_expander_set_output_port(0x08);
                    break;
                case 2:
                    // Turn ON D601, D600
                    io_expander_set_output_port(0x0C);
                    break;
                case 3:
                    // Turn ON D601, D600, D606
                    io_expander_set_output_port(0x0E);
                    break;
                case 4:
                    // Turn ON D601, D600, D606, D605
                    io_expander_set_output_port(0x0F);
                    break;
                default:
                    // Inavlid number of whammies
                    task_print_error("Invalid score");
                    break;
            }
        }
    }
}

void task_io_expander_init(void)
{
    i2c_init();

    q_IO_Exp = xQueueCreate(1, sizeof(uint8_t));

    xTaskCreate(
        task_io_expander,
        "Task IO Exp",
        configMINIMAL_STACK_SIZE,
        NULL,
        ECE353_TASK_PRIORITY_UI_INPUT,
        NULL);
}

#endif
