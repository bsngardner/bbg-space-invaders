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

void game_controller_update_alien_position(void); //function that updates the alien block position
void game_controller_kill_alien(void); //function that queries the user for an alien to kill, then wipes it out
void game_controller_fire_alien_missile(void); //function that fires the alien missiles
void game_controller_update_bullet(void); //function that updates the position of all bullets, both alien and tank
void game_controller_update_missiles(void); //update missile positions
void game_controller_erode_bunker(void); //function that updates (erodes) the states of the bunkers
void game_controller_move_saucer(void); //move the flying saucer
u8 game_controller_saucer_state(void);
void game_controller_saucer_state_toggle(void);
u8 game_controller_tank_life(void);
void game_controller_explode_(void);
u8 game_controller_saucer_life(void);
void game_controller_saucer_explode(void);
#endif /* GAME_CONTROLLER_H_ */
