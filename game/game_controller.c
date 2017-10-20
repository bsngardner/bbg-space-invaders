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
#include "render.h"
#include "table.h"
#include "gpio.h"

#define check(byte,bit) (byte & (table_bit[bit])) //checks to see if there is overlap in the alien bit table
#define set(byte,bit) (byte |= (table_bit[bit])) //sets an alien in the bit table
#define clear(byte,bit) (byte &= ~(table_bit[bit])) //macro for eliminating an alien from the bit table
#define to_digit(c) (c-'0')	//converts a char to integer
#define TENS_DIGIT 10	//factor for multiplying the tens digit
#define FIRST_INPUT 0	//first index in the input buffer
#define SECOND_INPUT 1 	//second index in the input buffer
#define BUFFER 20		//length of the input buffer
#define ENTER '\r'		//ENTER key stroke
#define INPUT_OVERFLOW 4 //limit the length of the number entered
#define	TWO_DIGITS 3	//location of the tens digit in the buffer
#define	SINGLE_DIGITS 2 //location of the singles digit in the buffer
#define KEY_2 '2'	//delete alien case
#define KEY_3 '3'	//fire alien missile case
#define KEY_4 '4'	//move tank left case
#define KEY_5 '5'	//fire tank bullet case
#define KEY_6 '6'	//move tank right case
#define KEY_7 '7'	//erode bunker case
#define KEY_8 '8'	//update alien block position case
#define KEY_9 '9'	//update bullet and missile position case
#define BULLET_POS -1 //initial bullet position
#define MOVE_SPRITE 2 //increment at which sprites move
#define OFFSET_MAX 10 //max offset for deleting columns
#define BIT_SHIFT 1

#define BUNKER_COUNT 4	//number of bunkers
#define BUNKER_MAX 4 //max health of bunkers
#define ALIEN_X GAME_WIDTH/6	//starting point for the x pos of the alien block
#define ALIEN_Y GAME_HEIGHT/8	//starting point for the y pos of the alien block
#define	ALIEN_COLS 11	//number of alien columns
#define ALIEN_WIDTH 18	//width of one alien
#define ALIEN_MID 8		//midpoint of an alien
#define ALIEN_INIT 100000; //timer for initializing the aiens
#define RIGHT_WALL 123  //position of the block when it hits the wall
#define INITIAL_MOVES 6 //number of times render is called to paint aliens for the first time
#define BLOCK_H 72		//height of the alien block
#define ALIEN_ROWS 5	//number of aliens rows
#define ALIEN_INDEX 4	//max index of the alien array
#define ALIEN_ROW_LEN 11 //number of alien columns
#define ALIEN_ROW_ALIVE 0x7ff
#define ALIEN_SEP 16	//pixels between tops of aliens
#define SCREEN_HEIGHT 480 //height of the screen
#define SCREEN_WIDTH 640 //width of the screen
#define GAME_HEIGHT (SCREEN_HEIGHT/2) //game height (half of screen)
#define GAME_WIDTH (SCREEN_WIDTH/2) //game width (half of screen)
#define TANK_BULL_Y 7		//bullet y offset for tank
#define	TANK_BULL_X (19/2)-1 //bullet x offset for tank
#define SAUCER_X 321 //starting x position
#define SAUCER_Y 15 //starting y position
#define LIFE_COUNT 3 //initial tank lives
//
//Local typedefs
enum {
	READY, SHOOT, MOVE_LEFT, MOVE_RIGHT
} tank_state = READY;

//Prototypes
void move_tank(direction); //function that moves the tank
void init_bunker_states(void); //function that sets the bunker states to max health
void tank_state_switch(void); //state machine for tank movement and shooting
void fire_tank_bullet(void); //function that fires the tank bullet
void update_tank_lives(void);
void kill_saucer(void);

static bool collision_detect(point_t pos1, point_t dim1, point_t pos2,
		point_t dim2);
static bool collision_detect_bmp(const u32* bmp1, point_t pos1, point_t dim1,
		const u32* bmp2, point_t pos2, point_t dim2, const u32* bmp_mask);
static bool detect_bunker_collision(u16* bunker_num, u16* block_num,
		point_t projectile_pos, const u32* bmp);
