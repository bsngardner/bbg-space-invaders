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
#include <stdbool.h>
#include "game.h"
#include "bmp.h"

#define RENDER_TANK_Y (GAME_H*7/8)
#define RENDER_TANK_X (GAME_W/2-BMP_TANK_W/2)

//Function prototypes
void render_init();
void render(tank_t* tank, alien_block_t* alienBlock,
		alien_missiles_t* alien_missiles, bunker_t* bunkers, saucer_t* saucer,
		u32 score);

void render_game_over(void);
void render_explosion_1(u16 x, u16 y);
void render_explosion_2(u16 x, u16 y);
void render_explosion_3(u16 x, u16 y);
void render_saucer_death(u16 x, u16 y);
void render_saucer_points(u16 x, u16 y, u16 points, u8 on);
void render_tank(void);
void render_saucer(void);

#endif /* RENDER_H_ */
