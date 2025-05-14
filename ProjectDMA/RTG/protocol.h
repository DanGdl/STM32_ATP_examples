/*
 * protocol.h
 *
 *  Created on: May 5, 2025
 *      Author: max
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdint.h>

#define LEN_ARRAY(x)		(sizeof(x)/sizeof(x[0]))
#define LEN_STR(x)			(sizeof(x)/sizeof(x[0]) - 1)
#define SIZE_BUFFER_PAGE 	10

#pragma pack (push, 1)


typedef enum TestResult {
	RC_NONE,				// uninitailized
	RC_OK,					// all OK
	RC_ERROR,				// peripherals error
	RC_FAIL,				// test failed
	RC_BAD_PARAMS,			// invalid command
	RC_TEST_IN_PROGRESS,	// test running
	RC_TEST_UNSUPPORTED		// test running
} TestResult_t;

typedef enum CommandSource {
	CMD_NONE, CMD_UART, CMD_ETH
} CommandSource_t;

typedef enum Peripheral {
	UART = 1, I2C, SPI, CAN
} Peripheral_t;


typedef struct TestCmdHeader {
	uint32_t test_id;
	Peripheral_t peripheral_id;
	uint8_t iterations;
	uint8_t pattern_len;
} TestCmdHeader_t;

typedef struct TestCommand {
	TestCmdHeader_t header;
	uint8_t pattern[256];
} TestCommand_t;

#pragma pack (pop)

#endif /* PROTOCOL_H_ */
