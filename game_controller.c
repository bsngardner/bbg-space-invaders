/*
 * game_controller.c
 *
 *  Created on: Oct 4, 2017
 *      Author: superman
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "game_controller.h"
#include "game.h"
#include "render.h"
#include "bmp.h"
#include "table.h"

#define check(byte,bit) (byte & (table_bit[bit]))
#define set(byte,bit) (byte |= (table_bit[bit]))
#define clear(byte,bit) (byte &= ~(table_bit[bit]))

//array of 4 bunker states
u16 bunker_states[BUNKERS];
//array of 4 alien missiles
alien_missiles_t alien_missiles[MISSILES];
//
tank_t tank = { { TANK_X, TANK_Y }, { { -1, -1 }, 0 } };
alien_block_t block = { { ALIEN_X, ALIEN_Y }, 0, 0, { 0 }, OUT };

void game_controller_init(void) {

	//initialize the bunker states to full health
	init_bunker_states();

	u32 init_timer = ALIEN_INIT;
	//initialize the alien life/death array
	//memset(block.alien_status, ALIEN_ROW_ALIVE, ALIEN_ROWS * sizeof(u16));
	s16 i;
	for (i = 0; i < ALIEN_ROWS; i++) {
		block.alien_status[i] = ALIEN_ROW_ALIVE;
	}
	render(&tank, &block, &alien_missiles, bunker_states);
	for (; i < INITIAL_MOVES; i++) {
		while (init_timer)
			init_timer--; // Decrement the timer.
		init_timer = ALIEN_INIT; // Reset the timer.
		block.pos.x += MOVE_SPRITE;
		render(&tank, &block, &alien_missiles, bunker_states);
	}

	//random seed
	srand(time(0));
}

void game_controller_run(void) {
	//wait for keyboard input from the user
	char input;
	input = getchar();
	//switch statement for handling different keyboard presses
	switch (input) {
	//case for key 2
	case KEY_2:
		//ask the user for an alien to kill
		kill_alien();
		break;
		//case for key 3
	case KEY_3:
		//randomly pick an alien on the bottom row and fire a missile
		fire_alien_missile();
		break;
		//case for key 4
	case KEY_4:
		//move the tank to the left
		move_tank(LEFT);
		break;
		//case for key 5
	case KEY_5:
		//fire a tank bullet at the tank's current position
		fire_tank_bullet();
		break;
		//case for key 6
	case KEY_6:
		//move the tank to the right
		move_tank(RIGHT);
		break;
		//case for key 7
	case KEY_7:
		//prompt the user for a bunker to erode
		erode_bunker();
		break;
		//case for key 8
	case KEY_8:
		//move the alien block from left to right and right to left
		update_alien_position();
		break;
		//case for key 9
	case KEY_9:
		//move the alien missiles down and the tank bullets up
		update_bullets();
		break;
		//case for an invalid button press
	default:
		break;
	}
	render(&tank, &block, &alien_missiles, bunker_states);
}

void init_bunker_states(void) {
	u8 i;
	for (i = 0; i < BUNKERS; i++)
		bunker_states[i] = BUNKER_MAX;
}

direction alien_direction = RIGHT;

void move_tank(direction d) {
	if (d == LEFT) {
		if (tank.pos.x >= MOVE_SPRITE)
			tank.pos.x -= MOVE_SPRITE;
	} else if (d == RIGHT) {
		if (tank.pos.x <= GAME_WIDTH - BMP_TANK_W - MOVE_SPRITE)
			tank.pos.x += MOVE_SPRITE;
	}
}
void update_alien_position(void) {

	if (alien_direction == LEFT) {
		if (block.pos.x + block.loffset >= MOVE_SPRITE) {
			block.pos.x -= MOVE_SPRITE;
		} else {
			block.pos.y += MOVE_SPRITE;
			alien_direction = RIGHT;
		}

	} else if (alien_direction == RIGHT) {
		if (block.pos.x - block.roffset < RIGHT_WALL) {
			block.pos.x += MOVE_SPRITE;
		} else {
			block.pos.y += MOVE_SPRITE;
			alien_direction = LEFT;
		}
	}

	if (block.legs == OUT)
		block.legs = IN;
	else
		block.legs = OUT;
}

void kill_alien(void) {
	char inputs[BUFFER];
	u8 i = 0;
	u8 alien_no;
	char input;
	while (input != ENTER) {
		input = getchar();
		inputs[i] = input;
		i++;
		if (i > INPUT_OVERFLOW) {
			return;
		}
	}
	if (i == SINGLE_DIGITS) {
		alien_no = to_digit(inputs[0]);
	} else if (i == TWO_DIGITS) {
		alien_no = 10 * to_digit(inputs[0]) + to_digit(inputs[1]);
	} else {
		return;
	}

	if (alien_no >= ALIEN_ROWS * ALIEN_ROW_LEN) {
		return;
	}
	clear(block.alien_status[alien_no/ALIEN_ROW_LEN],alien_no%ALIEN_ROW_LEN);

	u16 mask = 0;
	for (i = 0; i < ALIEN_ROWS; i++) {
		mask |= block.alien_status[i];
	}
	u16 maskp = mask;
	u8 n = 10;

	while (maskp >>= 1) {
		n--;
	}

	i = 10;
	maskp = mask;
	while ((maskp = ((maskp << 1) & ALIEN_ROW_ALIVE))) {
		i--;
	}
	block.loffset = i * ALIEN_SEP;
	block.roffset = n * ALIEN_SEP;

}
void fire_tank_bullet(void) {
	if (tank.missile.active == 0) {
		tank.missile.pos.x = tank.pos.x + TANK_BULL_X;
		tank.missile.pos.y = tank.pos.y - TANK_BULL_Y;
		tank.missile.active = 1;
	}
}
void fire_alien_missile(void) {
	u8 i;
	u8 shooter_col;

	for (i = 0; i < MISSILES; i++) {
		if (!alien_missiles[i].active) {
			alien_missiles[i].active = 1;
			break;
		}
	}
	if (i == MISSILES) {
		return;
	}
	u8 fired = 0;
	u8 shooter_row = ALIEN_ROWS - 1;
	while (!fired) {
		shooter_col = rand() % ALIEN_COLS;
		while (!check(block.alien_status[shooter_row],shooter_col))
			shooter_row--;
		if (shooter_row != 0) {
			fired = 1;
		}
	}
	alien_missiles[i].pos.x = block.pos.x + shooter_col * ALIEN_WIDTH
			+ ALIEN_MID;
	alien_missiles[i].pos.y = block.pos.y + (shooter_row + 1) * ALIEN_SEP
			- ALIEN_MID;
	if (rand() % 4 == 0)
		alien_missiles[i].type = STRONG;
	else
		alien_missiles[i].type = NORMAL;

}
void update_bullets(void) {
	if (tank.missile.active) {
		tank.missile.pos.y = tank.missile.pos.y - MOVE_SPRITE;
		if (tank.missile.pos.y < 0) {
			tank.missile.active = 0;
		}
	}
	u8 i;
	for (i = 0; i < MISSILES; i++) {
		if (alien_missiles[i].active) {
			alien_missiles[i].pos.y = alien_missiles[i].pos.y + MOVE_SPRITE;
			if (alien_missiles[i].state == 1)
				alien_missiles[i].state = 0;
			else
				alien_missiles[i].state = 1;

			if (alien_missiles[i].pos.y >= GAME_HEIGHT) {
				alien_missiles[i].active = 0;
			}

		}
	}

}
void erode_bunker(void) {
	char input;
	input = getchar();
	u8 bunker_no = to_digit(input);
	if (bunker_no >= BUNKERS) {
		return;
	}
	if (bunker_states[bunker_no] != 0)
		bunker_states[bunker_no] = bunker_states[bunker_no]--;
}
