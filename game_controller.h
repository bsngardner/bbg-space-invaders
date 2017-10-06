/*
 * game_controller.h
 *
 *  Created on: Oct 4, 2017
 *      Author: superman
 */

#ifndef GAME_CONTROLLER_H_
#define GAME_CONTROLLER_H_

#include "bmp.h"

#define ALIEN_ROWS 5
#define ALIEN_ROW_LEN 11
#define ALIEN_ROW_ALIVE 0x7ff
#define ALIEN_SEP 16
#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 640
#define GAME_HEIGHT (SCREEN_HEIGHT/2)
#define GAME_WIDTH (SCREEN_WIDTH/2)

#define TANK_X (GAME_WIDTH/2 - BMP_TANK_W/2)
#define TANK_Y GAME_HEIGHT*7/8
#define TANK_BULL_Y 7
#define	TANK_BULL_X (19/2)-1

#define ALIEN_X GAME_WIDTH/6
#define ALIEN_Y GAME_HEIGHT/8
#define	ALIEN_COLS 11
#define BOT_LEFT_ALIEN 44
#define ALIEN_WIDTH 18
#define ALIEN_MID 8
#define MISSILES 4
#define ALIEN_INIT 100000;
#define RIGHT_WALL 123
#define INITIAL_MOVES 6
#define BLOCK_H 72

#define BUNKERS 4
#define BUNKER_MAX 4

#define MOVE_SPRITE 2

#define BUFFER 20
#define ENTER '\r'
#define INPUT_OVERFLOW 4
#define	TWO_DIGITS 3
#define	SINGLE_DIGITS 2

#define KEY_2 '2'
#define KEY_3 '3'
#define KEY_4 '4'
#define KEY_5 '5'
#define KEY_6 '6'
#define KEY_7 '7'
#define KEY_8 '8'
#define KEY_9 '9'
#define to_digit(c) (c-'0')

typedef enum {
	LEFT, RIGHT
} direction;

#include "xil_types.h"
#include "game.h"
#include "render.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void game_controller_init(void);
void game_controller_run(void);

void move_tank(direction);
void update_alien_position(void);
void kill_alien(void);
void fire_tank_bullet(void);
void fire_alien_missile(void);
void update_bullets(void);
void erode_bunker(void);
void print_array(void);
void init_bunker_states(void);
void print_bunkers(void);

#endif /* GAME_CONTROLLER_H_ */
