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
	s16 x;
	s16 y;
	enum {
		WEAK, NORMAL, STRONG
	} type;
} alien_bullet_t;

#define ALIEN_COUNT 55
typedef struct alien_block {
	point_t pos;
	u8 alien_status[ALIEN_COUNT];
	enum {
		OUT = 0, IN = 1
	} legs;
} alien_block_t;

#endif /* GAME_H_ */
