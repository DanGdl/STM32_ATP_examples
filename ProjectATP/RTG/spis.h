/*
 * spis.h
 *
 *  Created on: May 5, 2025
 *      Author: max
 */

#ifndef SPIS_H_
#define SPIS_H_

#include <stdint.h>
#include "protocol.h"

TestResult_t SPIS_perform_test(const TestCommand_t* const command, uint32_t* const test_id);

void SPIS_reset_flags(void);

#endif /* SPIS_H_ */