static bool detect_alien_collision(u16* alien_row, u16* alien_col, point_t pos,
		point_t dim, const u32* bmp);
static bool detect_tank_collision(point_t pos, point_t dim, const u32* bmp);
static bool detect_saucer_collision(point_t pos, point_t dim, const u32* bmp);
void resurrect_tank();

//Const

//Variables
saucer_t saucer = { { SAUCER_X, SAUCER_Y }, 0, 1 };
bunker_t bunkers[BUNKER_COUNT]; //array of 4 bunker states
alien_missiles_t alien_missiles[GAME_CONTROLLER_MISSILES]; //array of 4 alien missiles
//initialize the tank with a position and a bullet
tank_t tank = { .pos = { GAME_TANK_STARTX, GAME_TANK_STARTY }, .lives =
		GAME_LIFE_COUNT, .changed = 1, .state = ALIVE, .missile = { {
		BULLET_POS, BULLET_POS }, 0 } };
//initialize the alien block with a position and flags
alien_block_t alien_block = { { ALIEN_X, ALIEN_Y }, 0, 0, { 0 }, OUT };
u32 game_score = 0;

//starts up the game and initializes the key components
void game_controller_init(void) {

	//initialize the bunker states to full health
	init_bunker_states();
	//initialize the alien render timer
	u32 init_timer = ALIEN_INIT;

	s16 i; //initialize the alien life/death array
	for (i = 0; i < ALIEN_ROWS; i++) {
		alien_block.alien_status[i] = ALIEN_ROW_ALIVE; //set all aliens to alive
	}

	render(&tank, &alien_block, &alien_missiles, bunkers, &saucer, game_score); //render the sprites
	for (; i < INITIAL_MOVES; i++) { //render six times
		while (init_timer) //cycle through the timer
			init_timer--; // Decrement the timer.
		init_timer = ALIEN_INIT; // Reset the timer.
		alien_block.pos.x += MOVE_SPRITE; //move the sprite by 2
		render(&tank, &alien_block, &alien_missiles, bunkers, &saucer,
				game_score); // render the sprites
	}
	srand(time(0)); //random seed
	saucer.active = 1;
}

#define LEFT_BTN 0x08	//bit mask for left button
#define SHOOT_BTN 0x01	//bit mask for shoot button
#define RIGHT_BTN 0x02	//bit mask for right button
direction tank_dir;
//function that blocks on the user input and goes to correct function handler
void game_controller_run(void) {

	tank_state_switch();

	render(&tank, &alien_block, &alien_missiles, bunkers, &saucer, game_score); //render after button press
}

//state machine for tank movement and shooting
void tank_state_switch(void) {

	switch (tank_state) {
	case READY:
		if (button_state & SHOOT_BTN) {
			tank_state = SHOOT;
		} else if (button_state & LEFT_BTN) {
			tank_state = MOVE_LEFT;
		} else if (button_state & RIGHT_BTN) {
			tank_state = MOVE_RIGHT;
		}
		break;
	case SHOOT:
		fire_tank_bullet();
		tank_state = READY;
		break;
	case MOVE_LEFT:
		move_tank(LEFT);
		tank_state = READY;
		break;
	case MOVE_RIGHT:
		move_tank(RIGHT);
		tank_state = READY;
		break;

	}
	if (tank.state == GAME_OVER && button_state != 0) {
		tank.state = ALIVE;
		tank.changed = 1;
	}
#if 0
	if (gpio_button_flag) {
		gpio_button_flag = 0;
		if (button_state & LEFT_BTN) {
			tank_state = MOVE_LEFT;
		} else if (button_state & RIGHT_BTN) {
			tank_state = MOVE_RIGHT;
		} else if (button_state & SHOOT_BTN) {
			tank_state = SHOOT;
		} else {
			tank_state = READY;
		}
	}
#endif
}

#define EMPTY_BLOCK1 9
#define EMPTY_BLOCK2 10

//function that sets the bunker states to max health
void init_bunker_states(void) {
	u8 i; //iterates through all the bunkers
	u8 j;
	for (i = 0; i < BUNKER_COUNT; i++) {
		bunkers[i].alive = 1; //set state to max
		bunkers[i].changed = 1;
		for (j = 0; j < GAME_BUNKER_BLOCK_COUNT; j++) {
			if (j == EMPTY_BLOCK1 || j == EMPTY_BLOCK2)
				continue;
			bunkers[i].block[j].block_health = BUNKER_MAX;
			bunkers[i].block[j].changed = 1;
		}
	}
}

