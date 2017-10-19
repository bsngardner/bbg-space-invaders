/*
 * main.c: simple test application
 *
 * main file for startup. Inits platform and calls game_controller
 *
 *  Broderick Gardner
 *  Benjamin Gardner
 */

#include <stdio.h>
#include <limits.h>
#include "platform.h"
#include "xparameters.h"
#include "xaxivdma.h"
#include "xio.h"
#include "xgpio.h"
#include "mb_interface.h"   // provides the microblaze interrupt enables, etc.
#include "xintc_l.h"        // Provides handy macros for the interrupt controller.
#include "time.h"
#include "unistd.h"

#include "gpio.h"
#include "timer.h"
#include "render.h"
#include "game_controller.h"
#include "time_advance.h"

//Defines
#define MAX_IDLE_COUNT 66651

//Variables
u32 idle_count = 0;
u32 avg_util = 0;
u32 max_util = 0;
u32 max_avg_util = 0;

//Prototypes
void interrupt_init();
void interrupt_handler_dispatcher();

#define DEBUG
#define PRINT_UTIL
void print(char *str);
void init();

int main() {
	u32 i = 10;
	init();

	while (1) {
		idle_count = 0;
		while (timer_flag == 0) {
			idle_count++;
		}
		timer_flag = 0;

#ifdef PRINT_UTIL
		if (timer_missed) {
			print("Timer missed!\n\r");
			timer_missed = 0;
		}

		idle_count = MAX_IDLE_COUNT - idle_count;

		avg_util = (avg_util - (avg_util >> 2)) + (idle_count >> 2);
		if (!(--i)) {
			xil_printf("utilization: %d\n\r", avg_util);
			i = 50;
		}

		if (idle_count > max_util) {
			max_util = idle_count;
			xil_printf("Max utilization: %d\n\r", max_util);
		}

		if (avg_util > max_avg_util) {
			max_avg_util = avg_util;
			xil_printf("Max average utilization: %d\n\r", max_avg_util);
		}
#endif

		//Tick state machines
		time_advance_tick();
		game_controller_run();
		//		game_controller_run(); //run the game
	}

	cleanup_platform();

	return 0;
}

void init() {

	init_platform(); // Necessary for all programs.

	gpio_init();
	interrupt_init();

	microblaze_enable_interrupts();

	render_init();
	game_controller_init(); //initialize the game
}

void interrupt_init() {

	microblaze_register_handler(interrupt_handler_dispatcher, NULL);
	XIntc_EnableIntr(
			XPAR_INTC_0_BASEADDR,
			(XPAR_FIT_TIMER_0_INTERRUPT_MASK
					| XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK));
	XIntc_MasterEnable(XPAR_INTC_0_BASEADDR);
}

void interrupt_handler_dispatcher(void* ptr) {
	XIntc_MasterDisable(XPAR_AXI_INTC_0_BASEADDR);

	int intc_status = XIntc_GetIntrStatus(XPAR_INTC_0_BASEADDR);
	// Check the FIT interrupt first.
	if (intc_status & XPAR_FIT_TIMER_0_INTERRUPT_MASK) {
		XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_FIT_TIMER_0_INTERRUPT_MASK);
		timer_interrupt_handler();
	}
	// Check the push buttons.
	if (intc_status & XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK) {
		gpio_interrupt_handler();
		XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK);
	}

	XIntc_MasterEnable(XPAR_AXI_INTC_0_BASEADDR);
}

