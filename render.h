/*
 * render.h
 *
 *  Created on: Sep 28, 2017
 *      Author: superman
 */

#ifndef RENDER_H_
#define RENDER_H_

#include "xil_types.h"
#include "game.h"
#include "bmp.h"

#define SCREEN_H 480
#define SCREEN_W 640
#define GAME_H (SCREEN_H/2)
#define GAME_W (SCREEN_W/2)

#define GAME_BUNKER_COUNT 4
#define GAME_BUNKER_POS (GAME_W/8-BMP_BUNKER_W/2)
#define GAME_BUNKER_SEP (GAME_W/4)
#define GAME_BUNKER_Y ((GAME_H*3)/4-BMP_BUNKER_H/2)

#define RENDER_TANK_Y (GAME_H*7/8)
#define RENDER_TANK_X (GAME_W/2-BMP_TANK_W/2)

//Function prototypes
void render_init();
void render(tank_t* tank, alien_block_t* alienBlock,
		alien_missiles_t* alienBullets, u16* bunkerStates);

#endif /* RENDER_H_ */
