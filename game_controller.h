/*
 * game_controller.h
 *
 *  Created on: Oct 4, 2017
 *      Author: superman
 */

#ifndef GAME_CONTROLLER_H_
#define GAME_CONTROLLER_H_


#include "xil_types.h"

#define GAME_CONTROLLER_MISSILES 4		//number of missiles



typedef enum {
	LEFT, RIGHT		//directions that the aliens and tank move
} direction;

void game_controller_init(void); //starts up the game and initializes the key components
void game_controller_run(void); //function that blocks on the user input and goes to correct function

void move_tank(direction); //function that moves the tank
void update_alien_position(void); //function that updates the alien block position
void kill_alien(void); //function that queries the user for an alien to kill, then wipes it out
void fire_tank_bullet(void); //function that fires the tank bullet
void fire_alien_missile(void); //function that fires the alien missiles
void update_bullets(void); //function that updates the position of all bullets, both alien and tank
void erode_bunker(void); //function that updates (erodes) the states of the bunkers
void init_bunker_states(void); //function that sets the bunker states to max health

#endif /* GAME_CONTROLLER_H_ */
