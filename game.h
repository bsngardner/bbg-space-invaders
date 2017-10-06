/*
 * game.h
 *
 *  Created on: Oct 4, 2017
 *      Author: superman
 */

#ifndef GAME_H_
#define GAME_H_

typedef struct {
	s16 x;
	s16 y;
} point_t;

typedef struct {
	point_t pos;
	struct {
		point_t pos;
		u8 active;
	} missile;
} tank_t;

typedef struct {
	point_t pos;
	u16 active :1;
	u16 state :2;
	enum {
		NORMAL = 0, STRONG = 1
	} type;
} alien_missiles_t;

typedef struct alien_block {
	point_t pos;
	s8 loffset;
	s8 roffset;
	u16 alien_status[5];
	enum {
		OUT = 0, IN = 1
	} legs;
} alien_block_t;

#endif /* GAME_H_ */
