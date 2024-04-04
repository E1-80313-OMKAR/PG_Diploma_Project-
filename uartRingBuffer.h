#ifndef UARTRINGBUFFER_H_
#define UARTRINGBUFFER_H_

#include "stm32f4xx_hal.h"

#define UART_BUFFER_SIZE 512

typedef struct
{
  unsigned char buffer[UART_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
} ring_buffer;


void Ringbuf_init(void);


int IsDataAvailable(void);


int Uart_peek();


int Copy_upto (char *string, char *buffertocopyinto);


int Wait_for (char *string);


void Uart_isr (UART_HandleTypeDef *huart);



#endif /* UARTRINGBUFFER_H_ */
