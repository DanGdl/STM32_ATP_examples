/*
 * uarts.h
 *
 *  Created on: May 5, 2025
 *      Author: max
 */

#ifndef UARTS_H_
#define UARTS_H_

#include <stdint.h>
#include "protocol.h"

TestResult_t UARTS_perform_test(const TestCommand_t* const command, uint32_t* const test_id);

void UARTS_reset_flags(void);

void UARTS_SendTestResult(const uint8_t* const response, unsigned short resp_len);

void UARTS_get_command(TestCommand_t* const command);

void UARTS_Send_test_cmd(void);

#endif /* UARTS_H_ */
