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
	RESET = 0,
	IDLE = 1,
	ADVANCE_ALIENS = 2,
	UPDATE_BULLET = 3,
	UPDATE_MISSILES = 4,
	MOVE_SAUCER = 5,
	FIRE_MISSILE = 6
} state = RESET;

char* states[] = { "reset\n\r", "idle\n\r", "advance_aliens\n\r",
		"update_bullet\n\r", "update_missiles\n\r", "move_saucer\n\r",
		"fire_missile\n\r" };

#define ALIEN_PERIOD 16
#define BULLET_PERIOD 2
#define MISSILE_PERIOD 5
#define SAUCER_PERIOD 6
#define SAUCER_INIT 500
#define ALIEN_MISS_PERIOD 75
#define ALIEN_MISS_INIT 400

u32 saucer_pause();

void time_advance_tick() {
	static u32 alien_period = ALIEN_PERIOD;
	static u32 bullet_period = BULLET_PERIOD;
	static u32 missile_period = MISSILE_PERIOD;
	static u32 saucer_period = SAUCER_INIT;
	static u32 alien_miss_period = ALIEN_MISS_INIT;

#ifdef DEBUG
	if (prev_state != state) {
		print(states[state]);
		prev_state = state;
	}
#endif

	//State actions
	switch (state) {
	case RESET:
		break;
	case IDLE:
		if (alien_period)
			alien_period--;
		if (bullet_period)
			bullet_period--;
		if (missile_period)
			missile_period--;
		if (saucer_period)
			saucer_period--;
		if (alien_miss_period)
			alien_miss_period--;
		break;
	case ADVANCE_ALIENS:
		//call advance aliens function
		game_controller_update_alien_position();
		break;
	case UPDATE_BULLET:
		//call update missiles and bullets
		game_controller_update_bullet();
		break;
	case UPDATE_MISSILES:
		game_controller_update_missiles();
		break;
	case MOVE_SAUCER:

		if(game_controller_saucer_state()) {
			xil_printf("UPDATE SAUCER\r\n");
			game_controller_move_saucer();
		} else
			game_controller_saucer_state_toggle();
		break;
	case FIRE_MISSILE:
		game_controller_fire_alien_missile();
		break;
	}

	//State transitions
	switch (state) {
	case RESET:
		state = IDLE;
		break;
	case IDLE:
		if (alien_period == 0) {
			state = ADVANCE_ALIENS;
		} else if (saucer_period == 0) {
			state = MOVE_SAUCER;
		} else if (missile_period == 0) {
			state = UPDATE_MISSILES;
		} else if (bullet_period == 0) {
			state = UPDATE_BULLET;
		} else if (alien_miss_period == 0) {
			state = FIRE_MISSILE;
		}

		break;
	case ADVANCE_ALIENS:
		alien_period = ALIEN_PERIOD;
		state = IDLE;
		break;
	case UPDATE_BULLET:
		bullet_period = BULLET_PERIOD;
		state = IDLE;
		break;
	case UPDATE_MISSILES:
		missile_period = MISSILE_PERIOD;
		state = IDLE;
		break;
	case MOVE_SAUCER:
		if(game_controller_saucer_state())
			saucer_period = SAUCER_PERIOD;
		else {
			saucer_period = saucer_pause();
			xil_printf("SAUCER STATE: %d\r\n", game_controller_saucer_state());
		}
		state = IDLE;
		break;
	case FIRE_MISSILE:
		alien_miss_period = ALIEN_MISS_PERIOD;
		state = IDLE;
		break;
	}

}
#define RANDOM_UP_RANGE 1000
#define RANDOM_LOW_RANGE 750
u32 saucer_pause() {
	u32 random_no = (rand() % (RANDOM_UP_RANGE - RANDOM_LOW_RANGE ) + RANDOM_LOW_RANGE);
	xil_printf("RANDOM NUMBER: %d\r\n", random_no);
	return random_no;
}
