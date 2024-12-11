/**
 * @file io-sw.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "io-sw.h"
#include "cyhal_hw_types.h"
#include "cyhal_system.h"
#include "cyhal_timer.h"
#include "ece353_events.h"
#include "timer.h"
#include <stdint.h>

/*****************************************************************************/
/* Public Global Variables */
/*****************************************************************************/

/*****************************************************************************/
/* Static Function Declarations */
/*****************************************************************************/
static void Handler_Timer_Buttons(void *handler_arg, cyhal_timer_event_t event);
static void push_buttons_timer_init(void);

/*****************************************************************************/
/* ICE 02 START */
/*****************************************************************************/
void push_buttons_init(bool enable_timer_irq)
{
    /* ADD CODE */
    /* Initialize the IO Pins connected to the push buttons as inputs */
    cy_rslt_t rslt;
    rslt = cyhal_gpio_init(PIN_SW1, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    rslt = cyhal_gpio_init(PIN_SW2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    rslt = cyhal_gpio_init(PIN_SW3, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);

    /* ADD CODE */
    if(enable_timer_irq)
    {
        push_buttons_timer_init();
    }
} 
/*****************************************************************************/
/* ICE 02 END */
/*****************************************************************************/

/*****************************************************************************/
/* ICE 06 START */
/*****************************************************************************/

/*
 * Handler used to debounce the push buttons
*/
static void Handler_Timer_Buttons(void *handler_arg, cyhal_timer_event_t event)
{
    /* Static Vars */
    static uint8_t debounce_counter_SW1 = 0;
    static uint8_t debounce_counter_SW2 = 0;
    static uint8_t debounce_counter_SW3 = 0;

    uint32_t reg_val = PORT_BUTTONS->IN;

    /* Many mechanical buttons will bounce when they are pressed.
    *  The signal may oscillate between 0 and 1 for several milliseconds.
    *
    * We will debounce a button by sampling the button and detecting the first
    * 50mS interval of time when the button is low.  When that period of time is
    * detected, we will alert the main application using ECE353_Events.
    */
    /* Debounce logic for SW1 */
    if ((reg_val & SW1_MASK) == 0) {
        if (debounce_counter_SW1 < 5) {
            debounce_counter_SW1++;
        }
    } else {
        debounce_counter_SW1 = 0;
    }

    /* Debounce logic for SW2 */
    if ((reg_val & SW2_MASK) == 0) {
        if (debounce_counter_SW2 < 5) {
            debounce_counter_SW2++;
        }
    } else {
        debounce_counter_SW2 = 0;
    }

    /* Debounce logic for SW3 */
    if ((reg_val & SW3_MASK) == 0) {
        if (debounce_counter_SW3 < 5) {
            debounce_counter_SW3++;
        }
    } else {
        debounce_counter_SW3 = 0;
    }

    // check that 50ms has passed per button
    if (debounce_counter_SW1 == 4) {
        ECE353_Events.sw1 = 1;
    }
    if (debounce_counter_SW2 == 4) {
        ECE353_Events.sw2 = 1;
    }
    if (debounce_counter_SW3 == 4) {
        ECE353_Events.sw3 = 1;
    }
}

/* ADD CODE */
/* Allocate static variables for a cyhal_timer_t and cyhal_timer_cfg_t that will be used
 * to configure the button timer*/
static cyhal_timer_t timer;
static cyhal_timer_cfg_t timer_cfg;

static void push_buttons_timer_init(void)
{
    /*ADD CODE*/
    /*
     * Use timer_init() to configure a timer that generates an interrupt every 10mS
     * This timer will be used to debounce the push buttons  on the ECE353 Dev Board
     */
    timer_init(&timer, &timer_cfg, TIMER_TICKS_MS_100/2, Handler_Timer_Buttons);
    cyhal_timer_start(&timer);
}
/*****************************************************************************/
/* ICE 06 END */
/*****************************************************************************/