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
#define BUNKER_BLOCK_COUNT 10
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
#define TANK_X (GAME_WIDTH/2 - BMP_TANK_W/2) //starting x location of the tank
#define TANK_Y GAME_HEIGHT*7/8	//starting y location of the tank
#define TANK_BULL_Y 7		//bullet y offset for tank
#define	TANK_BULL_X (19/2)-1 //bullet x offset for tank
typedef enum {
	READY, SHOOT, MOVE_LEFT, MOVE_RIGHT
} t_state;

void move_tank(direction); //function that moves the tank
void init_bunker_states(void); //function that sets the bunker states to max health
void tank_state_switch(void); //state machine for tank movement and shooting
void fire_tank_bullet(void); //function that fires the tank bullet
u16 detect_collision(u16 x, u16 y);
bunker_t bunkers[BUNKER_COUNT]; //array of 4 bunker states
t_state tank_state;
alien_missiles_t alien_missiles[GAME_CONTROLLER_MISSILES]; //array of 4 alien missiles
//initialize the tank with a position and a bullet
tank_t tank = { { TANK_X, TANK_Y }, { { BULLET_POS, BULLET_POS }, 0 } };
//initialize the alien block with a position and flags
alien_block_t block = { { ALIEN_X, ALIEN_Y }, 0, 0, { 0 }, OUT };

//starts up the game and initializes the key components
void game_controller_init(void) {

	//initialize the bunker states to full health
	init_bunker_states();
	//initialize the alien render timer
	u32 init_timer = ALIEN_INIT;

	s16 i; //initialize the alien life/death array
	for (i = 0; i < ALIEN_ROWS; i++) {
		block.alien_status[i] = ALIEN_ROW_ALIVE; //set all aliens to alive
	}
	render(&tank, &block, &alien_missiles, bunkers); //render the sprites
	for (; i < INITIAL_MOVES; i++) { //render six times
		while (init_timer) //cycle through the timer
			init_timer--; // Decrement the timer.
		init_timer = ALIEN_INIT; // Reset the timer.
		block.pos.x += MOVE_SPRITE; //move the sprite by 2
		render(&tank, &block, &alien_missiles, bunkers); // render the sprites
	}
	srand(time(0)); //random seed
	tank_state = READY;
}

#define LEFT_BTN 0x08	//bit mask for left button
#define SHOOT_BTN 0x01	//bit mask for shoot button
#define RIGHT_BTN 0x02	//bit mask for right button
direction tank_dir;
//function that blocks on the user input and goes to correct function handler
void game_controller_run(void) {

	tank_state_switch();

	/*
	 //switch statement for handling different keyboard presses
	 switch (input) {
	 case KEY_2:	//case for key 2
	 kill_alien();	//ask the user for an alien to kill
	 break;
	 case KEY_3:	//case for key 3
	 //randomly pick an alien on the bottom row and fire a missile
	 fire_alien_missile();
	 break;
	 case KEY_4: //case for key 4
	 move_tank(LEFT); //move the tank to the left
	 break;
	 case KEY_5: //case for key 5
	 //fire a tank bullet at the tank's current position
	 break;
	 case KEY_6: //case for key 6
	 move_tank(RIGHT); //move the tank to the right
	 break;
	 case KEY_7: //case for key 7
	 erode_bunker(); //prompt the user for a bunker to erode
	 break;
	 case KEY_8: //case for key 8
	 //move the alien block from left to right and right to left
	 update_alien_position();
	 break;
	 case KEY_9:	//case for key 9
	 //move the alien missiles down and the tank bullets up
	 update_bullets();
	 break;
	 default: //case for an invalid button press
	 break;
	 }
	 */
	render(&tank, &block, &alien_missiles, bunkers); //render after button press
}

//state machine for tank movement and shooting
void tank_state_switch(void) {
	switch (tank_state) {
	case READY:

		break;
	case SHOOT:
		fire_tank_bullet();
		tank_state = READY;
		break;

	case MOVE_LEFT:
		move_tank(LEFT);
		break;
	case MOVE_RIGHT:
		move_tank(RIGHT);
		break;

	}
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
}

//function that sets the bunker states to max health
void init_bunker_states(void) {
	u8 i; //iterates through all the bunkers
	u8 j;
	for (i = 0; i < BUNKER_COUNT; i++) {
		bunkers[i].alive = 1; //set state to max
		for (j = 0; j < BUNKER_BLOCK_COUNT; j++) {
			bunkers[i].block[j].block_health = BUNKER_MAX;
		}
	}
}

