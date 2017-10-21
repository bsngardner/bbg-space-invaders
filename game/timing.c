/*
 * timing.c
 *
 *  Created on: Oct 17, 2017
 *      Author: superman
 */
#include <stdio.h>
#include <stdlib.h>
#include "xil_types.h"
#include "timing.h"
#include "control.h"
#include "game.h"

static enum {
	INIT = 0, //do nothing state
	RUNNING = 1,
	GAME_OVER = 2
} state = INIT; //enum for declaring the states

#define ALIEN_PERIOD 40 //freqeuency of alien movement
#define BULLET_PERIOD 2 //frequency of bullet movement
#define MISSILE_PERIOD 6 //frequency of missile movement
#define SAUCER_PERIOD 10 //frequency of saucer movement
#define SAUCER_INIT 200 //initial frequency of saucer
#define ALIEN_MISS_PERIOD 200 //frequency of alien missile shooting
#define ALIEN_MISS_INIT 600 //initial frequency of alien missile shooting
#define EXPLODE_PERIOD 5
#define EXPLODE_WAIT 10
#define SAUCER_KILL_PERIOD 15

void timing_game_tick() {
	static u32 alien_period = ALIEN_PERIOD; //alien move
	static u32 bullet_period = BULLET_PERIOD; //bullet move
	static u32 missile_period = MISSILE_PERIOD; //missile move
	static u32 saucer_period = SAUCER_INIT; //saucer move
	static u32 alien_miss_period = ALIEN_MISS_INIT; //missile shoot
	static u32 explode_period = EXPLODE_PERIOD;
	static u32 saucer_kill_period = SAUCER_KILL_PERIOD;

	switch (state) {
	case INIT:
		print("init\n\r");
		state = RUNNING;
	break;
	case RUNNING:
		if (alien_period) //if alien timer is above 0
			alien_period--; //decrement
		if (bullet_period) //repeat logic for all other timers
			bullet_period--;
		if (missile_period) //missile timer
			missile_period--;
		if (saucer_period) //saucer timer
			saucer_period--;
		if (saucer_kill_period) //saucer timer
			saucer_kill_period--;
		if (alien_miss_period) //missile shooting timer
			alien_miss_period--;
		if (explode_period)
			explode_period--;

		if (alien_period == 0) { //check to see if timers have decremented to 0
			control_update_alien_position();
			alien_period = ALIEN_PERIOD;
		} else if (missile_period == 0) {
			control_update_missiles();
			missile_period = MISSILE_PERIOD;
		} else if (bullet_period == 0) {
			bullet_period = BULLET_PERIOD;
			control_update_bullet();
		} else if (saucer_period == 0) {
			saucer_period = SAUCER_PERIOD;
			control_saucer_move(); //if active then move
		} else if (alien_miss_period == 0) {
			alien_miss_period = ALIEN_MISS_PERIOD;
			control_alien_fire_missile();

		}
	break;
	case GAME_OVER:
		print("gameover\n\r");
	break;
	}

	control_run();
}

#define RANDOM_UP_RANGE 750 //upper bound of the random wait time
#define RANDOM_LOW_RANGE 500 //lower bound
u32 saucer_pause() {
	//generate random number for the saucer wait period
	u32 random_no = (rand() % (RANDOM_UP_RANGE - RANDOM_LOW_RANGE)
			+ RANDOM_LOW_RANGE);
	return random_no;
}
