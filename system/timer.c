/*
 * timer.c
 *
 *  Created on: Oct 17, 2017
 *      Author: superman
 */
#include "xil_types.h"

#include "timer.h"

//Defines
#define DEBOUNCE_COUNT 5

//Variables
volatile u32 debounce_cnt = 0;
volatile u32 timer_flag = 0;
volatile u32 button_flag = 0;

void timer_interrupt_handler() {
	timer_flag = 1;

	if (debounce_cnt && !(--debounce_cnt)) {
		button_flag = 1;
	}
}

void timer_set_debounce() {
	debounce_cnt = DEBOUNCE_COUNT;
}
