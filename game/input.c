/*
 * input.c
 *
 *  Created on: Oct 20, 2017
 *      Author: superman
 */

#include "xil_types.h"
#include "input.h"
#include "gpio.h"
#include "control.h"

#define LEFT_BTN 0x08	//bit mask for left button
#define SHOOT_BTN 0x01	//bit mask for shoot button
#define RIGHT_BTN 0x02	//bit mask for right button
//state machine for tank movement and shooting
void input_tank_controls(void) {

	if (button_state & SHOOT_BTN) {
		control_tank_fire();
	}
	if (button_state & LEFT_BTN) {
		control_tank_move(LEFT);
	} else if (button_state & RIGHT_BTN) {
		control_tank_move(RIGHT);
	}
}
