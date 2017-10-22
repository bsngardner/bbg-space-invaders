/*
 * timing.c
 *
 *  Created on: Oct 17, 2017
 *      Author: superman
 */
#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include "xil_types.h"
#include "timing.h"
#include "control.h"
#include "game.h"
#include "render.h"

static enum {
	INIT = 0, //do nothing state
	RUNNING = 1,
	GAME_OVER = 2,
	WIN = 3
} state = INIT; //enum for declaring the states

#define ALIEN_PERIOD 80 //freqeuency of alien movement
#define BULLET_PERIOD 2 //frequency of bullet movement
#define MISSILE_PERIOD 12 //frequency of missile movement
#define SAUCER_PERIOD 20 //frequency of saucer movement
#define SAUCER_INIT 200 //initial frequency of saucer
#define ALIEN_MISS_PERIOD 200 //frequency of alien missile shooting
#define ALIEN_MISS_INIT 600 //initial frequency of alien missile shooting
#define EXPLODE_PERIOD 5
#define EXPLODE_WAIT 10

void timing_game_tick() {
	static u32 alien_period = ALIEN_PERIOD; //alien move
	static u32 bullet_period = BULLET_PERIOD; //bullet move
	static u32 missile_period = MISSILE_PERIOD; //missile move
	static u32 saucer_period = SAUCER_INIT; //saucer move
	static u32 alien_miss_period = ALIEN_MISS_INIT; //missile shoot
	static u32 explode_period = EXPLODE_PERIOD;

	switch (state) {
	case INIT:
		srand(time(0)); //random seed
		control_init(); //initialize the game
		render_init();
		print("init complete\n\r");
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
		} else if (explode_period == 0) {
			control_tank_explode();
			explode_period = EXPLODE_PERIOD;
		}

		break;
	case GAME_OVER:
		render_gameover();
		break;
	case WIN:
		timing_restart_game();
		break;
	}

	control_run();
}

void timing_set_gameover() {
	state = GAME_OVER;
}

void timing_set_win() {
	state = WIN;
}

void timing_restart_game() {
	control_init();
	render_restart();
	state = RUNNING;
}

#define RANDOM_UP_RANGE 750 //upper bound of the random wait time
#define RANDOM_LOW_RANGE 500 //lower bound
u32 saucer_pause() {
	//generate random number for the saucer wait period
	u32 random_no = (rand() % (RANDOM_UP_RANGE - RANDOM_LOW_RANGE)
			+ RANDOM_LOW_RANGE);
	return random_no;
}
