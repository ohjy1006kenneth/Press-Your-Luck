/**
 * @file task_sw.c
 * @author Kenneth Oh (oh87@wisc.edu)
 * @brief
 * @version 0.1
 * @date 2024-08-23
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "task_sw.h"

#if defined(HW05) || defined(HW06)
#include "main.h"
#include "press-your-luck/pyl.h"
#include "press-your-luck/tasks/task_console.h"

/* Event Groups */
extern EventGroupHandle_t eg_UI;

/**
 * @brief
 * Function used to detect when SW1, SW2, and SW3 are been pressed.
 * The task should detect the initial button press by debouncing
 * the push button for 50mS.  Pressing the button for longer than 50mS
 * should only result in a single button event.
 *
 * @param param
 *  Unused
 */
void task_switches(void* param)
{
    bool sw1_button_prev = false;
    bool sw1_button_curr = false;
    bool sw2_button_prev = false;
    bool sw2_button_curr = false;
    bool sw3_button_prev = false;
    bool sw3_button_curr = false;

    while (1) {
        // Sleep for 50mS
        vTaskDelay(50);

        /* SW1 */
        // Read the current state of the SW1
        if ((REG_PUSH_BUTTON_IN & SW1_MASK) == 0) {
            sw1_button_curr = true; // SW1 pressed
        } else {
            sw1_button_curr = false; // SW1 released
        }

        // See if SW1 has been pressed
        if (sw1_button_curr && !sw1_button_prev) {
            xEventGroupSetBits(eg_UI, EVENT_UI_SW1);
        }

        /* SW2 */
        // Read the current state of the SW2
        if ((REG_PUSH_BUTTON_IN & SW2_MASK) == 0) {
            sw2_button_curr = true; // SW2 pressed
        } else {
            sw2_button_curr = false; // SW2 released
        }

        // See if SW2 has been pressed
        if (sw2_button_curr && !sw2_button_prev) {
            xEventGroupSetBits(eg_UI, EVENT_UI_SW2);
        }

        /* SW3 */
        // Read the current state of the SW3
        if ((REG_PUSH_BUTTON_IN & SW3_MASK) == 0) {
            sw3_button_curr = true; // SW3 pressed
        } else {
            sw3_button_curr = false; // SW3 released
        }

        // See if SW3 has been pressed
        if (sw3_button_curr && !sw3_button_prev) {
            xEventGroupSetBits(eg_UI, EVENT_UI_SW3);
        }

        // Update the previous state of the SW1, SW2, and SW3
        sw1_button_prev = sw1_button_curr;
        sw2_button_prev = sw2_button_curr;
        sw3_button_prev = sw3_button_curr;
    }
}

/**
 * @brief
 * Initializes User Push Buttons and creates the corresponding FreeRTOS tasks for SW1 and SW2
 */
void task_sw_init(void)
{
    push_buttons_init(false);

    // Register the Task with FreeRTOS
    xTaskCreate(
        task_switches,
        "Task Switches",
        configMINIMAL_STACK_SIZE,
        NULL,
        ECE353_TASK_PRIORITY_UI_INPUT,
        NULL);
}

#endif
