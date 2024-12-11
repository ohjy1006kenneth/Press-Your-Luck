/**
 * @file task_lcd_game_info.c
 * @author Kenneth Oh (oh87@wisc.edu)
 * @brief
 * @version 0.1
 * @date 2024-07-31
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "task_lcd_game_info.h"
// #include "FreeRTOSConfig.h"

#if defined(HW05) || defined(HW06)

#include "FreeRTOSConfig.h"
#include "main.h"
#include "press-your-luck/pyl.h"
#include "press-your-luck/tasks/task_console.h"

/* Event Groups */
extern EventGroupHandle_t eg_UI;

/* Semaphores */
extern SemaphoreHandle_t s_LCD;

QueueHandle_t q_LCD_Info;

/******************************************************************************
 * FreeRTOS Related Functions
 ******************************************************************************/

/**
 * @brief
 * @param param
 */
void task_lcd_game_info(void* param)
{
    /* Suppress warning for unused parameter */
    (void)param;

    /* Repeatedly running part of the task */
    for (;;) {
        /* ADD CODE */
        game_info_msg_t game_info_data;

        // Wait for message from the queue
        if (xQueueReceive(q_LCD_Info, &game_info_data, portMAX_DELAY) == pdPASS) {

            xSemaphoreTake(s_LCD, portMAX_DELAY);

            if (game_info_data.cmd == CMD_UPDATE_LOCAL_DATA) {
                pyl_print_digits_6(game_info_data.score, LOCATION_LOCAL_SCORE_X, LOCATION_LOCAL_SCORE_Y, COLOR_LOCAL_SCORE, 0x00);
                pyl_print_digits_2(game_info_data.spins, LOCATION_LOCAL_SPINS_X, LOCATION_LOCAL_SPINS_Y, COLOR_LOCAL_SPINS, 0x00);
                pyl_print_digits_2(game_info_data.passed, LOCATION_LOCAL_PASSED_X, LOCATION_LOCAL_PASSED_Y, COLOR_LOCAL_PASSED, 0x00);

                lcd_draw_image(IMAGE_COORD_X_GAME_OVER, IMAGE_COORD_Y_GAME_OVER, IMG_BITMAP_WIDTH_GAME_OVER, IMG_BITMAP_HEIGHT_GAME_OVER, IMG_BITMAP_GAME_OVER, LCD_COLOR_BLACK, LCD_COLOR_BLACK);
            } else if (game_info_data.cmd == CMD_UPDATE_REMOTE_DATA) {
                pyl_print_digits_6(game_info_data.score, LOCATION_REMOTE_SCORE_X, LOCATION_REMOTE_SCORE_Y, COLOR_REMOTE_SCORE, 0x00);
                pyl_print_digits_2(game_info_data.spins, LOCATION_REMOTE_SPINS_X, LOCATION_REMOTE_SPINS_Y, COLOR_REMOTE_SPINS, 0x00);
                pyl_print_digits_2(game_info_data.passed, LOCATION_REMOTE_PASSED_X, LOCATION_REMOTE_PASSED_Y, COLOR_REMOTE_PASSED, 0x00);
            } else if (game_info_data.cmd == CMD_GAME_OVER) {
                lcd_draw_image(IMAGE_COORD_X_GAME_OVER, IMAGE_COORD_Y_GAME_OVER, IMG_BITMAP_WIDTH_GAME_OVER, IMG_BITMAP_HEIGHT_GAME_OVER, IMG_BITMAP_GAME_OVER, LCD_COLOR_RED, LCD_COLOR_BLACK);
            }

            xSemaphoreGive(s_LCD);
        }
    }
}

void task_lcd_game_info_init(void)
{
    q_LCD_Info = xQueueCreate(1, sizeof(game_info_msg_t));

    xTaskCreate(
        task_lcd_game_info,
        "Task LCD Game Info",
        5 * configMINIMAL_STACK_SIZE,
        NULL,
        ECE353_TASK_PRIORITY_UI_OUTPUT,
        NULL);
}

#endif