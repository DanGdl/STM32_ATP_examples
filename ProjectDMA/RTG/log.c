
#include "usart.h"

extern UART_HandleTypeDef huart3;

#ifdef __GNUC__

#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

#else

#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)

#endif


PUTCHAR_PROTOTYPE {
//	HAL_StatusTypeDef status = HAL_UART_Transmit(&huart3, (uint8_t*) &ch, 1, HAL_MAX_DELAY);
//	assert_param(status == HAL_OK);
	HAL_UART_Transmit(&huart3, (uint8_t*) &ch, 1, HAL_MAX_DELAY);
	return ch;
}


#ifdef __GNUC__

#define GETCHAR_PROTOTYPE int __io_getchar()

#else

#define GETCHAR_PROTOTYPE int fgetc(FILE *f)

#endif

GETCHAR_PROTOTYPE {
	uint8_t ch = 0;
	HAL_UART_Receive(&huart3, &ch, 1, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart3, &ch, 1, HAL_MAX_DELAY);
	return ch;
}

int _read(int file, char *ptr, int len) {
	UNUSED(len);
	UNUSED(ptr);
	UNUSED(file);
	uint8_t ch = 0;
	HAL_UART_Receive(&huart3, &ch, 1, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart3, &ch, 1, HAL_MAX_DELAY);
	*ptr = ch;
	return 1;
}
