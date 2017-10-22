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

//Game screen defines
#define GAME_SCREEN_H 480
#define GAME_SCREEN_W 640
#define GAME_H (GAME_SCREEN_H/2) //game height
#define GAME_W (GAME_SCREEN_W/2) //game width
#define GAME_BACKGROUND COLOR_BLACK
//
//Alien defines
#define GAME_ALIEN_STARTX (GAME_W/10)
#define GAME_ALIEN_STARTY (GAME_H/6)
#define GAME_ALIEN_SEPX (BMP_ALIEN_W)	//Horizontal separation between aliens
#define GAME_ALIEN_SEPY (BMP_ALIEN_W-2)	//Separation between aliens vertically
#define GAME_ALIEN_DROP 2
#define GAME_ALIEN_ROWS 5 //number of alien rows
#define GAME_ALIEN_COLS 11
#define GAME_ALIEN_DIR RIGHT
#define GAME_MISSILE_COUNT 4
#define GAME_INIT_ROW_COL -1

//Saucer defines
#define GAME_SAUCER_STARTX 400
#define GAME_SAUCER_Y 20

//Bunker defines
#define GAME_BUNKER_COUNT 4 //number of bunkers
#define GAME_BUNKER_BLOCK_COUNT 12 //number of bunker blocks per bunker
#define GAME_BUNKER_POS (GAME_W/8-BMP_BUNKER_W/2) //location of bunkers
#define GAME_BUNKER_SEP (GAME_W/4) //separation of the bunkers
#define GAME_BUNKER_Y ((GAME_H*3)/4-BMP_BUNKER_H/2) //y position of the bunkers
#define GAME_BUNKER_WIDTH 4
#define GAME_BUNKER_MAX 4

//Tank defines
#define GAME_TANK_STARTX (GAME_W/2 - BMP_TANK_W/2)
#define GAME_TANK_STARTY (GAME_H*5/6)
#define GAME_LIFE_COUNT 3

//
//Struct for coordinate point
typedef union {
	struct {
		s16 x;
		s16 y;
	};
	s32 xy;
} point_t;

//Tank struct, contains missile
typedef struct {
	point_t pos;
	u8 lives;
	u8 changed;
	enum {
		EMPTY = 0, ALIVE = 1, EXPLODE1 = 2, EXPLODE2 = 3
	} state;
	struct {
		point_t pos;
		u8 active;
	} missile;
} tank_t;

typedef struct {
	point_t pos;
	u8 active;
	u8 alive;
	u16 points;
} saucer_t;

//Alien missile type
typedef struct {
	point_t pos;
	u16 active :1;
	enum {
		GUISE_0 = 0, GUISE_1 = 1
	//guise of the alien bullet
	} guise;
	enum {
		NORMAL = 0, STRONG = 1
	} type;
} alien_missiles_t;

//Alien block
typedef struct alien_block {
	point_t pos;
	u8 loffset;
	u8 roffset;
	s8 changed;
	s8 alive;
	u16 alien_status[GAME_ALIEN_ROWS];
	point_t row_col;
	enum {
		OUT = 0, IN = 1
	} legs;
} alien_block_t;

typedef struct {
	u8 changed;
	u8 alive;
	struct {
		u8 changed;
		u8 block_health;
	} block[GAME_BUNKER_BLOCK_COUNT];
} bunker_t;

#endif /* GAME_H_ */
