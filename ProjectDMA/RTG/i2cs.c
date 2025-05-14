/*
 * i2cs.c
 *
 *  Created on: May 5, 2025
 *      Author: max
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "i2cs.h"
#include "i2c.h"


#define I2C_SENDER 			(&hi2c1)
#define I2C_RECEIVER 		(&hi2c2)

#define I2C_SENDER_RXED		(1)
#define I2C_SENDER_ERR 		(1<<2)
#define I2C_RECEIVER_RXED	(1<<3)
#define I2C_RECEIVER_ERR 	(1<<4)

#define I2C_TEST_MASK 		(I2C_SENDER_RXED | I2C_SENDER_ERR | I2C_RECEIVER_RXED | I2C_RECEIVER_ERR)


static uint8_t i2cs_state = 0;
static uint8_t i2cs_state_tmp = 0;
static uint8_t i2c_test_status = 0;
static TestCommand_t i2c_cmd = { 0 };
static uint8_t i2c_buffer[257] = { 0 };


static void I2CS_check_errors(void) {
	if (i2cs_state & I2C_SENDER_ERR) {
		i2cs_state &= ~I2C_SENDER_ERR;
		printf("Error on I2C sender: 0x%"PRIX32", state 0x%X\r\n", HAL_I2C_GetError(I2C_SENDER), HAL_I2C_GetState(I2C_SENDER));
	}
	if (i2cs_state & I2C_RECEIVER_ERR) {
		i2cs_state &= ~I2C_RECEIVER_ERR;
		printf("Error on I2C receiver: 0x%"PRIX32", state 0x%X\r\n", HAL_I2C_GetError(I2C_RECEIVER), HAL_I2C_GetState(I2C_RECEIVER));
	}
}

// called when buffer is full
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *i2c) {
	if (i2c == I2C_SENDER) {
		i2cs_state |= I2C_SENDER_RXED;
	}
	else if (i2c == I2C_RECEIVER) {
		i2cs_state |= I2C_RECEIVER_RXED;
	}
	else {
		UNUSED(i2c);
	}
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *i2c) {
	if (i2c == I2C_SENDER) {
		i2cs_state |= I2C_SENDER_ERR;
	}
	else if (i2c == I2C_RECEIVER) {
		i2cs_state |= I2C_RECEIVER_ERR;
	}
	else {
		UNUSED(i2c);
	}
}

TestResult_t I2CS_perform_test(const TestCommand_t* const command, uint32_t* const test_id) {
	if (command == NULL || command->header.pattern_len == 0 || test_id == NULL) {
		return RC_BAD_PARAMS;
	}
	*test_id = i2c_cmd.header.test_id;
	if (i2c_test_status == 0) {
		i2c_cmd = *command;
		i2c_test_status = 1;
		memset(i2c_buffer, 0, sizeof(i2c_buffer));

		HAL_StatusTypeDef status = HAL_I2C_Slave_Receive_DMA(I2C_RECEIVER, (uint8_t*) i2c_buffer, command->header.pattern_len);
		if (status != HAL_OK) {
			printf("Failed to receiveIT on I2C receiver: %d, error 0x%"PRIX32"\r\n", status, HAL_I2C_GetError(I2C_RECEIVER));
			return RC_ERROR;
		}

		i2cs_state_tmp = i2cs_state & I2C_TEST_MASK;
		status = HAL_I2C_Master_Transmit_DMA(I2C_SENDER, I2C_RECEIVER->Init.OwnAddress1, (uint8_t*) command->pattern, command->header.pattern_len);
		if (status != HAL_OK) {
			printf("Failed to sendIT on I2C sender: %d, error 0x%"PRIX32"\r\n", status, HAL_I2C_GetError(I2C_SENDER));
			return RC_ERROR;
		}
	}
	if (i2c_test_status == 1 && (i2cs_state & I2C_TEST_MASK) != i2cs_state_tmp) {
		i2c_test_status = 0;
		if (i2cs_state & (I2C_SENDER_ERR | I2C_RECEIVER_ERR)) {
			I2CS_check_errors();
			i2cs_state &= ~I2C_TEST_MASK;
			return RC_ERROR;
		}
		i2cs_state &= ~I2C_TEST_MASK;
		return memcmp(i2c_buffer, i2c_cmd.pattern, i2c_cmd.header.pattern_len) == 0 ? RC_OK : RC_FAIL;
	}
	return RC_TEST_IN_PROGRESS;
}

void I2CS_reset_flags(void) {
	i2c_test_status = 0;
	i2cs_state_tmp = 0;
	i2cs_state = 0;
}
