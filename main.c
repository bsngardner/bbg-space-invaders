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

//Variables
u32 idle_count = 0;
u32 max_idle_count = 0;
u32 avg_idle_count = 0;
u32 min_idle_count = ULONG_MAX;

//Prototypes
void interrupt_init();
void interrupt_handler_dispatcher();

#define DEBUG
void print(char *str);
void init();

int main() {

	init();

	while (1) {
		idle_count = 0;
		while (timer_flag == 0) {
			idle_count++;
		}
		timer_flag = 0;
		if (idle_count > max_idle_count)
			max_idle_count = idle_count;
		if (idle_count < min_idle_count)
			min_idle_count = idle_count;
		avg_idle_count = (avg_idle_count - (avg_idle_count >> 2)) + (idle_count
				>> 2);
		//Tick state machines
		time_advance_tick();
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

