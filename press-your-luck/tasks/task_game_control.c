/**
 * @file task_game_control.c
 * @author Kenneth Oh (oh87@wisc.edu)
 * @brief
 * @version 0.1
 * @date 2024-12-09
 *
 * @copyright Copyright (c) 2024
 */

#include "task_game_control.h"
#include "portmacro.h"
#include "task_io_expander.h"
#include "task_remote_rx.h"
#include "task_remote_tx.h"

#if defined(HW05)

#define DEFAULT_RANDOM_DELAY 1500
#define DEFAULT_ACTIVE_DELAY 400

#include "main.h"
#include "press-your-luck/pyl.h"
#include "press-your-luck/tasks/ece353_tasks.h"
#include "press-your-luck/tasks/task_console.h"
#include "press-your-luck/tasks/task_eeprom.h"
#include "press-your-luck/tasks/task_lcd_game_info.h"
#include "press-your-luck/tasks/task_lcd_squares.h"

extern cyhal_trng_t trng_obj;

char STUDENTS[] = "Kenneth Oh, Pranav Jayabalan";
char TEAM[] = "Team02";

/* Queues */
TaskHandle_t Task_Handle_Game_Control;
TimerHandle_t Timer_Handle_Randomize;
TimerHandle_t Timer_Handle_Active_Square;
QueueHandle_t q_Control_Rx_Data;
QueueHandle_t q_Control_EEPROM_Rx;

void timer_randomize_callback(TimerHandle_t xTimer)
{
    /* Set the  in the event group */
    xEventGroupSetBits(eg_UI, EVENT_TMR_RAND_SQUARES);
}

void timer_active_callback(TimerHandle_t xTimer)
{
    /* Set the  in the event group */
    xEventGroupSetBits(eg_UI, EVENT_TMR_RAND_ACTIVE);
}

