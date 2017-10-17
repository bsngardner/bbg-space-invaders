/*
 * game.h
 *
 * Contains typedefs and defines for the game
 *
 *  Created on: Oct 4, 2017
 *      Author:  Broderick Gardner
 *      		Benjamin Gardner
 */

#ifndef GAME_H_
#define GAME_H_

#define ALIEN_ROWS 5 //number of alien rows

#define GAME_H (SCREEN_H/2) //game height
#define GAME_W (SCREEN_W/2) //game width

#define GAME_BUNKER_COUNT 4 //number of bunkers
#define GAME_BUNKER_POS (GAME_W/8-BMP_BUNKER_W/2) //location of bunkers
#define GAME_BUNKER_SEP (GAME_W/4) //separation of the bunkers
#define GAME_BUNKER_Y ((GAME_H*3)/4-BMP_BUNKER_H/2) //y position of the bunkers

//Struct for coordinate point
typedef struct {
	s16 x;
	s16 y;
} point_t;

//Tank struct, contains missile
typedef struct {
	point_t pos;
	struct {
		point_t pos;
		u8 active;
	} missile;
} tank_t;

//Alien missile type
typedef struct {
	point_t pos;
	u16 active :1;
	enum {
		GUISE_0=0, GUISE_1=1 //guise of the alien bullet
	} guise;
	enum {
		NORMAL = 0, STRONG = 1
	} type;
} alien_missiles_t;

//Alien block
typedef struct alien_block {
	point_t pos;
	s8 loffset;
	s8 roffset;
	u16 alien_status[ALIEN_ROWS];
	enum {
		OUT = 0, IN = 1
	} legs;
} alien_block_t;

#endif /* GAME_H_ */