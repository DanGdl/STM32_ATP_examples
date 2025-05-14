
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "tim.h"

extern TIM_HandleTypeDef htim2;
#define TIMER (&htim2)

/*
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PeriodElapsedHalfCpltCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_IC_CaptureHalfCpltCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_TriggerCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_TriggerHalfCpltCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim);
*/


uint8_t timer_error = 0;
uint32_t ts_timer_irq = 0;

void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim) {
	if (htim == TIMER) {
		timer_error = 1;
	}
	else {
		UNUSED(htim);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim == TIMER) {
		ts_timer_irq = HAL_GetTick();
		HAL_TIM_Base_Stop(htim);
	}
	else {
		UNUSED(htim);
	}
}

void rtg_main(void) {
	puts("Card On Air\r\n");
	uint32_t ts_timer_start = 0;

	while (1) {
		switch(ts_timer_start) {
		case 0:
			ts_timer_start = HAL_GetTick();
			const HAL_StatusTypeDef status = HAL_TIM_Base_Start_IT(TIMER);
			if (status != HAL_OK) {
				ts_timer_start = 0;
				printf("Failed to launch timer: 0x%X\r\n", status);
			}
			break;

		default:
			if (timer_error) {
				printf("Error on timer: state 0x%X\r\n", HAL_TIM_Base_GetState(TIMER));
				HAL_TIM_Base_Stop(TIMER);
			}
			else if (ts_timer_irq) {
				const uint32_t diff = (ts_timer_irq - ts_timer_start);
				 // 1 tick == 1 millis, See const HAL_TickFreqTypeDef freq = HAL_GetTickFreq();
				printf("Timer IRQ happened: %lu ticks => %lu millis\r\n", diff, diff * 1);
				ts_timer_start = 0;
				ts_timer_irq = 0;
			}
			break;
		}
	}

}
