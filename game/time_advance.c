/*
 * time_advance.c
 *
 *  Created on: Oct 17, 2017
 *      Author: superman
 */
#include <stdio.h>
#include <stdlib.h>
#include "xil_types.h"
#include "time_advance.h"
#include "game_controller.h"

static enum {
	RESET = 0, //do nothing state
	IDLE = 1, //update timers
	ADVANCE_ALIENS = 2, //move alien block
	UPDATE_BULLET = 3, //move bullet
	UPDATE_MISSILES = 4, //move missiles
	MOVE_SAUCER = 5, //move saucer
	FIRE_MISSILE = 6 //shoot alien missile
} state = RESET; //enum for declaring the states

#define ALIEN_PERIOD 16 //freqeuency of alien movement
#define BULLET_PERIOD 2 //frequency of bullet movement
#define MISSILE_PERIOD 5 //frequency of missile movement
#define SAUCER_PERIOD 6 //frequency of saucer movement
#define SAUCER_INIT 500 //initial frequency of saucer
#define ALIEN_MISS_PERIOD 75 //frequency of alien missile shooting
#define ALIEN_MISS_INIT 400 //initial frequency of alien missile shooting

u32 saucer_pause(); //randomly generate a pause for the saucer

//state machine for various timed events
void time_advance_tick() {
	//set the periods for the first time
	static u32 alien_period = ALIEN_PERIOD; //alien move
	static u32 bullet_period = BULLET_PERIOD; //bullet move
	static u32 missile_period = MISSILE_PERIOD; //missile move
	static u32 saucer_period = SAUCER_INIT; //saucer move
	static u32 alien_miss_period = ALIEN_MISS_INIT; //missile shoot

	//State actions
	switch (state) {
	case RESET:
		break;
	case IDLE: //decrement timers
		if (alien_period) //if alien timer is above 0
			alien_period--; //decrement
		if (bullet_period) //repeat logic for all other timers
			bullet_period--;
		if (missile_period) //missile timer
			missile_period--;
		if (saucer_period) //saucer timer
			saucer_period--;
		if (alien_miss_period) //missile shooting timer
			alien_miss_period--;
		break;
	case ADVANCE_ALIENS:
		//call advance aliens function
		game_controller_update_alien_position();
		break;
	case UPDATE_BULLET:
		//call update bullets
		game_controller_update_bullet();
		break;
	case UPDATE_MISSILES:
		//call update missiles
		game_controller_update_missiles();
		break;
	case MOVE_SAUCER:
		//check to see if the saucer is active
		if(game_controller_saucer_state()) {
			game_controller_move_saucer(); //if active then move
		} else
			game_controller_saucer_state_toggle(); //else toggle to active
		break;
	case FIRE_MISSILE:
		//fire an alien missile
		game_controller_fire_alien_missile();
		break;
	}

	//State transitions
	switch (state) {
	case RESET:
		state = IDLE; //move directly to idle
		break;
	case IDLE:
		if (alien_period == 0) { //check to see if timers have decremented to 0
			state = ADVANCE_ALIENS; //priority level 1
		} else if (saucer_period == 0) {
			state = MOVE_SAUCER; //priority level 2
		} else if (missile_period == 0) {
			state = UPDATE_MISSILES; //priority level 3
		} else if (bullet_period == 0) {
			state = UPDATE_BULLET; //priority level 4
		} else if (alien_miss_period == 0) {
			state = FIRE_MISSILE; //priority level 5
		}
		break;
	case ADVANCE_ALIENS:
		alien_period = ALIEN_PERIOD; //reset the alien timer
		state = IDLE; //return to idle
		break;
	case UPDATE_BULLET:
		bullet_period = BULLET_PERIOD; //reset the bullet timer
		state = IDLE; //return to idle
		break;
	case UPDATE_MISSILES:
		missile_period = MISSILE_PERIOD; //reset the missile timer
		state = IDLE; //return to idle
		break;
	case MOVE_SAUCER:
		if(game_controller_saucer_state()) //if the saucer is active
			saucer_period = SAUCER_PERIOD; //reset to the normal movement period
		else {
			saucer_period = saucer_pause(); //if inactive reset to a random wait period
		}
		state = IDLE; //move back to idle
		break;
	case FIRE_MISSILE:
		alien_miss_period = ALIEN_MISS_PERIOD; //reset alien missile shooting timer
		state = IDLE; //move back to idle
		break;
	}

}
#define RANDOM_UP_RANGE 750 //upper bound of the random wait time
#define RANDOM_LOW_RANGE 500 //lower bound
u32 saucer_pause() {
	//generate random number for the saucer wait period
	u32 random_no = (rand() % (RANDOM_UP_RANGE - RANDOM_LOW_RANGE ) + RANDOM_LOW_RANGE);
	return random_no;
}
