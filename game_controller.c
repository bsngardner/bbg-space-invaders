/*
 * game_controller.c
 *
 *  Created on: Oct 4, 2017
 *      Author: superman
 */

#include "game_controller.h"

u8 alien_life[ALIENS];
//array of 4 bunker states
u16 bunker_states[BUNKERS];
//array of 4 alien missiles
alien_bullet_t alien_missiles[MISSILES];
//array of 4 alien missile states
u8 alien_missile_flag[MISSILES];
//
point_t tank_pos;
point_t tank_bullet_pos;
u8 tank_bullet_flag;
alien_block_t block;
point_t alien_block_pos;

void game_controller_init(void) {


	//initialize the x tank position to the middle of the screen
	tank_pos.x = TANK_X;
	//initialize the y tank position to the bottom of the screen
	tank_pos.y = TANK_Y;
	xil_printf("\r\nTANK POSITION %d %d\r\n", tank_pos.x, tank_pos.y);

	u32 init_timer = ALIEN_INIT;
	block.pos.x = ALIEN_XY;
	block.pos.y = ALIEN_XY;
	block.legs = OUT;
	//initialize the alien life/death array
	memset(block.alien_status, 1, ALIENS);
	render(&tank_pos, 0, &block, 0, 0);
	u8 i = 0;
	for (i; i < INITIAL_MOVES; i++) {
		while (init_timer)
			init_timer--; // Decrement the timer.
		init_timer = ALIEN_INIT; // Reset the timer.
		block.pos.x += MOVE_SPRITE;
		render(&tank_pos, 0, &block, 0, 0);
	}

	xil_printf("ALIEN BLOCK POSITION %d %d\r\n", block.pos.x,
			block.pos.y);

	//initialize the bunker states to full health
	init_bunker_states();
	//no tank flag yet
	tank_bullet_flag = 0;
	//random seed
	srand(time(0));
}

void game_controller_run(void) {
	//wait for keyboard input from the user
	char input;
	input = getchar();
	xil_printf("%c\r\n", input);
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
		xil_printf("Invalid input (%c)\r\n", input);
		break;
	}
	render(&tank_pos, 0, &block, 0, 0);
}


void init_bunker_states(void) {
	u8 i;
	for (i = 0; i < BUNKERS; i++)
		bunker_states[i] = BUNKER_MAX;
}

void print_bunkers(void) {
	u8 i;
	for (i = 0; i < BUNKERS; i++)
		xil_printf("%d", bunker_states[i]);
	xil_printf("\r\n");
}

direction alien_direction = LEFT;

void move_tank(direction d) {
	if (d == LEFT) {
		if (tank_pos.x >= MOVE_SPRITE)
			tank_pos.x -= MOVE_SPRITE;
	} else if (d == RIGHT) {
		if (tank_pos.x <= GAME_WIDTH + MOVE_SPRITE)
			tank_pos.x += MOVE_SPRITE;
	}
	xil_printf("TANK POSITION %d %d\r\n", tank_pos.x, tank_pos.y);
}
void update_alien_position(void) {

	if (alien_direction == LEFT) {
		if (block.pos.x >= MOVE_SPRITE) {
			block.pos.x -= MOVE_SPRITE;
		} else {
			block.pos.y += MOVE_SPRITE;
			alien_direction = RIGHT;
		}

	} else if (alien_direction == RIGHT) {
		if (block.pos.x < RIGHT_WALL) {
			block.pos.x += MOVE_SPRITE;
		} else {
			block.pos.y += MOVE_SPRITE;
			alien_direction = LEFT;
		}
	}
	xil_printf("ALIEN BLOCK POSITION %d %d\r\n", block.pos.x,
			block.pos.y);

	if (block.legs == OUT)
		block.legs = IN;
	else
		block.legs = OUT;
}

void print_array(void) {
	u8 i;
	for (i = 0; i < ALIENS; i++)
		xil_printf("%d", alien_life[i]);
	xil_printf("\r\n");
}