direction alien_direction = RIGHT; //global variable that tracks direction of alien block

//function that moves the tank
void move_tank(direction d) {
	if (tank.state != ALIVE)
		return;
	//if the tank is moving left
	if (d == LEFT) {
		//check to see if it has hit the edge
		if (tank.pos.x >= MOVE_SPRITE) {
			//update the tank position to the left
			tank.pos.x -= MOVE_SPRITE;
			tank.changed = 1;
		}
	} else if (d == RIGHT) { //check to see if the tank is moving right
		if (tank.pos.x <= GAME_WIDTH - BMP_TANK_W - MOVE_SPRITE) { //check to see if tank is moving off edge
			tank.pos.x += MOVE_SPRITE; // move to the right
			tank.changed = 1;
		}
	}
}

void game_controller_explode(u8 state) {
	if (state == EXPLODE_1) {
		render_explosion_1(tank.pos.x, tank.pos.y);
		tank.state = EXPLODE_2;
	} else if (state == EXPLODE_2) {
		render_explosion_2(tank.pos.x, tank.pos.y);
		tank.state = INIT;
	} else if (state == INIT) {
		render_explosion_3(tank.pos.x, tank.pos.y);
	}
}

u8 game_controller_tank_life(void) {
	return tank.state;
}

//function that updates the alien block position
void game_controller_update_alien_position(void) {

	if (alien_direction == LEFT) { //if aliens are moving left
		if (alien_block.pos.x + alien_block.loffset >= MOVE_SPRITE) { //and they haven't run off the screen
			alien_block.pos.x -= MOVE_SPRITE; //move the block by 2 pixels
		} else { //otherwise
			alien_block.pos.y += MOVE_SPRITE; //move the block down by 2 pixels
			alien_direction = RIGHT; //and change the direction to right
		}

	} else if (alien_direction == RIGHT) { //if aliens are moving right
		if (alien_block.pos.x - alien_block.roffset < RIGHT_WALL) { //and they haven't run into the wall
			alien_block.pos.x += MOVE_SPRITE; //move the block by 2 pixels
		} else { //otherwise
			alien_block.pos.y += MOVE_SPRITE; //move the block down by 2 pixels
			alien_direction = LEFT; // and change the direction to the left
		}
	}
	if (alien_block.legs == OUT) //alternate between the aliens with legs out
		alien_block.legs = IN; //and legs in
	else
		//if the legs are already in then shift back to out
		alien_block.legs = OUT;
}

