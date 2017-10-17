/*
 * time_advance.c
 *
 *  Created on: Oct 17, 2017
 *      Author: superman
 */
#include <stdio.h>
#include "xil_types.h"
#include "time_advance.h"

static enum {
	RESET, IDLE, ADVANCE_ALIENS, UPDATE_PROJECTILES, MOVE_SAUCER
} state = RESET;
void update_alien_position(void);

#define ALIEN_PERIOD 100
#define PROJ_PERIOD 20
#define SAUCER_PERIOD 100

void time_advance_tick() {
	static u32 alien_period = ALIEN_PERIOD;
	static u32 proj_period = PROJ_PERIOD;
	static u32 saucer_period = SAUCER_PERIOD;

	//State actions
	switch (state) {
	case RESET:
		break;
	case IDLE:
		if (alien_period)
			alien_period--;
		if (proj_period)
			proj_period--;
		if (saucer_period)
			saucer_period--;
		break;
	case ADVANCE_ALIENS:
		//call advance aliens function
		update_alien_position();
		break;
	case UPDATE_PROJECTILES:
		//call update missiles and bullets
		break;
	case MOVE_SAUCER:
		//call move saucer
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
		} else if (proj_period == 0) {
			state = UPDATE_PROJECTILES;
		} else if (saucer_period == 0) {
			state = MOVE_SAUCER;
		}

		break;
	case ADVANCE_ALIENS:
		alien_period = ALIEN_PERIOD;
		break;
	case UPDATE_PROJECTILES:
		proj_period = PROJ_PERIOD;
		break;
	case MOVE_SAUCER:
		break;
	}
}
