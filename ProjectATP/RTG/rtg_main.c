
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "udp.h"
#include "lwip.h"

#include "rtg_main.h"
#include "protocol.h"
#include "uarts.h"
#include "i2cs.h"
#include "spis.h"

extern struct netif gnetif; // my addr


#define SERVER_PORT 5000
#define TIMEOUT 	30 		// ticks (30  millis). See const HAL_TickFreqTypeDef freq = HAL_GetTickFreq();


typedef void (*ResetFlags_t)(void);
typedef void (*SendResult_t)(const uint8_t* const response, unsigned short resp_len);
typedef TestResult_t (*PerformTest_t)(const TestCommand_t* const command, uint32_t* const test_id);


CommandSource_t cmd_source = CMD_NONE;
TestCommand_t command_uart = { 0 };
TestCommand_t command_eth = { 0 };
uint8_t request_command = 1;

struct udp_pcb* upcb = NULL;
ip_addr_t remote_addr = { 0 };
u16_t remote_port = 0;



void udp_receive_callback(
	void *arg, struct udp_pcb* upcb, struct pbuf* p, const ip_addr_t* addr, u16_t port
) {
	UNUSED(arg);
	UNUSED(upcb);
	if (command_eth.header.test_id == 0 && p->len > sizeof(TestCmdHeader_t)) {
		const TestCmdHeader_t* const tmp = (TestCmdHeader_t*) p->payload;
		if ((tmp->pattern_len + sizeof(*tmp)) == p->len || sizeof(TestCommand_t) == p->len) {
			memcpy(&command_eth, p->payload, p->len);
			remote_addr = *addr;
			remote_port = port;
		} else {
			printf("Mallformed command. Expected %u, got %u\r\n", tmp->pattern_len + sizeof(*tmp), p->len);
		}
	} else {
		puts("Can't handle command from ETH, test in progress\r\n");
	}
	pbuf_free(p);
}

static int UDP_open_socket(void) {
	upcb = udp_new();
	err_t err = udp_bind(upcb, IP_ADDR_ANY, SERVER_PORT);
	if (err == ERR_OK) {
		udp_bind(upcb, &gnetif.ip_addr, SERVER_PORT);
		udp_recv(upcb, udp_receive_callback, NULL);
		return 0;
	} else {
		udp_remove(upcb);
		return -1;
	}
}

void UDP_send(const uint8_t* const response, unsigned short resp_len) {
	if (resp_len == 0 || response == NULL || upcb == NULL || remote_addr.addr == 0 || remote_port == 0) {
		return;
	}
	struct pbuf* udp_buffer = pbuf_alloc(PBUF_TRANSPORT, resp_len, PBUF_RAM);
	pbuf_take(udp_buffer, response, resp_len);
	if (udp_sendto(upcb, udp_buffer, &remote_addr, remote_port) != ERR_OK ) {
		puts("Failed to send response to UDP\r\n");
	}
	pbuf_free(udp_buffer);
}

void rtg_main(void) {
	puts("Card On Air\r\n");
	uint8_t perform_test = 0;
	TestCommand_t command = { 0 };

	uint8_t send_test_cmd = 0;

	uint32_t test_start_ts = 0;
	UDP_open_socket();
	while(1) {
		MX_LWIP_Process();
		sys_check_timeouts();

		// check if testId in command is not 0 (e.g we got something)
		if (perform_test == 0 &&  command_uart.header.test_id) {
			cmd_source = CMD_UART;
			command = command_uart;
			perform_test = 1;
			memset(&command_uart, 0, sizeof(command_uart));
		}
		else if (perform_test == 0 && command_eth.header.test_id) {
			cmd_source = CMD_ETH;
			command = command_eth;
			perform_test = 1;
			memset(&command_eth, 0, sizeof(command_eth));
		}

		// schedule receiving of command
		if (request_command) {
			UARTS_get_command(&command_uart);
			request_command = 0;

			if (send_test_cmd) {
				UARTS_Send_test_cmd();
				send_test_cmd = 0;
			}
		}

		if (perform_test) {
			if (!test_start_ts) {
				test_start_ts = HAL_GetTick();
			}
			uint32_t test_id = 0;
			PerformTest_t test_executor = NULL;
			ResetFlags_t reset_function = NULL;
			switch(command.header.peripheral_id) {
			case UART:
				test_executor = UARTS_perform_test;
				reset_function = UARTS_reset_flags;
				break;

			case I2C:
				test_executor = I2CS_perform_test;
				reset_function = I2CS_reset_flags;
				break;

			case SPI:
				test_executor = SPIS_perform_test;
				reset_function = SPIS_reset_flags;
				break;

			default:
				test_executor = NULL;
				reset_function = NULL;
			}
			const TestResult_t result = test_executor ? test_executor(&command, &test_id) : RC_TEST_UNSUPPORTED;

			short resp_len = 0;
			char response[128] = { 0 };
			switch(result) {
			case RC_OK:
				resp_len = sprintf(response, "%lu: %s", test_id, "OK");
				break;

			case RC_TEST_IN_PROGRESS:
				if (test_start_ts && (HAL_GetTick() - test_start_ts) >= TIMEOUT) {
					resp_len = sprintf(response, "%lu: %s", test_id, "Fail by timeout");
					if (reset_function) {
						reset_function();
					}
					break;
				} else {
					continue;
				}

			case RC_TEST_UNSUPPORTED:
				resp_len = sprintf(response, "%lu: %s", test_id, "Unsupported peripheral");
				break;

			case RC_ERROR:
				resp_len = sprintf(response, "%lu: %s", test_id, "Error");
				break;

			default:
				resp_len = sprintf(response, "%lu: %s", test_id, "NotOK");
			}
			SendResult_t sender = NULL;
			switch(cmd_source) {
			case CMD_UART:
				sender = UARTS_SendTestResult;
				break;

			case CMD_ETH:
				sender = UDP_send;
				break;

			default:
				sender = NULL;
			}
			if (sender && resp_len > 0) {
				response[resp_len] = '\0';
				sender((uint8_t*) response, resp_len);
			}
			else {
				printf("Unsupported command's source: %d\r\n", cmd_source);
			}
			printf("%s\r\n", response);
			test_start_ts = 0;
			command.header.iterations--;
			perform_test = command.header.iterations;
			request_command = !command.header.iterations;
		}
	}
}
