#include "uartRingBuffer.h"
#include <string.h>

/**** define the UART you are using  ****/

extern UART_HandleTypeDef huart1;

#define uart &huart1

#define TIMEOUT_DEF 500  // 500ms timeout for the functions
uint16_t timeout;

ring_buffer rx_buffer = { { 0 }, 0, 0};
ring_buffer tx_buffer = { { 0 }, 0, 0};

ring_buffer *_rx_buffer;
ring_buffer *_tx_buffer;

void store_char(unsigned char c, ring_buffer *buffer);


void Ringbuf_init(void)
{
  _rx_buffer = &rx_buffer;
  _tx_buffer = &tx_buffer;

  /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
  __HAL_UART_ENABLE_IT(uart, UART_IT_ERR);

  /* Enable the UART Data Register not empty Interrupt */
  __HAL_UART_ENABLE_IT(uart, UART_IT_RXNE);
}

/* checks, if the entered string is present in the giver buffer ?
 */
static int check_for(char *str, char *buffertolookinto) {
    int stringlength = strlen(str);
    int bufferlength = strlen(buffertolookinto);
    int so_far = 0;
    int indx = 0;

    while (indx < bufferlength) {
        if (str[so_far] == buffertolookinto[indx]) {
            so_far++;
            indx++;
            if (so_far == stringlength) {
                return 1;  // String found
            }
        } else {
            so_far = 0;
            indx++;
        }
    }

    return 0;  // String not found
}

/* checks if the new data is available in the incoming buffer
 */
int IsDataAvailable(void)
{
  return (uint16_t)(UART_BUFFER_SIZE + _rx_buffer->head - _rx_buffer->tail) % UART_BUFFER_SIZE;
}

int Uart_peek()
{
  if(_rx_buffer->head == _rx_buffer->tail)
  {
    return -1;
  }
  else
  {
    return _rx_buffer->buffer[_rx_buffer->tail];
  }
}


int Copy_upto(char *string, char *buffertocopyinto) {
    int so_far = 0;
    int len = strlen(string);
    int indx = 0;
    int found = 0;

    while (!found) {
        while (Uart_peek() != string[so_far]) {
            buffertocopyinto[indx] = _rx_buffer->buffer[_rx_buffer->tail];
            _rx_buffer->tail = (unsigned int)(_rx_buffer->tail + 1) % UART_BUFFER_SIZE;
            indx++;
            while (!IsDataAvailable());
        }

        while (Uart_peek() == string[so_far]) {
            so_far++;
            buffertocopyinto[indx++] = Uart_read();
            if (so_far == len) {
                found = 1;
                break;
            }
            timeout = TIMEOUT_DEF;
            while ((!IsDataAvailable()) && timeout);
            if (timeout == 0) return 0;
        }

        if (so_far != len) {
            so_far = 0;
        }
    }

    return 1;
}


int Wait_for(char *string) {
    int len = strlen(string);  // Length of the input string
    int currentPos = 0;  // Current position in the string

    // Loop until the entire string is found or a timeout occurs
    while (1) {
        timeout = TIMEOUT_DEF;  // Set the timeout value

        // Wait for data to become available or timeout
        while ((!IsDataAvailable()) && timeout);

        if (timeout == 0) return 0;  // Timeout occurred, return 0

        // Check if the next character in the UART buffer matches the current position in the string
        if (Uart_peek() == string[currentPos]) {
            currentPos++;  // Move to the next character in the string
            _rx_buffer->tail = (unsigned int)(_rx_buffer->tail + 1) % UART_BUFFER_SIZE;  // Move the tail pointer

            if (currentPos == len) return 1;  // Entire string found, return 1
        } else {
            // Reset position and start over
            currentPos = 0;
        }
    }

    // This should not be reached, but for completeness
    return 0;
}

void Uart_isr (UART_HandleTypeDef *huart)
{
	  uint32_t isrflags   = READ_REG(huart->Instance->SR);
	  uint32_t cr1its     = READ_REG(huart->Instance->CR1);

    /* if DR is not empty and the Rx Int is enabled */
    if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
    {
		huart->Instance->SR;                       /* Read status register */
        unsigned char c = huart->Instance->DR;     /* Read data register */
        store_char (c, _rx_buffer);  // store data in buffer
        return;
    }

    /*If interrupt is caused due to Transmit Data Register Empty */
    if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
    {
    	if(tx_buffer.head == tx_buffer.tail)
    	    {
    	      // Buffer empty, so disable interrupts
    	      __HAL_UART_DISABLE_IT(huart, UART_IT_TXE);

    	    }

    	 else
    	    {
    	      // There is more data in the output buffer. Send the next byte
    	      unsigned char c = tx_buffer.buffer[tx_buffer.tail];
    	      tx_buffer.tail = (tx_buffer.tail + 1) % UART_BUFFER_SIZE;
    	      huart->Instance->SR;
    	      huart->Instance->DR = c;

    	    }
    	return;
    }
}

