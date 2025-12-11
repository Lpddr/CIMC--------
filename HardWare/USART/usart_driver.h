#ifndef UART_H
#define UART_H

#include "HeaderFiles.h"
#define UART_RX_BUFFER_SIZE 128
#define UART_TIMEOUT_MS 100

extern uint8_t uart_dma_rx_buffer[UART_RX_BUFFER_SIZE];
extern uint8_t uart_read_buffer[UART_RX_BUFFER_SIZE];
extern uint8_t uart_flag;

void Uart_Init(void);
void Uart_Proc(void);
int my_printf(uint32_t usart_periph, const char *format, ...);
#endif