void kill_alien(void) {
	xil_printf("PLEASE SPECIFY A NUMBER 0-54\r\n");
	char inputs[BUFFER];
	u8 i = 0;
	u8 alien_no;
	char input;
	while (input != ENTER) {
		input = getchar();
		xil_printf("%c", input);
		inputs[i] = input;
		i++;
		if (i > INPUT_OVERFLOW) {
			xil_printf("\r\nSTOP IT!!\r\n");
			return;
		}
	}
	if (i == SINGLE_DIGITS) {
		alien_no = to_digit(inputs[0]);
	} else if (i == TWO_DIGITS) {
		alien_no = 10 * to_digit(inputs[0]) + to_digit(inputs[1]);
	} else {
		xil_printf("INVALID NUMBER\r\n");
		return;
	}

	if (alien_no >= ALIENS) {
		xil_printf("INVALID NUMBER\r\n");
		return;
	}
	alien_life[alien_no] = 0;
	print_array();
}
void fire_tank_bullet(void) {
	if (tank_bullet_flag == 0) {
		tank_bullet_pos.x = tank_pos.x + TANK_BULL_X;
		tank_bullet_pos.y = tank_pos.y - TANK_BULL_Y;
		tank_bullet_flag = 1;
		xil_printf("TANK BULLET POSITION %d %d\r\n", tank_bullet_pos.x,
				tank_bullet_pos.y);
	} else {
		xil_printf("BULLET ALREADY FIRED\r\n");
	}
}
void fire_alien_missile(void) {
	u8 i = 0;
	u8 alien_shooter;
	for (i; i < MISSILES; i++) {
		if (!alien_missile_flag[i]) {
			alien_missile_flag[i] = 1;
			break;
		}
	}
	if (i == MISSILES) {
		xil_printf("MISSILES ALREADY FIRED\r\n");
		return;
	}
	u8 fired = 0;
	while (!fired) {
		alien_shooter = rand() % ALIEN_COLS + BOT_LEFT_ALIEN;
		if (alien_life[alien_shooter])
			fired = 1;
	}
	alien_missiles[i].x = alien_block_pos.x + (alien_shooter % ALIEN_COLS)
			* ALIEN_WIDTH + ALIEN_MID;
	alien_missiles[i].y = alien_block_pos.y;
	alien_missiles[i].type = WEAK;

	xil_printf("MISSILE %d FIRED AT %d %d\r\n", i, alien_missiles[i].x,
			alien_missiles[i].y);
}
void update_bullets(void) {
	if (tank_bullet_flag) {
		tank_bullet_pos.y = tank_bullet_pos.y - MOVE_SPRITE;
		if (tank_bullet_pos.y == 0) {
			tank_bullet_flag = 0;
		}
		xil_printf("TANK BULLET POSITION %d %d\r\n", tank_bullet_pos.x,
				tank_bullet_pos.y);
	}
	u8 i;
	for (i = 0; i < MISSILES; i++) {
		if (alien_missile_flag[i]) {
			alien_missiles[i].y = alien_missiles[i].y + MOVE_SPRITE;
			if (alien_missiles[i].y == GAME_HEIGHT) {
				alien_missile_flag[i] = 0;
			}
			xil_printf("ALIEN MISSILE POSITION %d %d\r\n", alien_missiles[i].x,
					alien_missiles[i].y);
		}
	}

}
void erode_bunker(void) {
	xil_printf("PLEASE SPECIFY A NUMBER 0-3\r\n");
	char input;
	input = getchar();
	xil_printf("%c\r\n", input);
	u8 bunker_no = to_digit(input);
	if (bunker_no >= BUNKERS) {
		xil_printf("INVALID NUMBER\r\n");
		return;
	}
	if (bunker_states[bunker_no] != 0)
		bunker_states[bunker_no] = bunker_states[bunker_no]--;
	xil_printf("ERODED BUNKER %d to state %d\r\n", bunker_no,
			bunker_states[bunker_no]);
	print_bunkers();
}
