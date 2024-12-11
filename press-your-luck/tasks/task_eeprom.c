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
#include "task_eeprom.h"
#include "ece353_tasks.h"
#include "portmacro.h"

#if defined(HW05) || defined (HW06)
#include "main.h"
#include "press-your-luck/pyl.h"
#include "press-your-luck/tasks/task_console.h"

/* Event Groups */
extern EventGroupHandle_t eg_UI;

QueueHandle_t q_EEPROM;

/**
 * @brief 
 * Displays the number of whammies that have been selected
 * @param param 
 * Not Used
 */
void task_eeprom(void *param)
{
    /* Suppress warning for unused parameter */
    (void)param;

    /* Repeatedly running part of the task */
    for (;;)
    {
        eeprom_msg_t eeprom_msg;
        bool rslt;

        rslt = xQueueReceive(q_EEPROM, &eeprom_msg, portMAX_DELAY);

        if (rslt == pdPASS)
        {
            switch (eeprom_msg.operation)
            {
                case EEPROM_WRITE:
                    uint8_t byte0 = (uint8_t)(eeprom_msg.score & 0xFF);
                    uint8_t byte1 = eeprom_msg.score >> 8;
                    eeprom_write_byte(0x20, byte0);
                    eeprom_write_byte(0x21, byte1);
                    break;
                case EEPROM_READ:
                    uint16_t byte_0 = eeprom_read_byte(0x20);
                    uint16_t byte_1 = eeprom_read_byte(0x21);
                    eeprom_msg.score = (byte_1 << 8) + byte_0;
                    xQueueSend(eeprom_msg.return_queue, &eeprom_msg, portMAX_DELAY);
                    break;
                case EEPROM_RESET:
                    task_print_info("Resetting EEPROM");
                    eeprom_write_byte(0x20, 0);
                    eeprom_write_byte(0x21, 0);
                    break;
                default:
                    break;
            }
        }
    }
}

void task_eeprom_init(void)
{
    /* Enable the SPI interface */
    spi_init();

    /* Configure the IO pin used to control the chip select*/
    eeprom_cs_init();

    q_EEPROM = xQueueCreate(1, sizeof(eeprom_msg_t));

    xTaskCreate(
        task_eeprom,
        "Task EEPROM",
        configMINIMAL_STACK_SIZE,
        NULL,
        ECE353_TASK_PRIORITY_UI_INPUT,
        NULL);
}

#endif
