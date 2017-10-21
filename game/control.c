/*
 * control.c
 *
 *  Created on: Oct 4, 2017
 *      Author: superman
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "control.h"
#include "render.h"
#include "table.h"
#include "gpio.h"

#define check(byte,bit) (byte & (table_bit[bit])) //checks to see if there is overlap in the alien bit table
#define set(byte,bit) (byte |= (table_bit[bit])) //sets an alien in the bit table
#define clear(byte,bit) (byte &= ~(table_bit[bit])) //macro for eliminating an alien from the bit table
#define BULLET_POS -1 //initial bullet position
#define MOVE_SPRITE 2 //increment at which sprites move
#define TANK_MOVE_DIST 1
#define OFFSET_MAX 10 //max offset for deleting columns
#define BIT_SHIFT 1

#define ALIEN_MID (BMP_ALIEN_W/2)		//midpoint of an alien
#define ALIEN_ROW_ALIVE 0x7ff
#define TANK_BULL_Y 7		//bullet y offset for tank
#define	TANK_BULL_X (19/2)-1 //bullet x offset for tank
#define LIFE_COUNT 3 //initial tank lives
//
//Local typedefs


//Prototypes
void init_bunker_states(void); //function that sets the bunker states to max health
void tank_state_switch(void); //state machine for tank movement and shooting
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
saucer_t saucer;
bunker_t bunkers[GAME_BUNKER_COUNT]; //array of 4 bunker states
alien_missiles_t alien_missiles[GAME_MISSILE_COUNT]; //array of 4 alien missiles
//initialize the tank with a position and a bullet
tank_t tank;
//initialize the alien block with a position and flags
alien_block_t alien_block;
u32 game_score = 0;

//starts up the game and initializes the key components
void control_init(void) {

	srand(time(0)); //random seed

	//initialize the bunker states to full health
	init_bunker_states();

	s16 i; //initialize the alien life/death array
	for (i = 0; i < GAME_ALIEN_ROWS; i++) {
		alien_block.alien_status[i] = ALIEN_ROW_ALIVE; //set all aliens to alive
	}
	alien_block.legs = OUT;
	alien_block.loffset = 0;
	alien_block.roffset = 0;
	alien_block.pos.x = GAME_ALIEN_STARTX;
	alien_block.pos.y = GAME_ALIEN_STARTY;

	saucer.pos.x = GAME_SAUCER_STARTX;
	saucer.pos.y = GAME_SAUCER_Y;

	tank.changed = 1;
	tank.lives = GAME_LIFE_COUNT;
	tank.pos.x = GAME_TANK_STARTX;
	tank.pos.y = GAME_TANK_STARTY;
	tank.state = ALIVE;
	tank.missile.active = 0;


	//render(&tank, &alien_block, &alien_missiles, bunkers, &saucer, game_score); //render the sprites
}

//function that blocks on the user input and goes to correct function handler
void control_run(void) {

	tank_state_switch();
	render(&tank, &alien_block, &alien_missiles, bunkers, &saucer, game_score); //render after button press
}

#define EMPTY_BLOCK1 9
#define EMPTY_BLOCK2 10

//function that sets the bunker states to max health
void init_bunker_states(void) {
	u8 i; //iterates through all the bunkers
	u8 j;
	for (i = 0; i < GAME_BUNKER_COUNT; i++) {
		bunkers[i].alive = 1; //set state to max
		bunkers[i].changed = 1;
		for (j = 0; j < GAME_BUNKER_BLOCK_COUNT; j++) {
			if (j == EMPTY_BLOCK1 || j == EMPTY_BLOCK2)
				continue;
			bunkers[i].block[j].block_health = GAME_BUNKER_MAX;
			bunkers[i].block[j].changed = 1;
		}
	}
}

//function that moves the tank
void control_tank_move(direction_t dir) {
	if (tank.state != ALIVE)
		return;
	//if the tank is moving left
	if (dir == LEFT) {
		//check to see if it has hit the edge
		if (tank.pos.x >= TANK_MOVE_DIST) {
			//update the tank position to the left
			tank.pos.x -= TANK_MOVE_DIST;
			tank.changed = 1;
		}
	} else if (dir == RIGHT) { //check to see if the tank is moving right
		if (tank.pos.x <= GAME_W - BMP_TANK_W - TANK_MOVE_DIST) { //check to see if tank is moving off edge
			tank.pos.x += TANK_MOVE_DIST; // move to the right
			tank.changed = 1;
		}
	}
}

#define RIGHT_WALL (GAME_W - GAME_ALIEN_COLS*GAME_ALIEN_SEPX)
//function that updates the alien block position
void control_update_alien_position(void) {
	static direction_t dir = GAME_ALIEN_DIR;

	if (dir == LEFT) { //if aliens are moving left
		if (alien_block.pos.x + alien_block.loffset >= MOVE_SPRITE) { //and they haven't run off the screen
			alien_block.pos.x -= MOVE_SPRITE; //move the block by 2 pixels
		} else { //otherwise
			alien_block.pos.y += MOVE_SPRITE; //move the block down by 2 pixels
			dir = RIGHT; //and change the direction to right
		}

	} else if (dir == RIGHT) { //if aliens are moving right
		if (alien_block.pos.x - alien_block.roffset < RIGHT_WALL) { //and they haven't run into the wall
			alien_block.pos.x += MOVE_SPRITE; //move the block by 2 pixels
		} else { //otherwise
			alien_block.pos.y += MOVE_SPRITE; //move the block down by 2 pixels
			dir = LEFT; // and change the direction to the left
		}
	}
	if (alien_block.legs == OUT) //alternate between the aliens with legs out
		alien_block.legs = IN; //and legs in
	else
		//if the legs are already in then shift back to out
		alien_block.legs = OUT;
	alien_block.changed = 1;
}

//function that queries the user for an alien to kill, then wipes it out
void control_kill_alien(u16 alien_row, u16 alien_col) {
	u8 i = 0; //keep track of length of input

	//remove the alien from the alien bit table
	clear(alien_block.alien_status[alien_row],alien_col);


	//takes all the row status bits and ORs them together to find offset
	u16 mask = 0;
	for (i = 0; i < GAME_ALIEN_ROWS; i++) {
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
	alien_block.loffset = i * GAME_ALIEN_SEPX; //calculate the left offset
	alien_block.roffset = n * GAME_ALIEN_SEPX; //calculate the right offset
	alien_block.changed = 1;
}
//function that fires the tank bullet
void control_tank_fire(void) {
	static u8 fired = 0;
	//check to see if the tank bullet is already active
	if (fired == 0) {
		//place the tank bullet at the correct x position
		tank.missile.pos.x = tank.pos.x + TANK_BULL_X;
		//place the bullet at the top of the tank
		tank.missile.pos.y = tank.pos.y - TANK_BULL_Y;
		//set the bullet to active
		tank.missile.active = 1;
		//kill_saucer();
		//update_tank_lives();
		fired = 1;
	} else {
		if (tank.missile.active == 0)
			fired = 0;
	}

}
//function that fires the alien missiles
void control_alien_fire_missile(void) {
	u8 i;
	//used to identify the column of the shooter
	u8 shooter_col;
	//iterate over the missiles to check if they are active
	for (i = 0; i < CONTROL_MISSILES; i++) {
		//if the next alien missile is not active
		if (!alien_missiles[i].active) {
			//activate it
			alien_missiles[i].active = 1;
			//stop checking the missiles
			break;
		}
	}
	//if all missiles were checked and all were active
	if (i == CONTROL_MISSILES) {
		//exit the function
		return;
	}
	//flag to check if a missile was fired
	u8 fired = 0;
	//track the shooter's row
	u8 shooter_row = GAME_ALIEN_ROWS;
	//until a missile has been fired
	while (!fired) {
		//pick a random column to shoot
		shooter_col = rand() % GAME_ALIEN_COLS;
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
	alien_missiles[i].pos.x = alien_block.pos.x + shooter_col * BMP_ALIEN_W
			+ ALIEN_MID;
	//update the alien missile y position
	alien_missiles[i].pos.y = alien_block.pos.y + (shooter_row + 1)
			* GAME_ALIEN_SEPY - ALIEN_MID;


	//about once every 4 shots shoot a strong alien missile
	if (rand() % CONTROL_MISSILES == 0)
		//set the alien missile type to strong
		alien_missiles[i].type = STRONG;
	else
		//else set the alien missile type to normal
		alien_missiles[i].type = NORMAL;
	alien_missiles[i].type = GUISE_0;
}

#define RES_SCALE 2
#define OFFLIMITS 10
//function that updates the position of all bullets, both alien and tank
void control_update_bullet(void) {
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
			control_kill_alien(alien_row, alien_col);
			tank.missile.active = 0;
		}

		if (detect_saucer_collision(tank.missile.pos, bmp_missile_dim,
				bmp_bullet_straight_3x5)) {
			tank.missile.active = 0;
			saucer.active = 0;
			//control_saucer_explode();

		}

	}
}

void control_update_missiles(void) {
	//iterate through all the alien missiles
	u8 i;
	for (i = 0; i < CONTROL_MISSILES; i++) {
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
//
void control_saucer_move(void) {
	static direction_t saucer_dir = LEFT; //start the saucer going left
	if (saucer_dir == LEFT) { //if the saucer is moving left
		if (saucer.pos.x > NEG_SAUCER_WIDTH) //move as long as not off screen
			saucer.pos.x -= MOVE_SPRITE; //decrement x position
		else {
			saucer.active = 0; //if off screen then deactivate
			saucer_dir = RIGHT; //change saucer direction
		}
	} else if (saucer_dir == RIGHT) { //if the saucer is moving right
		if (saucer.pos.x < GAME_W + SAUCER_WIDTH) //move until off the screen
			saucer.pos.x += MOVE_SPRITE; //increment position
		else {
			saucer.active = 0; //deactivate
			saucer_dir = LEFT; //change saucer direction
		}
	}
}

//return the saucer state
u8 control_saucer_state(void) {
	return saucer.active;
}
//change the saucer state
void control_saucer_state_toggle(void) {
	saucer.active = !(saucer.active);

}

void update_tank_lives(void) {

	tank.state = EXPLODE1;
	tank.changed = 1;
	tank.lives--;

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
	point_t alien_dim;
	alien_dim.x = GAME_ALIEN_COLS * GAME_ALIEN_SEPX;
	alien_dim.y = GAME_ALIEN_ROWS * GAME_ALIEN_SEPY;
	if (!collision_detect(pos, dim, alien_block.pos, alien_dim))
		return false;

	point_t rel_pos;
	rel_pos.x = abs(pos.x - alien_block.pos.x);
	rel_pos.y = abs(pos.y - alien_block.pos.y);
	u16 col = rel_pos.x / GAME_ALIEN_SEPX;
	u16 row = rel_pos.y / GAME_ALIEN_SEPY;
	if (!check(alien_block.alien_status[row],col))
		return false;
	point_t alien_pos;
	alien_pos.x = alien_block.pos.x + GAME_ALIEN_SEPX * col;
	alien_pos.y = alien_block.pos.y + GAME_ALIEN_SEPY * row;

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

	point_t bunker_pos;
	bunker_pos.x = GAME_BUNKER_POS;
	bunker_pos.y = GAME_BUNKER_Y;
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

