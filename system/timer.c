/*
 * timer.c
 *
 *  Created on: Oct 17, 2017
 *      Author: superman
 */
#include <stdio.h>
#include "xil_types.h"
#include "xparameters.h"

#include "timer.h"
#include "gpio.h"

#include "xac97_l.h"

//Defines
#define DEBOUNCE_COUNT 8

//Variables
volatile u16 debounce_cnt = 0;
volatile u16 timer_flag = 0;
volatile u16 timer_missed = 0;

void timer_interrupt_handler() {
	if (timer_flag == 1) {
		timer_missed = 1;
	}
	timer_flag = 1;
	if (debounce_cnt && !(--debounce_cnt)) {
		gpio_button_flag = 1;
	}

	//	static u32 fifo_level;
	//	u32 fifo = XAC97_getInFIFOLevel(XPAR_AXI_AC97_0_BASEADDR);
	//	if (fifo != fifo_level) {
	//		fifo_level = fifo;
	//		xil_printf("FIFO: %d\n\r", fifo_level);
	//	}
}

void timer_set_debounce() {
	debounce_cnt = DEBOUNCE_COUNT;
}
