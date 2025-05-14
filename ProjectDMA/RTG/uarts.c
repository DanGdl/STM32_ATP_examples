/*
 * uarts.c
 *
 *  Created on: May 5, 2025
 *      Author: max
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>


#include "uarts.h"
#include "usart.h"


#define UART_SENDER 		(&huart4)
#define UART_RECEIVER 		(&huart6)
#define UART_COMMANDS 		(&huart7)


#define UART_COMMANDS_RXED	(1)
#define UART_COMMANDS_ERR 	(1<<2)

#define UART_SENDER_RXED	(1<<3)
#define UART_SENDER_ERR 	(1<<4)
#define UART_RECEIVER_RXED	(1<<5)
#define UART_RECEIVER_ERR 	(1<<6)

#define UART_TEST_MASK 		(UART_SENDER_RXED | UART_SENDER_ERR | UART_RECEIVER_RXED | UART_RECEIVER_ERR)


static uint8_t uarts_state = 0;
static uint8_t uarts_state_tmp = 0;
static uint8_t uart_test_status = 0;
static TestCommand_t uart_cmd = { 0 };
static uint8_t uarts_buffer[257] = { 0 };


static void UARTS_check_errors(void) {
	if (uarts_state & UART_SENDER_ERR) {
		uarts_state &= ~UART_SENDER_ERR;
		printf("Error on UART sender: 0x%"PRIX32", state 0x%lX\r\n", HAL_UART_GetError(UART_SENDER), HAL_UART_GetState(UART_SENDER));
	}
	if (uarts_state & UART_RECEIVER_ERR) {
		uarts_state &= ~UART_RECEIVER_ERR;
		printf("Error on UART receiver: 0x%"PRIX32", state 0x%lX\r\n", HAL_UART_GetError(UART_RECEIVER), HAL_UART_GetState(UART_RECEIVER));
	}
	if (uarts_state & UART_COMMANDS_ERR) {
		uarts_state &= ~UART_COMMANDS_ERR;
		printf("Error on UART commands: 0x%"PRIX32", state 0x%lX\r\n", HAL_UART_GetError(UART_COMMANDS), HAL_UART_GetState(UART_COMMANDS));
	}
}

// called when buffer is full
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == UART_SENDER) {
		uarts_state |= UART_SENDER_RXED;
	}
	else if (huart == UART_RECEIVER) {
		uarts_state |= UART_RECEIVER_RXED;
	}
	else if (huart == UART_COMMANDS) {
		uarts_state |= UART_COMMANDS_RXED;
	}
	else {
		UNUSED(huart);
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
	UNUSED(size);
	if (huart == UART_SENDER) {
		uarts_state |= UART_SENDER_RXED;
	}
	else if (huart == UART_RECEIVER) {
		uarts_state |= UART_RECEIVER_RXED;
	}
	else if (huart == UART_COMMANDS) {
		uarts_state |= UART_COMMANDS_RXED;
	}
	else {
		UNUSED(huart);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart == UART_SENDER) {
		uarts_state |= UART_SENDER_ERR;
	}
	else if (huart == UART_RECEIVER) {
		uarts_state |= UART_RECEIVER_ERR;
	}
	else if (huart == UART_COMMANDS) {
		uarts_state |= UART_COMMANDS_ERR;
	}
	else {
		UNUSED(huart);
	}
}

TestResult_t UARTS_perform_test(const TestCommand_t* const command, uint32_t* const test_id) {
	if (command == NULL || command->header.pattern_len == 0 || test_id == NULL) {
		return RC_BAD_PARAMS;
	}
	*test_id = uart_cmd.header.test_id;
	if (uart_test_status == 0) {
		uart_cmd = *command;
		uart_test_status = 1;
		memset(uarts_buffer, 0, sizeof(uarts_buffer));

		HAL_StatusTypeDef status = HAL_UART_Receive_DMA(UART_RECEIVER, (uint8_t*) uarts_buffer, command->header.pattern_len);
		if (status != HAL_OK) {
			printf("Failed to receiveIT on UART receiver: %d, error 0x%"PRIX32"\r\n", status, HAL_UART_GetError(UART_RECEIVER));
			return RC_ERROR;
		}

		uarts_state_tmp = uarts_state & UART_TEST_MASK;
		status = HAL_UART_Transmit_DMA(UART_SENDER, command->pattern, command->header.pattern_len);
		if (status != HAL_OK) {
			printf("Failed to sendIT on UART sender: %d, error 0x%"PRIX32"\r\n", status, HAL_UART_GetError(UART_SENDER));
			return RC_ERROR;
		}
	}
	if (uart_test_status == 1 && (uarts_state & UART_TEST_MASK) != uarts_state_tmp) {
		uart_test_status = 0;
		if (uarts_state & (UART_SENDER_ERR | UART_RECEIVER_ERR)) {
			UARTS_check_errors();

			uarts_state &= ~UART_TEST_MASK;
			return RC_ERROR;
		}
		uarts_state &= ~UART_TEST_MASK;
		return memcmp(uarts_buffer, uart_cmd.pattern, uart_cmd.header.pattern_len) == 0 ? RC_OK : RC_FAIL;
	}
	return RC_TEST_IN_PROGRESS;
}

void UARTS_reset_flags(void) {
	uart_test_status = 0;
	uarts_state_tmp = 0;
	uarts_state = 0;
}

void UARTS_SendTestResult(const uint8_t* const response, unsigned short resp_len) {
	if (response == NULL || resp_len == 0) {
		return;
	}
	HAL_UART_Transmit(UART_COMMANDS, response, resp_len, HAL_MAX_DELAY);
}

void UARTS_get_command(TestCommand_t* const command) {
	if (command == NULL) {
		return;
	}
	HAL_UART_Receive_IT(UART_COMMANDS, (uint8_t*) command, sizeof(*command));
}

void UARTS_Send_test_cmd(void) {
	TestCommand_t cmd = {
			.header = {
				.test_id = 1,
				.peripheral_id = SPI,
				.iterations = 4,
				.pattern_len = sizeof("YOHOHO and bottle of rum!"),
			},
			.pattern = "YOHOHO and bottle of rum!",
	};
	HAL_UART_Transmit(UART_COMMANDS, (uint8_t*) &cmd, sizeof(cmd), HAL_MAX_DELAY);
}