//function that queries the user for an alien to kill, then wipes it out
void game_controller_kill_alien(u16 alien_row, u16 alien_col) {
	u8 i = 0; //keep track of length of input

	//remove the alien from the alien bit table
	clear(alien_block.alien_status[alien_row],alien_col);

	//takes all the row status bits and ORs them together to find offset
	u16 mask = 0;
	for (i = 0; i < ALIEN_ROWS; i++) {
		mask |= alien_block.alien_status[i]; //OR together the statuses
	}
	u16 maskp = mask; //init the mask
	u8 n = OFFSET_MAX; //init the max offset

	while (maskp >>= BIT_SHIFT) { //finding the right offset
		n--;
	}

	i = OFFSET_MAX; //init the max offset
	maskp = mask;
	while ((maskp = ((maskp << BIT_SHIFT) & ALIEN_ROW_ALIVE))) {//find the left offset
		i--;
	}
	alien_block.loffset = i * ALIEN_SEP; //calculate the left offset
	alien_block.roffset = n * ALIEN_SEP; //calculate the right offset

}
//function that fires the tank bullet
void fire_tank_bullet(void) {
	//check to see if the tank bullet is already active
	if (tank.missile.active == 0) {
		//place the tank bullet at the correct x position
		tank.missile.pos.x = tank.pos.x + TANK_BULL_X;
		//place the bullet at the top of the tank
		tank.missile.pos.y = tank.pos.y - TANK_BULL_Y;
		//set the bullet to active
		tank.missile.active = 1;
		//kill_saucer();
		//update_tank_lives();
	}

}
//function that fires the alien missiles
void game_controller_fire_alien_missile(void) {
	u8 i;
	//used to identify the column of the shooter
	u8 shooter_col;
	//iterate over the missiles to check if they are active
	for (i = 0; i < GAME_CONTROLLER_MISSILES; i++) {
		//if the next alien missile is not active
		if (!alien_missiles[i].active) {
			//activate it
			alien_missiles[i].active = 1;
			//stop checking the missiles
			break;
		}
	}
	//if all missiles were checked and all were active
	if (i == GAME_CONTROLLER_MISSILES) {
		//exit the function
		return;
	}
	//flag to check if a missile was fired
	u8 fired = 0;
	//track the shooter's row
	u8 shooter_row = ALIEN_INDEX;
	//until a missile has been fired
	while (!fired) {
		//pick a random column to shoot
		shooter_col = rand() % ALIEN_COLS;
		//check to see if the shooter is in the bottom row
		while (!check(alien_block.alien_status[shooter_row],shooter_col))
			shooter_row--;
		//if the shooter was in the bottom row
		if (shooter_row != 0) {
			//show that a missile was fired
			fired = 1;
		}
	}
	//update the alien missile x position
	alien_missiles[i].pos.x = alien_block.pos.x + shooter_col * ALIEN_WIDTH
			+ ALIEN_MID;
	//update the alien missile y position
	alien_missiles[i].pos.y = alien_block.pos.y + (shooter_row + 1) * ALIEN_SEP
			- ALIEN_MID;

	//about once every 4 shots shoot a strong alien missile
	if (rand() % GAME_CONTROLLER_MISSILES == 0)
		//set the alien missile type to strong
		alien_missiles[i].type = STRONG;
	else
		//else set the alien missile type to normal
		alien_missiles[i].type = NORMAL;
}

#define RES_SCALE 2
#define OFFLIMITS 10
//function that updates the position of all bullets, both alien and tank
void game_controller_update_bullet(void) {
	//if there is already an active tank bullet
	if (tank.missile.active) {
		//update its y position by 2 pixels
		tank.missile.pos.y = tank.missile.pos.y - MOVE_SPRITE;
		//if the tank bullet leaves the screen
		if (tank.missile.pos.y < OFFLIMITS) {
			//allow for another active bullet
			tank.missile.active = 0;
		}
		u16 bunker_num;
		u16 block_num;
		if (detect_bunker_collision(&bunker_num, &block_num, tank.missile.pos,
				bmp_bullet_straight_3x5)) {

			tank.missile.active = 0;
			bunkers[bunker_num].block[block_num].block_health--;
			bunkers[bunker_num].block[block_num].changed = 1;
			bunkers[bunker_num].changed = 1;
		}

		u16 alien_row;
		u16 alien_col;
		if (detect_alien_collision(&alien_row, &alien_col, tank.missile.pos,
				bmp_missile_dim, bmp_bullet_straight_3x5)) {
			game_controller_kill_alien(alien_row, alien_col);
			tank.missile.active = 0;
		}

		if (detect_saucer_collision(tank.missile.pos, bmp_missile_dim,
				bmp_bullet_straight_3x5)) {
			tank.missile.active = 0;
			saucer.active = 0;
			saucer.alive = 0;
			game_controller_saucer_explode();

		}

	}
}

