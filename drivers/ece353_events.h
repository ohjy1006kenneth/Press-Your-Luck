/**
 * @file ece353_events.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2024-08-14
 * 
 * @copyright Copyright (c) 2024
 * 
 */
 #ifndef  __ECE353_EVENTS_H__
 #define  __ECE353_EVENTS_H__

 typedef struct ece353_events_t
{
    unsigned int sw1 : 1;       /* Falling edge of SW1 detected */
    unsigned int sw2 : 1;       /* Falling edge of SW2 detected */
    unsigned int sw3 : 1;       /* Falling edge of SW3 detected */
    unsigned int joystick : 1;  /* Joystick has moved position */
    unsigned int ipc_rx : 1;    /* Data received from remote board */
    unsigned int tmr_0100_ms : 1;  /* Timer 100ms has occured*/
    unsigned int io_exp_button_press : 1; /* indicates that the button is connected to the IO expander*/
} ece353_events_t;

extern volatile ece353_events_t ECE353_Events;

 #endif