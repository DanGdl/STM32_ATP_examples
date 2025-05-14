/*
 * i2cs.h
 *
 *  Created on: May 5, 2025
 *      Author: max
 */

#ifndef I2CS_H_
#define I2CS_H_

#include <stdint.h>
#include "protocol.h"

TestResult_t I2CS_perform_test(const TestCommand_t* const command, uint32_t* const test_id);

void I2CS_reset_flags(void);

#endif /* I2CS_H_ */