void game_controller_update_missiles(void) {
	//iterate through all the alien missiles
	u8 i;
	for (i = 0; i < GAME_CONTROLLER_MISSILES; i++) {
		//check whether the alien missile is active
		if (alien_missiles[i].active) {
			//move the alien missile position up by 2 pixels
			alien_missiles[i].pos.y = alien_missiles[i].pos.y + MOVE_SPRITE;
			//check the guise of the alien missile
			if (alien_missiles[i].guise == GUISE_1)
				//alternate the guise every movement
				alien_missiles[i].guise = GUISE_0;
			else
				//change the guise
				alien_missiles[i].guise = GUISE_1;
			//if the alien missile exits the screen, allow it to be fired again
			if (alien_missiles[i].pos.y >= 220) {
				//allow for another alien missile
				alien_missiles[i].active = 0;
			}
			u16 bunker_num;
			u16 block_num;
			if (detect_bunker_collision(
					&bunker_num,
					&block_num,
					alien_missiles[i].pos,
					bmp_alien_missiles[alien_missiles[i].type][alien_missiles[i].guise])) {

				alien_missiles[i].active = 0;
				bunkers[bunker_num].block[block_num].block_health--;
				bunkers[bunker_num].block[block_num].changed = 1;
				bunkers[bunker_num].changed = 1;
			} else if (detect_tank_collision(
					alien_missiles[i].pos,
					bmp_missile_dim,
					bmp_alien_missiles[alien_missiles[i].type][alien_missiles[i].guise])) {
				alien_missiles[i].active = 0;
				update_tank_lives();

			}
		}
	}
}
#define SAUCER_WIDTH 20
#define NEG_SAUCER_WIDTH -20 //saucer can go off screen
direction saucer_dir = LEFT; //start the saucer going left
void game_controller_move_saucer(void) {
	if (saucer_dir == LEFT) { //if the saucer is moving left
		if (saucer.pos.x > NEG_SAUCER_WIDTH) //move as long as not off screen
			saucer.pos.x -= MOVE_SPRITE; //decrement x position
		else {
			saucer.active = 0; //if off screen then deactivate
			saucer_dir = RIGHT; //change saucer direction
		}
	} else if (saucer_dir == RIGHT) { //if the saucer is moving right
		if (saucer.pos.x < GAME_WIDTH + SAUCER_WIDTH) //move until off the screen
			saucer.pos.x += MOVE_SPRITE; //increment position
		else {
			saucer.active = 0; //deactivate
			saucer_dir = LEFT; //change saucer direction
		}
	}
}

void kill_saucer() {
	saucer.alive = 0;
	saucer.active = 0;
}

#define MAX_RAND 9
#define MIN_SCORE 50
void game_controller_saucer_explode(void) {
	if (!saucer.alive) {
		render_saucer_death(saucer.pos.x, saucer.pos.y);
		u32 points = ((rand() % MAX_RAND) * MIN_SCORE) + MIN_SCORE;
		game_score += points;
		saucer.alive = 1;
		saucer.active = 0;
		saucer.pos.x = SAUCER_X;
		saucer.pos.y = SAUCER_Y;
		render_saucer();
	}
}

u8 game_controller_saucer_life(void) {
	return saucer.alive;
}
//return the saucer state
u8 game_controller_saucer_state(void) {
	return saucer.active;
}
//change the saucer state
void game_controller_saucer_state_toggle(void) {
	saucer.active = !(saucer.active);

}

void update_tank_lives(void) {

	tank.state = EXPLODE;
	tank.changed = 1;
	tank.lives--;

}

//function that updates (erodes) the states of the bunkers
void erode_bunker(void) {
	//waits for user input
	char input;
	input = getchar();
	//converts the keyboard input to an integer
	u8 bunker_no = to_digit(input);
	//if the bunker number is out of bounds then skip the erosion
	if (bunker_no >= BUNKER_COUNT) {
		return;
	}
	//if the bunker state is not already zero
	if (bunkers[bunker_no].alive != 0)
		//erode the bunker in the bunker state array
		bunkers[bunker_no].alive--;
}

static bool detect_saucer_collision(point_t pos, point_t dim, const u32* bmp) {
	if (collision_detect_bmp(bmp, pos, dim, bmp_saucer_16x7, saucer.pos,
			bmp_saucer_dim, 0))
		return true;
	return false;
}

static bool detect_tank_collision(point_t pos, point_t dim, const u32* bmp) {
	if (collision_detect_bmp(bmp, pos, dim, bmp_tank_15x8, tank.pos,
			bmp_tank_dim, 0)) {
		return true;
	}
	return false;
}

