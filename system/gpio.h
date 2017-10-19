/*
 * gpio.h
 *
 *  Created on: Oct 17, 2017
 *      Author: superman
 */

#ifndef GPIO_H_
#define GPIO_H_

void gpio_init();
void gpio_interrupt_handler();

extern volatile u32 gpio_button_flag;
extern volatile u32 button_state;

#endif /* GPIO_H_ */