void task_game_control(void* param)
{
    EventBits_t active_events;
    image_type_t active_square;

    image_t* pyl_images = pvPortMalloc(14 * sizeof(image_t));

    (void)param;

    task_print("\x1b[2J\x1b[;H");
    task_print("**************************************************\n\r");
    task_print("* %s\n\r", APP_DESCRIPTION);
    task_print("* Date: %s\n\r", __DATE__);
    task_print("* Time: %s\n\r", __TIME__);
    task_print("* Team: %s\n\r", TEAM);
    task_print("* Students: %s\n\r", STUDENTS);
    task_print("**************************************************\n\r");

    /* Start the random image timer */
    xTimerStart(Timer_Handle_Randomize, portMAX_DELAY);

    /* Start the Random Square Timer*/
    xTimerStart(Timer_Handle_Active_Square, portMAX_DELAY);

    /* Initialize game info messages */
    game_info_msg_t local_stats = { .cmd = CMD_UPDATE_LOCAL_DATA, .score = 0, .spins = 5, .passed = 0 };
    game_info_msg_t remote_stats = { .cmd = CMD_UPDATE_REMOTE_DATA, .score = 0, .spins = 5, .passed = 0 };
    game_info_msg_t claim_active = { .cmd = CMD_CLAIM_ACTIVE, .score = 0, .spins = 5, .passed = 0 };
    game_info_msg_t pass_spins = { .cmd = CMD_PASS_SPINS, .score = 0, .spins = local_stats.spins, .passed = 0 };
    game_info_msg_t game_over = { .cmd = CMD_GAME_OVER, .score = 0, .spins = 0, .passed = 0 };
    game_info_msg_t restart_game = { .cmd = CMD_RESTART, .score = 0, .spins = 5, .passed = 0 };

    /* Initialize local variable */
    uint8_t active_square_index = 0;
    uint8_t num_whammies = 0;
    uint16_t high_score_read = 0;
    bool is_active = false;

    /* Repeatedly running part of the task */
    for (;;) {

        xQueueSend(q_IO_Exp, &num_whammies, portMAX_DELAY);

        /* Read high score from EEPROM */
        eeprom_msg_t msg;
        msg.operation = EEPROM_READ;
        msg.return_queue = q_Control_EEPROM_Rx;
        xQueueSend(q_EEPROM, &msg, portMAX_DELAY);
        xQueueReceive(q_Control_EEPROM_Rx, &msg, portMAX_DELAY);
        high_score_read = msg.score;
        task_print_info("High Score: %u", high_score_read);

        /* Clear events that may have occurred when not in the Active state */
        xEventGroupClearBits(eg_UI, 0xFFFFFF);

        /* Draw empty squares */
        for (int i = 0; i < 14; i++) {
            pyl_images[i].reset = true;
            pyl_images[i].square_id = i;
            xQueueSend(q_LCD_SQUARES, &pyl_images[i], portMAX_DELAY);
        }

        /* Display LCD with local and remote stats*/
        xQueueSend(q_LCD_Info, &local_stats, portMAX_DELAY);
        xQueueSend(q_LCD_Info, &remote_stats, portMAX_DELAY);

        active_events = xEventGroupWaitBits(eg_UI, EVENT_UI_SW3 | EVENT_IPC_RX_DATA, pdTRUE, pdFALSE, portMAX_DELAY);

        /* Check if the remote or the local user pressed SW3 first */
        if ((active_events & EVENT_IPC_RX_DATA) == EVENT_IPC_RX_DATA) {
            xQueueReceive(q_Control_Rx_Data, &remote_stats, portMAX_DELAY);
            if (remote_stats.cmd == CMD_CLAIM_ACTIVE) {
                // Remote user pressed SW3 first
                xEventGroupSetBits(eg_UI, EVENT_START_GAME);
                is_active = 0;
            }
        } else if ((active_events & EVENT_UI_SW3) == EVENT_UI_SW3) {
            // Local user pressed SW3 before remote user
            is_active = 1;
            xEventGroupSetBits(eg_UI, EVENT_START_GAME);
            xQueueSend(q_Remote_Tx, &claim_active, portMAX_DELAY);
            pyl_images_randomize(pyl_images);
            pyl_images[active_square_index].invert_colors = true;
            for (int i = 0; i < 14; i++) {
                pyl_images[i].reset = false;
                xQueueSend(q_LCD_SQUARES, &pyl_images[i], portMAX_DELAY);
            }
        }

        xEventGroupClearBits(eg_UI, 0xFFFF9F);
        active_events = xEventGroupWaitBits(eg_UI, EVENT_START_GAME, pdTRUE, pdFALSE, portMAX_DELAY);

        if ((active_events & EVENT_START_GAME) == EVENT_START_GAME) {
            // Enter the main game loop once the game starts

            while (1) {
                // Check if this board is the active player
                if (is_active) {
                    // Wait for SW1, SW2, or Timer events that randomize squares/active square
                    active_events = xEventGroupWaitBits(eg_UI,
                        EVENT_UI_SW1 | EVENT_UI_SW2 | EVENT_TMR_RAND_SQUARES | EVENT_TMR_RAND_ACTIVE,
                        pdTRUE, pdFALSE, portMAX_DELAY);

                    // If SW1 was pressed, select the active squar
                    if ((active_events & EVENT_UI_SW1) == EVENT_UI_SW1) {
                        // Identify the image type of the currently active square
                        active_square = pyl_images[active_square_index].image_type;
                        // Blink the active square 3 times (off/on) to indicate selection
                        for (int i = 0; i < 3; i++) {
                            pyl_images[active_square_index].invert_colors = true;
                            xQueueSend(q_LCD_SQUARES, &pyl_images[active_square_index], portMAX_DELAY);
                            vTaskDelay(750);
                            pyl_images[active_square_index].invert_colors = false;
                            xQueueSend(q_LCD_SQUARES, &pyl_images[active_square_index], portMAX_DELAY);
                            vTaskDelay(750);
                        }
                        // If the local player has passed spins available, use those first
                        if (local_stats.passed > 0) {
                            switch (active_square) {
                            case IMG_TYPE_0100:
                            case IMG_TYPE_0250:
                            case IMG_TYPE_0500:
                            case IMG_TYPE_1000:
                            case IMG_TYPE_2500:
                            case IMG_TYPE_5000:
                                local_stats.score += pyl_images[active_square_index].value;
                                local_stats.passed--;
                                break;
                            case IMG_TYPE_1000_PLUS_SPIN:
                            case IMG_TYPE_2500_PLUS_SPIN:
                            case IMG_TYPE_5000_PLUS_SPIN:
                                local_stats.score += pyl_images[active_square_index].value;
                                local_stats.passed--;
                                local_stats.spins++;
                                break;
                            case IMG_TYPE_WHAMMY:
                                local_stats.score = 0;
                                local_stats.passed--;
                                local_stats.spins += local_stats.passed;
                                local_stats.passed = 0;
                                num_whammies++;
                                break;
                            default:
                                break;
                            }
                        }
                        // If no passed spins, but the player still has earned spins
                        else if (local_stats.spins > 0) {
                            switch (active_square) {
                            case IMG_TYPE_0100:
                            case IMG_TYPE_0250:
                            case IMG_TYPE_0500:
                            case IMG_TYPE_1000:
                            case IMG_TYPE_2500:
                            case IMG_TYPE_5000:
                                local_stats.score += pyl_images[active_square_index].value;
                                local_stats.spins -= 1;
                                break;
                            case IMG_TYPE_1000_PLUS_SPIN:
                            case IMG_TYPE_2500_PLUS_SPIN:
                            case IMG_TYPE_5000_PLUS_SPIN:
                                local_stats.score += pyl_images[active_square_index].value;
                                break;
                            case IMG_TYPE_WHAMMY:
                                local_stats.score = 0;
                                local_stats.spins -= 1;
                                num_whammies++;
                                break;
                            default:
                                break;
                            }
                        }
                        // If no spins remain (passed or earned)
                        else {
                            is_active = 0;
                            // Send current local stats to the remote user
                            xQueueSend(q_Remote_Tx, &local_stats, portMAX_DELAY);
                            remote_stats.spins = local_stats.spins;
                            remote_stats.passed = local_stats.passed;
                            remote_stats.score = local_stats.score;
                            xQueueSend(q_LCD_Info, &remote_stats, portMAX_DELAY);
                            // Clear all squares to reflect inactivity
                            for (int i = 0; i < 14; i++) {
                                pyl_images[i].reset = true;
                                pyl_images[i].square_id = i;
                                xQueueSend(q_LCD_SQUARES, &pyl_images[i], portMAX_DELAY);
                            }
                            break;
                        }

                        xQueueSend(q_IO_Exp, &num_whammies, portMAX_DELAY);
                        // Check for game-over conditions:
                        // 1. If 4 whammies have been selected by the player
                        // 2. If both players have no spins remaining (passed or earned)
                        if (num_whammies >= 4 || (local_stats.spins == 0 && local_stats.passed == 0 && remote_stats.spins == 0 && remote_stats.passed == 0)) {

                            uint16_t temp_score = local_stats.score;
                            remote_stats.score = 0;
                            local_stats.spins = 0;
                            local_stats.passed = 0;
                            remote_stats.spins = 0;
                            remote_stats.passed = 0;
                            is_active = false;
                            game_over.score = local_stats.score;
                            xQueueSend(q_Remote_Tx, &game_over, portMAX_DELAY);
                            xQueueSend(q_LCD_Info, &local_stats, portMAX_DELAY);
                            xQueueSend(q_LCD_Info, &game_over, portMAX_DELAY);

                            eeprom_msg_t msg;
                            msg.operation = EEPROM_READ;
                            msg.return_queue = q_Control_EEPROM_Rx;
                            xQueueSend(q_EEPROM, &msg, portMAX_DELAY);
                            xQueueReceive(q_Control_EEPROM_Rx, &msg, portMAX_DELAY);
                            high_score_read = msg.score;

                            // If current score is greater than the high score, update EEPROM
                            if (temp_score > high_score_read) {
                                msg.operation = EEPROM_WRITE;
                                msg.score = temp_score;
                                msg.return_queue = q_Control_EEPROM_Rx;
                                xQueueSend(q_EEPROM, &msg, portMAX_DELAY);
                            }

                            active_events = xEventGroupWaitBits(eg_UI, EVENT_UI_SW3 | EVENT_IPC_RX_DATA,
                                pdTRUE, pdFALSE, portMAX_DELAY);
                            // If SW3 was pressed locally, reset the game state and inform remote
                            if ((active_events & EVENT_UI_SW3) == EVENT_UI_SW3) {
                                num_whammies = 0;
                                local_stats.spins = 5;
                                remote_stats.spins = 5;
                                xQueueSend(q_Remote_Tx, &restart_game, portMAX_DELAY);
                            } else if ((active_events & EVENT_IPC_RX_DATA) == EVENT_IPC_RX_DATA) {
                                xQueueReceive(q_Control_Rx_Data, &remote_stats, portMAX_DELAY);
                            }

                            break;
                        }
                        // Additional condition where local has no spins, but remote does have spins
                        else if (num_whammies >= 4 || (local_stats.spins == 0 && local_stats.passed == 0 && (remote_stats.spins != 0 || remote_stats.passed != 0))) {
                            // pass spins to remote user
                            pass_spins.score = local_stats.score;
                            pass_spins.spins = local_stats.spins;
                            pass_spins.passed = local_stats.spins;
                            remote_stats.passed = local_stats.spins;

                            xQueueSend(q_Remote_Tx, &pass_spins, portMAX_DELAY);

                            local_stats.spins = 0;
                            xQueueSend(q_LCD_Info, &local_stats, portMAX_DELAY);
                            xQueueSend(q_LCD_Info, &remote_stats, portMAX_DELAY);

                            // local user becomes inactive
                            is_active = false;

                            // Draw empty squares to reset board
                            for (int i = 0; i < 14; i++) {
                                pyl_images[i].reset = true;
                                pyl_images[i].square_id = i;
                                xQueueSend(q_LCD_SQUARES, &pyl_images[i], portMAX_DELAY);
                            }
                        } else {
                            // Update LCD with local stats if game not over or no special action taken
                            xQueueSend(q_LCD_Info, &local_stats, portMAX_DELAY);
                            local_stats.cmd = CMD_UPDATE_REMOTE_DATA;
                            xQueueSend(q_Remote_Tx, &local_stats, portMAX_DELAY);
                            local_stats.cmd = CMD_UPDATE_LOCAL_DATA;

                            // Notify remote of updated local stats
                            if (local_stats.spins == 0 && local_stats.passed == 0) {
                                is_active = 0;
                                xQueueSend(q_Remote_Tx, &claim_active, portMAX_DELAY);
                                xQueueSend(q_LCD_Info, &local_stats, portMAX_DELAY);
                                for (int i = 0; i < 14; i++) {
                                    pyl_images[i].reset = true;
                                    pyl_images[i].square_id = i;
                                    xQueueSend(q_LCD_SQUARES, &pyl_images[i], portMAX_DELAY);
                                }
                            }
                        }
                    } else if ((active_events & EVENT_UI_SW2) == EVENT_UI_SW2) {
                        task_print_info("SW2 Pressed");

                        // pass spins to remote user if earned spins > 0 and passed spins = 0
                        if (local_stats.spins > 0 && local_stats.passed == 0) {
                            // pass spins to remote user
                            pass_spins.score = local_stats.score;
                            pass_spins.spins = local_stats.spins;
                            pass_spins.passed = local_stats.spins;
                            remote_stats.passed = local_stats.spins;

                            xQueueSend(q_Remote_Tx, &pass_spins, portMAX_DELAY);

                            local_stats.spins = 0;
                            xQueueSend(q_LCD_Info, &local_stats, portMAX_DELAY);
                            xQueueSend(q_LCD_Info, &remote_stats, portMAX_DELAY);

                            // local user becomes inactive
                            is_active = 0;

                            // Draw empty squares to reset board
                            for (int i = 0; i < 14; i++) {
                                pyl_images[i].reset = true;
                                pyl_images[i].square_id = i;
                                xQueueSend(q_LCD_SQUARES, &pyl_images[i], portMAX_DELAY);
                            }
                        }
                    } else if ((active_events & EVENT_TMR_RAND_SQUARES) == EVENT_TMR_RAND_SQUARES) {
                        pyl_images_randomize(pyl_images);
                        for (int i = 0; i < 14; i++) {
                            pyl_images[i].reset = false;
                            xQueueSend(q_LCD_SQUARES, &pyl_images[i], portMAX_DELAY);
                        }
                    } else if ((active_events & EVENT_TMR_RAND_ACTIVE) == EVENT_TMR_RAND_ACTIVE) {
                        uint16_t old_index = active_square_index;
                        active_square_index = cyhal_trng_generate(&trng_obj) % 14;
                        pyl_images[active_square_index].invert_colors = true;
                        xQueueSend(q_LCD_SQUARES, &pyl_images[active_square_index], portMAX_DELAY);
                        pyl_images[old_index].invert_colors = false;
                        xQueueSend(q_LCD_SQUARES, &pyl_images[old_index], portMAX_DELAY);
                    }

                    xEventGroupClearBits(eg_UI, 0xFFFFFF);
                } else {
                    active_events = xEventGroupWaitBits(eg_UI, EVENT_IPC_RX_DATA, pdTRUE, pdFALSE, portMAX_DELAY);
                    // If there is data available from the remote user (EVENT_IPC_RX_DATA), receive it.
                    // This code processes commands sent from the remote player to update the local state.
                    // Depending on the received command, we may update LCD info, adjust local spins,
                    // switch which player is active, or handle game over scenarios.
                    // After processing certain commands (like GAME_OVER), we may wait for user input
                    // (SW3 press) or additional remote data before restarting the game.
                    if ((active_events & EVENT_IPC_RX_DATA) == EVENT_IPC_RX_DATA) {
                        xQueueReceive(q_Control_Rx_Data, &remote_stats, portMAX_DELAY);
                        if (remote_stats.cmd == CMD_UPDATE_REMOTE_DATA) {
                            xQueueSend(q_LCD_Info, &remote_stats, portMAX_DELAY);
                            if ((remote_stats.passed == 0 && remote_stats.spins == 0) && (local_stats.passed > 0 || local_stats.spins > 0)) {
                                is_active = 1;
                            }
                        } else if (remote_stats.cmd == CMD_PASS_SPINS) {
                            local_stats.passed = remote_stats.passed;
                            remote_stats.spins = 0;
                            remote_stats.passed = 0;
                            remote_stats.cmd = CMD_UPDATE_REMOTE_DATA;
                            xQueueSend(q_LCD_Info, &local_stats, portMAX_DELAY);
                            xQueueSend(q_LCD_Info, &remote_stats, portMAX_DELAY);
                            is_active = 1;
                        } else if (remote_stats.cmd == CMD_GAME_OVER) {
                            local_stats.spins = 5;
                            remote_stats.spins = 5;
                            remote_stats.cmd = CMD_UPDATE_REMOTE_DATA;
                            xQueueSend(q_LCD_Info, &remote_stats, portMAX_DELAY);
                            xQueueSend(q_LCD_Info, &game_over, portMAX_DELAY);
                            eeprom_msg_t msg;
                            msg.operation = EEPROM_READ;
                            msg.return_queue = q_Control_EEPROM_Rx;
                            xQueueSend(q_EEPROM, &msg, portMAX_DELAY);
                            xQueueReceive(q_Control_EEPROM_Rx, &msg, portMAX_DELAY);
                            high_score_read = msg.score;
                            if (local_stats.score > high_score_read) {
                                msg.operation = EEPROM_WRITE;
                                msg.score = local_stats.score;
                                msg.return_queue = q_Control_EEPROM_Rx;
                                xQueueSend(q_EEPROM, &msg, portMAX_DELAY);
                            }
                            active_events = xEventGroupWaitBits(eg_UI, EVENT_UI_SW3 | EVENT_IPC_RX_DATA,
                                pdTRUE, pdFALSE, portMAX_DELAY);
                            if ((active_events & EVENT_UI_SW3) == EVENT_UI_SW3) {
                                xQueueSend(q_Remote_Tx, &restart_game, portMAX_DELAY);
                            } else if ((active_events & EVENT_IPC_RX_DATA) == EVENT_IPC_RX_DATA) {
                                xQueueReceive(q_Control_Rx_Data, &remote_stats, portMAX_DELAY);
                            }
                            break;
                        }
                    }
                }
            }
        }

        xEventGroupClearBits(eg_UI, 0xFFFFFF);
    }
}

void task_game_control_init(void)
{
    q_Control_Rx_Data = xQueueCreate(1, sizeof(game_info_msg_t));
    q_Control_EEPROM_Rx = xQueueCreate(1, sizeof(eeprom_msg_t));

    Timer_Handle_Randomize = xTimerCreate(
        "Randomize",
        DEFAULT_RANDOM_DELAY,
        true, (void*)0,
        timer_randomize_callback);

    Timer_Handle_Active_Square = xTimerCreate(
        "Active Square",
        DEFAULT_ACTIVE_DELAY,
        true,
        (void*)0,
        timer_active_callback);

    xTaskCreate(
        task_game_control,
        "Task Game Control",
        10 * configMINIMAL_STACK_SIZE,
        NULL,
        ECE353_TASK_PRIORITY_FSM,
        &Task_Handle_Game_Control);
}

#endif
