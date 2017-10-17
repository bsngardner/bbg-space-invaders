/*
 * timer.h
 *
 *  Created on: Oct 17, 2017
 *      Author: superman
 */

#ifndef TIMER_H_
#define TIMER_H_

void timer_interrupt_handler();
void timer_set_debounce();

extern volatile u32 timer_flag;


#endif /* TIMER_H_ */