static bool detect_alien_collision(u16* alien_row, u16* alien_col, point_t pos,
		point_t dim, const u32* bmp) {
	point_t alien_dim = { .x = GAME_ALIEN_COLS * ALIENS_SEPX, .y =
			GAME_ALIEN_ROWS * ALIENS_SEPY };
	if (!collision_detect(pos, dim, alien_block.pos, alien_dim))
		return false;

	point_t rel_pos;
	rel_pos.x = abs(pos.x - alien_block.pos.x);
	rel_pos.y = abs(pos.y - alien_block.pos.y);
	u16 col = rel_pos.x / ALIENS_SEPX;
	u16 row = rel_pos.y / ALIENS_SEPY;
	if (!check(alien_block.alien_status[row],col))
		return false;
	point_t alien_pos = { .x = alien_block.pos.x + ALIENS_SEPX * col, .y =
			alien_block.pos.y + ALIENS_SEPY * row };
	alien_dim.x = BMP_ALIEN_W;
	alien_dim.y = BMP_ALIEN_H;
	if (collision_detect_bmp(bmp, pos, dim, bmp_aliens[alien_block.legs][row],
			alien_pos, alien_dim, 0)) {

		*alien_row = row;
		*alien_col = col;
		return true;
	}
	return false;
}

static bool detect_bunker_collision(u16* bunker_num, u16* block_num,
		point_t projectile_pos, const u32* bmp) {

	point_t bunker_pos = { .x = GAME_BUNKER_POS, .y = GAME_BUNKER_Y };
	point_t block_pos;
	//Iterate through bunkers, check for collision with each
	for (u16 i = 0; i < GAME_BUNKER_COUNT; i++) {
		//If collision with bunker detected
		if (collision_detect(projectile_pos, bmp_missile_dim, bunker_pos,
				bmp_bunker_dim)) {
			//Iterate through blocks in bunker; once collision has been detected, return
			for (u16 j = 0; j < GAME_BUNKER_BLOCK_COUNT; j++) {
				//If block is already dead, dont check
				if (bunkers[i].block[j].block_health == 0)
					continue;

				block_pos.x = bunker_pos.x + BMP_BUNKER_BLOCK_W * (j
						% GAME_BUNKER_WIDTH);
				block_pos.y = bunker_pos.y + BMP_BUNKER_BLOCK_H * (j
						/ GAME_BUNKER_WIDTH);
				if (collision_detect_bmp(bmp, projectile_pos, bmp_missile_dim,
						bmp_bunker_blocks[j], block_pos, bmp_bunker_block_dim,
						bmp_bunker_damages[bunkers[i].block[j].block_health])) {
					*block_num = j;
					*bunker_num = i;
					return true;
				}
			}

		}
		bunker_pos.x += GAME_BUNKER_SEP;
	}

	return false;
}

static bool collision_detect(point_t pos1, point_t dim1, point_t pos2,
		point_t dim2) {

	s16 top1 = pos1.y;
	s16 bottom2 = pos2.y + dim2.y;
	if (top1 > bottom2) //first is below second
		return false;
	s16 bottom1 = pos1.y + dim1.y;
	s16 top2 = pos2.y;
	if (top2 > bottom1) //first is above second
		return false;
	s16 left1 = pos1.x;
	s16 right2 = pos2.x + dim2.x;
	if (left1 > right2) //first is right of second
		return false;
	s16 right1 = pos1.x + dim1.x;
	s16 left2 = pos2.x;
	if (left2 > right1) //first is left of second
		return false;
	return true;
}

static bool collision_detect_bmp(const u32* bmp1, point_t pos1, point_t dim1,
		const u32* bmp2, point_t pos2, point_t dim2, const u32* bmp_mask) {
	if (!collision_detect(pos1, dim1, pos2, dim2))
		return false;

	u16 shift1 = 0;
	u16 shift2 = 0;
	u16 y1 = 0;
	u16 y2 = 0;
	u16 count;
	u32 bmp_row1;
	u32 bmp_row2;

	if (pos1.x < pos2.x) {
		shift1 = pos2.x - pos1.x;
	} else {
		shift2 = pos1.x - pos2.x;
	}
	if (pos1.y < pos2.y) {
		y1 = pos2.y - pos1.y;
		count = dim1.y - y1;
	} else {
		y2 = pos1.y - pos2.y;
		count = dim2.y - y2;
	}

	while (count-- > 0) {
		bmp_row1 = bmp1[y1];
		bmp_row2 = bmp2[y2];
		if (bmp_mask)
			bmp_row2 &= bmp_mask[y2];
		if ((bmp_row1 >> shift1) & (bmp_row2 >> shift2))
			return true;
	}
	return false;
}

