/**
 * @file pyl_ipc.c
 * @author Kenneth Oh (oh87@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2024-08-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */
 #include "pyl_ipc.h"

 #if defined (HW03) || defined (HW04)

/* Data Structure Used to store data being received from the remote UART */
remote_uart_data_t IPC_Rx_Data = {
    .num_bytes = 0, 
    .payload = {0,0,0,0,0,0,0,0,0,0}
};

/**
 * @brief
 * Verifies that the data received is valid
 * @param packet
 * An array of 4 bytes received from the remote board
 * @return true
 * Packet is valid
 * @return false
 * Packet contained invalid data
 */ 
static bool pyl_packet_verify(uint8_t *packet)
{
    /* ADD CODE */
    if (packet[0] == REMOTE_PACKET_START  &&
        packet[1] == CMD_UPDATE_REMOTE_DATA &&
        packet[6] == REMOTE_PACKET_END)
    {
        return true;
    }
    return false;
}


/**
 * @brief 
 * This function will initialize game_info with the data found in the 
 * circular buffer.  If the data in the circular buffer is not valid, then 
 * this function will return false.  If the data is valid, return true.
 *
 * Rx Interrupts should be enabled and data should be received using the 
 * Rx_Circular_Buffer
 * @param game_info 
  * @return true 
  * @return false 
 */
bool pyl_ipc_rx(game_info_msg_t *game_info)
{
    uint8_t packet[7];
    cyhal_uart_enable_event(&remote_uart_obj, CYHAL_UART_IRQ_RX_NOT_EMPTY, CYHAL_ISR_PRIORITY_DEFAULT, true);
    
    // Ensure there's enough data in the circular buffer for a full packet
    if (circular_buffer_get_num_bytes(Rx_Circular_Buffer) < 7) {
        return false; // Not enough data for a complete packet
    }
    

    // Extract data from the circular buffer
    for (int i = 0; i < 7; i++) {

        packet[i] = circular_buffer_remove(Rx_Circular_Buffer);
        printf("%x ",packet[i]);
    }

    // Verify the packet
    if (!pyl_packet_verify(packet)) {
        return false;
    }

    // Initialize game_info with the data from the packet
    game_info->cmd = (game_info_cmd_t)packet[1];
    game_info->score = (packet[2] << 8) | packet[3];
    game_info->spins = packet[4];
    game_info->passed = packet[5];

    return true;
}

/**
 * @brief 
 * This function will transmit the data in game_info to the remote uart. Your function
 * should only transmit 7 bytes of data.  DO NOT transmit a NULL character, \n, or \r
 *
 * Tx Interrupts should be enabled and data should be transmitted using the 
 * Tx_Circular_Buffer
 *@param game_info 
 */
bool pyl_ipc_tx(game_info_msg_t *game_info)
{
    uint8_t packet[7];

    // Prepare the packet
    packet[0] = REMOTE_PACKET_START;
    packet[1] = CMD_UPDATE_REMOTE_DATA;
    packet[2] = (game_info->score >> 8) & 0xFF; // Upper 8-bits of score
    packet[3] = game_info->score & 0xFF;        // Lower 8-bits of score
    packet[4] = game_info->spins;
    packet[5] = game_info->passed;
    packet[6] = REMOTE_PACKET_END;

    // Ensure there's enough space in the circular buffer for the full packet
    if (circular_buffer_get_num_bytes(Tx_Circular_Buffer) + 7 > Tx_Circular_Buffer->max_size) {
        printf("Not enough space");
        return false; // Not enough space for a complete packet
    }

    // Transmit the packet
    for (int i = 0; i < 7; i++) {
        circular_buffer_add(Tx_Circular_Buffer, packet[i]);
    }

    // Enable the Transmit Empty interrupt
    cyhal_uart_enable_event(&remote_uart_obj, CYHAL_UART_IRQ_TX_EMPTY, 7, true);

    return true;
}

/**
 * @brief 
 * Handler used to collect a packet of data from the remote board.
 * @param callback_arg 
 * @param event 
 */
void handler_ipc(void *callback_arg, cyhal_uart_event_t event)
{
    uint8_t c;

    /* remove compiler warnings */
    (void) callback_arg;
    (void) event;

    if ((event & CYHAL_UART_IRQ_TX_ERROR) == CYHAL_UART_IRQ_TX_ERROR)
    {
      /* An error occurred in Tx */
      /* Insert application code to handle Tx error */
    }
    else if ((event & CYHAL_UART_IRQ_RX_NOT_EMPTY) == CYHAL_UART_IRQ_RX_NOT_EMPTY)
    {
        /* ADD CODE */

        /* Read in the current character */
        cyhal_uart_getc(&remote_uart_obj, &c, 0);

        /* Add the character to the circular buffer */
        circular_buffer_add(Rx_Circular_Buffer, c);

        /* If REMOTE_PACKET_END received, set the corresponding bit in ECE353_Event */
        if (c == REMOTE_PACKET_END) {
            ECE353_Events.ipc_rx = 1;
        }

    }
    else if ((event & CYHAL_UART_IRQ_TX_EMPTY) == CYHAL_UART_IRQ_TX_EMPTY)
    {
        /* The UART finished transferring data, so check and see if
        * there is more data in the circular buffer to send*/
        if (!circular_buffer_empty(Tx_Circular_Buffer)) {
            c = circular_buffer_remove(Tx_Circular_Buffer);
            cyhal_uart_putc(&remote_uart_obj, c);
        } else {
            // No more data to send, disable TX empty interrupt
            cyhal_uart_enable_event(&remote_uart_obj, CYHAL_UART_IRQ_TX_EMPTY, 7, false);
        }
    }

}

 #endif