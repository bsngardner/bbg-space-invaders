/*
 * gpio.c
 *
 *  Created on: Oct 17, 2017
 *      Author: superman
 */
#include <stdio.h>
#include "xil_types.h"
#include "platform.h"
#include "xparameters.h"
#include "xaxivdma.h"
#include "xio.h"
#include "xgpio.h"
#include "mb_interface.h"   // provides the microblaze interrupt enables, etc.
#include "xintc_l.h"        // Provides handy macros for the interrupt controller.
#include "gpio.h"
#include "timer.h"
//variables
XGpio gpLED; // This is a handle for the LED GPIO block.
XGpio gpPB; // This is a handle for the push-button GPIO block.
volatile u32 button_state = 0;
volatile u32 gpio_button_flag;
void gpio_init() {
	u32 success;
	success = XGpio_Initialize(&gpPB, XPAR_PUSH_BUTTONS_5BITS_DEVICE_ID);
	// Set the push button peripheral to be inputs.
	XGpio_SetDataDirection(&gpPB, 1, 0x0000001F);
	// Enable the global GPIO interrupt for push buttons.
	XGpio_InterruptGlobalEnable(&gpPB);
	// Enable all interrupts in the push button peripheral.
	XGpio_InterruptEnable(&gpPB, 0xFFFFFFFF);
}

void gpio_interrupt_handler() {

	// Clear the GPIO interrupt.
	XGpio_InterruptGlobalDisable(&gpPB); // Turn off all PB interrupts for now.
	button_state = XGpio_DiscreteRead(&gpPB, 1); // Get the current state of the buttons.

	timer_set_debounce(); //Initiate debouncing

	XGpio_InterruptClear(&gpPB, 0xFFFFFFFF); // Ack the PB interrupt.
	XGpio_InterruptGlobalEnable(&gpPB); // Re-enable PB interrupts.
}
