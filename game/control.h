/*
 * control.h
 *
 *  Created on: Oct 4, 2017
 *      Author: superman
 */

#ifndef CONTROL_H_
#define CONTROL_H_

#include "xil_types.h"

#define CONTROL_MISSILES 4		//number of missiles
typedef enum {
	LEFT, RIGHT
//directions that the aliens and tank move
} direction_t;

void control_init(void); //starts up the game and initializes the key components
void control_run(void); //function that blocks on the user input and goes to correct function

void control_tank_fire();
void control_update_bullet(void); //function that updates the position of all bullets, both alien and tank
void control_tank_move(direction_t dir);
void control_alien_fire_missile();
void control_kill_alien(u16, u16);
void control_update_alien_position(void); //function that updates the alien block position
void control_tank_explode();

void control_update_missiles(void); //update missile positions
void control_saucer_move(void); //move the flying saucer

#endif /* CONTROL_H_ */
