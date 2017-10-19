/*
 * timer.c
 *
 *  Created on: Oct 17, 2017
 *      Author: superman
 */
#include "xil_types.h"

#include "timer.h"
#include "gpio.h"

//Defines
#define DEBOUNCE_COUNT 12

//Variables
volatile u32 debounce_cnt = 0;
volatile u32 timer_flag = 0;
volatile u32 timer_missed = 0;

void timer_interrupt_handler() {
	if (timer_flag == 1) {
		timer_missed = 1;
	}
	timer_flag = 1;

	if (debounce_cnt && !(--debounce_cnt)) {
		gpio_button_flag = 1;
	}
}

void timer_set_debounce() {
	debounce_cnt = DEBOUNCE_COUNT;
}
