/*
 * render.h
 *
 *  Created on: Sep 28, 2017
 *      Author: superman
 */

#ifndef RENDER_H_
#define RENDER_H_

#include "xil_types.h"

typedef struct point_t {
	s16 x;
	s16 y;
} point_t;

typedef struct alien_bullet_t{
	s16 x;
	s16 y;
	enum {
		WEAK, NORMAL, STRONG
	} type;
} alien_bullet_t;

//Function prototypes
void render(point_t* tankPos, point_t* tankBulletPos, point_t* alienBlockPos,
		alien_bullet_t* alienBullets, u16* bunkerStates);

void render_init();

#endif /* RENDER_H_ */
