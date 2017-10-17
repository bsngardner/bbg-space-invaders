/*
 * render.h
 *
 * Render module handles screen drawing and updates
 *
 *  Created on: Sep 28, 2017
 *      Author:  Broderick Gardner
 *      		Benjamin Gardner
 */

#ifndef RENDER_H_
#define RENDER_H_

#include "xil_types.h"
#include "game.h"
#include "bmp.h"

#define RENDER_TANK_Y (GAME_H*7/8)
#define RENDER_TANK_X (GAME_W/2-BMP_TANK_W/2)

//Function prototypes
void render_init();
void render(tank_t* tank, alien_block_t* alienBlock,
		alien_missiles_t* alienBullets, u16* bunkerStates);

#endif /* RENDER_H_ */