direction alien_direction = RIGHT; //global variable that tracks direction of alien block

//function that moves the tank
void move_tank(direction d) {
	//if the tank is moving left
	if (d == LEFT) {
		//check to see if it has hit the edge
		if (tank.pos.x >= MOVE_SPRITE)
			//update the tank position to the left
			tank.pos.x -= MOVE_SPRITE;
	} else if (d == RIGHT) { //check to see if the tank is moving right
		if (tank.pos.x <= GAME_WIDTH - BMP_TANK_W - MOVE_SPRITE) //check to see if tank is moving off edge
			tank.pos.x += MOVE_SPRITE; // move to the right
	}
}

//function that updates the alien block position
void game_controller_update_alien_position(void) {

	if (alien_direction == LEFT) { //if aliens are moving left
		if (block.pos.x + block.loffset >= MOVE_SPRITE) { //and they haven't run off the screen
			block.pos.x -= MOVE_SPRITE; //move the block by 2 pixels
		} else { //otherwise
			block.pos.y += MOVE_SPRITE; //move the block down by 2 pixels
			alien_direction = RIGHT; //and change the direction to right
		}

	} else if (alien_direction == RIGHT) { //if aliens are moving right
		if (block.pos.x - block.roffset < RIGHT_WALL) { //and they haven't run into the wall
			block.pos.x += MOVE_SPRITE; //move the block by 2 pixels
		} else { //otherwise
			block.pos.y += MOVE_SPRITE; //move the block down by 2 pixels
			alien_direction = LEFT; // and change the direction to the left
		}
	}
	if (block.legs == OUT) //alternate between the aliens with legs out
		block.legs = IN; //and legs in
	else
		//if the legs are already in then shift back to out
		block.legs = OUT;
}

//function that queries the user for an alien to kill, then wipes it out
void game_controller_kill_alien(void) {
	char inputs[BUFFER]; //create an input buffer
	u8 i = 0; //keep track of length of input
	u8 alien_no; //integer used for indexing
	char input;
	while (input != ENTER) {//until enter has been pressed
		input = getchar(); //read in the users input
		inputs[i] = input; //store it int the buffer
		i++; //increment length of input
		if (i > INPUT_OVERFLOW) { //if the number is too long
			return; //quit the function
		}
	}
	if (i == SINGLE_DIGITS) {//if it is a single digit number
		alien_no = to_digit(inputs[FIRST_INPUT]); //convert from a char to an integer
	} else if (i == TWO_DIGITS) {//if the input is two digits
		//multiply the first number by ten and add the second
		alien_no = TENS_DIGIT * to_digit(inputs[FIRST_INPUT])
				+ to_digit(inputs[SECOND_INPUT]);
	} else {
		return; //if the number is longer than two digits then quit
	}
	//if the alien number is larger than the block quit
	if (alien_no >= ALIEN_ROWS * ALIEN_ROW_LEN) {
		return;
	}
	//remove the alien from the alien bit table
	clear(block.alien_status[alien_no/ALIEN_ROW_LEN],alien_no%ALIEN_ROW_LEN);

	//takes all the row status bits and ORs them together to find offset
	u16 mask = 0;
	for (i = 0; i < ALIEN_ROWS; i++) {
		mask |= block.alien_status[i]; //OR together the statuses
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
	block.loffset = i * ALIEN_SEP; //calculate the left offset
	block.roffset = n * ALIEN_SEP; //calculate the right offset

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
		while (!check(block.alien_status[shooter_row],shooter_col))
			shooter_row--;
		//if the shooter was in the bottom row
		if (shooter_row != 0) {
			//show that a missile was fired
			fired = 1;
		}
	}
	//update the alien missile x position
	alien_missiles[i].pos.x = block.pos.x + shooter_col * ALIEN_WIDTH
			+ ALIEN_MID;
	//update the alien missile y position
	alien_missiles[i].pos.y = block.pos.y + (shooter_row + 1) * ALIEN_SEP
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
#define OFFLIMITS 25
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
		if (render_detect_collision(bmp_bullet_straight_3x5,
				tank.missile.pos.x, tank.missile.pos.y, BMP_BULLET_H)) {
			print("Hit!");
			tank.missile.active = 0;
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
		}
	}
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
