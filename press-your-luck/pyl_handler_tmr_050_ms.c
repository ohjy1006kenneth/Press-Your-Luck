/**
 * @file pyl_handler_tmr_050_ms.c
 * @brief Timer interrupt handler that runs every 50ms to detect button presses and joystick movement.
 */

#include "pyl.h"
#include "pyl_handler_tmr_050_ms.h"
#include "ece353_events.h"
#include "io-sw.h"
#include "joystick.h"

#if defined(HW02) || defined(HW03) || defined(HW04)

/* Timer handler function that runs every 50ms */
void handler_timer_050_ms(void *callback_arg, cyhal_timer_event_t event)
{
    static button_state_t sw1_prev_state = BUTTON_RELEASED;
    static button_state_t sw2_prev_state = BUTTON_RELEASED;
    static button_state_t sw3_prev_state = BUTTON_RELEASED;
    static joystick_position_t prev_position = JOYSTICK_POS_CENTER;

    // Check the state of SW1
    button_state_t sw1_curr_state = (REG_PUSH_BUTTON_IN & SW1_MASK) == 0 ? BUTTON_PRESSED : BUTTON_RELEASED;
    if (sw1_curr_state == BUTTON_RELEASED && sw1_prev_state == BUTTON_PRESSED)
    {
        ECE353_Events.sw1 = 1;
    }
    sw1_prev_state = sw1_curr_state;

    // Check the state of SW2
    button_state_t sw2_curr_state = (REG_PUSH_BUTTON_IN & SW2_MASK) == 0 ? BUTTON_PRESSED : BUTTON_RELEASED;
    if (sw2_curr_state == BUTTON_RELEASED && sw2_prev_state == BUTTON_PRESSED)
    {
        ECE353_Events.sw2 = 1; 
    }
    sw2_prev_state = sw2_curr_state; 

    // Check the state of SW3
    button_state_t sw3_curr_state = (REG_PUSH_BUTTON_IN & SW3_MASK) == 0 ? BUTTON_PRESSED : BUTTON_RELEASED;
    if (sw3_curr_state == BUTTON_RELEASED && sw3_prev_state == BUTTON_PRESSED)
    {
        ECE353_Events.sw3 = 1;
    }
    sw3_prev_state = sw3_curr_state;

    // Check the joystick position
    joystick_position_t curr_position = joystick_get_pos();
    if (curr_position != prev_position)
    {
        ECE353_Events.joystick = 1;   
        prev_position = curr_position;
    } else {
        ECE353_Events.joystick = 0;
    }
}

#endif