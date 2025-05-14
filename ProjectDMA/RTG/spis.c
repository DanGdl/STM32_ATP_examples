/*
 * spis.c
 *
 *  Created on: May 5, 2025
 *      Author: max
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "spis.h"
#include "spi.h"


#define SPI_SENDER 			(&hspi4)
#define SPI_RECEIVER 		(&hspi5)

#define SPI_SENDER_RXED		(1)
#define SPI_SENDER_ERR 		(1<<2)
#define SPI_RECEIVER_RXED	(1<<3)
#define SPI_RECEIVER_ERR 	(1<<4)

#define SPI_TEST_MASK 		(SPI_SENDER_RXED | SPI_SENDER_ERR | SPI_RECEIVER_RXED | SPI_RECEIVER_ERR)


static uint8_t spis_state = 0;
static uint8_t spis_state_tmp = 0;
static uint8_t spi_test_status = 0;
static TestCommand_t spi_cmd = { 0 };
static uint8_t spi_buffer[257] = { 0 };


/*
HAL_SPI_StateTypeDef:
HAL_SPI_STATE_RESET      = 0x00U,    //!< Peripheral not Initialized
HAL_SPI_STATE_READY      = 0x01U,    //!< Peripheral Initialized and ready for use
HAL_SPI_STATE_BUSY       = 0x02U,    //!< an internal process is ongoing
HAL_SPI_STATE_BUSY_TX    = 0x03U,    //!< Data Transmission process is ongoing
HAL_SPI_STATE_BUSY_RX    = 0x04U,    //!< Data Reception process is ongoing
HAL_SPI_STATE_BUSY_TX_RX = 0x05U,    //!< Data Transmission and Reception process is ongoing
HAL_SPI_STATE_ERROR      = 0x06U,    //!< SPI error state
HAL_SPI_STATE_ABORT      = 0x07U     //!< SPI abort is ongoing

#define HAL_SPI_ERROR_NONE              (0x00000000U)   //!< No error
#define HAL_SPI_ERROR_MODF              (0x00000001U)   //!< MODF error
#define HAL_SPI_ERROR_CRC               (0x00000002U)   //!< CRC error
#define HAL_SPI_ERROR_OVR               (0x00000004U)   //!< OVR error
#define HAL_SPI_ERROR_FRE               (0x00000008U)   //!< FRE error
#define HAL_SPI_ERROR_DMA               (0x00000010U)   //!< DMA transfer error
#define HAL_SPI_ERROR_FLAG              (0x00000020U)   //!< Error on RXNE/TXE/BSY/FTLVL/FRLVL Flag
#define HAL_SPI_ERROR_ABORT             (0x00000040U)   //!< Error during SPI Abort procedure
*/
static void SPIS_check_errors(void) {
	if (spis_state & SPI_SENDER_ERR) {
		spis_state &= ~SPI_SENDER_ERR;
		printf("Error on SPI sender: 0x%"PRIX32", state 0x%X\r\n", HAL_SPI_GetError(SPI_SENDER), HAL_SPI_GetState(SPI_SENDER));

		HAL_SPI_Abort(SPI_SENDER);
	}
	if (spis_state & SPI_RECEIVER_ERR) {
		spis_state &= ~SPI_RECEIVER_ERR;
		printf("Error on SPI receiver: 0x%"PRIX32", state 0x%X\r\n", HAL_SPI_GetError(SPI_RECEIVER), HAL_SPI_GetState(SPI_RECEIVER));

		HAL_SPI_Abort(SPI_RECEIVER);
	}
}

// called when buffer is full
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi == SPI_SENDER) {
		spis_state |= SPI_SENDER_RXED;
	}
	else if (hspi == SPI_RECEIVER) {
		spis_state |= SPI_RECEIVER_RXED;
	}
	else {
		UNUSED(hspi);
	}
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi == SPI_SENDER) {
		SPI_RECEIVER->Instance->CR1 |= SPI_CR1_SSI; // turn OFF
	}
	else {
		UNUSED(hspi);
	}
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
	if (hspi == SPI_SENDER) {
		spis_state |= SPI_SENDER_ERR;
	}
	else if (hspi == SPI_RECEIVER) {
		spis_state |= SPI_RECEIVER_ERR;
	}
	else {
		UNUSED(hspi);
	}
}

TestResult_t SPIS_perform_test(const TestCommand_t* const command, uint32_t* const test_id) {
	if (command == NULL || command->header.pattern_len == 0 || test_id == NULL) {
		return RC_BAD_PARAMS;
	}
	*test_id = spi_cmd.header.test_id;
	if (spi_test_status == 0) {
		spi_cmd = *command;
		spi_test_status = 1;
		memset(spi_buffer, 0, sizeof(spi_buffer));

		SPI_RECEIVER->Instance->CR1 |= SPI_CR1_SSI; // turn OFF
		HAL_StatusTypeDef status = HAL_SPI_Receive_DMA(SPI_RECEIVER, (uint8_t*) spi_buffer, command->header.pattern_len);
		if (status != HAL_OK) {
			printf("Failed to receive on SPI receiver: %d, error 0x%"PRIX32"\r\n", status, HAL_SPI_GetError(SPI_RECEIVER));
			return RC_ERROR;
		}

		spis_state_tmp = spis_state & SPI_TEST_MASK;
		SPI_RECEIVER->Instance->CR1 &= ~SPI_CR1_SSI; // turn ON

		status = HAL_SPI_Transmit_DMA(SPI_SENDER, (uint8_t*) command->pattern, command->header.pattern_len);
		if (status != HAL_OK) {
			SPI_RECEIVER->Instance->CR1 |= SPI_CR1_SSI; // turn OFF
			printf("Failed to send on SPI sender: %d, error 0x%"PRIX32"\r\n", status, HAL_SPI_GetError(SPI_SENDER));
			return RC_ERROR;
		}
	}
	if (spi_test_status == 1 && (spis_state & SPI_TEST_MASK) != spis_state_tmp) {
		SPI_RECEIVER->Instance->CR1 |= SPI_CR1_SSI;
		spi_test_status = 0;
		if (spis_state & (SPI_SENDER_ERR | SPI_RECEIVER_ERR)) {
			SPIS_check_errors();
			spis_state &= ~SPI_TEST_MASK;
			return RC_ERROR;
		}
		spis_state &= ~SPI_TEST_MASK;
		return memcmp(spi_buffer, spi_cmd.pattern, spi_cmd.header.pattern_len) == 0 ? RC_OK : RC_FAIL;
	}
	return RC_TEST_IN_PROGRESS;
}

void SPIS_reset_flags(void) {
	spi_test_status = 0;
	spis_state_tmp = 0;
	spis_state = 0;
}
