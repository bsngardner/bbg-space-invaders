/*
 * game_controller.h
 *
 *  Created on: Oct 4, 2017
 *      Author: superman
 */

#ifndef GAME_CONTROLLER_H_
#define GAME_CONTROLLER_H_
#define ALIENS 55
#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 640
#define GAME_HEIGHT (SCREEN_HEIGHT/2)
#define GAME_WIDTH (SCREEN_WIDTH/2)

#define TANK_X (GAME_WIDTH/2)
#define TANK_Y GAME_HEIGHT*7/8
#define TANK_BULL_Y 10
#define	TANK_BULL_X 8

#define ALIEN_X (GAME_WIDTH/2)-40
#define ALIEN_Y (GAME_HEIGHT/2)
#define	ALIEN_COLS 11
#define BOT_LEFT_ALIEN 44
#define ALIEN_WIDTH 8
#define ALIEN_MID 4
#define MISSILES 4


#define BUNKERS 4
#define BUNKER_MAX 5

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

#include "xil_types.h"
#include "render.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void game_controller_init(void);
void init_array(void);
void game_controller_run(void);

typedef enum {LEFT, RIGHT} direction;

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
