/*
 * bmp.h
 *
 *  Created on: Sep 28, 2017
 *      Author: superman
 */

#ifndef BMP_H_
#define BMP_H_

#include "xil_types.h"

#define BMP_BUNKER_H 18
#define BMP_BUNKER_W 24

#define BMP_TANK_H 8
#define BMP_TANK_W 19
#define BMP_MID BMP_TANK_W/2

#define BMP_ALIEN_H 8
#define BMP_ALIEN_W 18

#define BMP_BUNKER_STATES 4
#define BMP_EROSION_H 6

#define BMP_BULLET_H 5
#define BMP_BULLET_W 3
#define BMP_BULLET_OFF 5
#define BMP_MISSILES 4

//Function prototypes

//Externs

// Must define packword for each of the different bit-widths.
extern const u32* bmp_aliens_out[];
extern const u32* bmp_aliens_in[];
extern const u32** bmp_aliens[];

extern const u32 bmp_saucer_16x7[];
extern const u32 bmp_alien_explosion_12x10[];
extern const u32 bmp_alien_top_in_12x8[];
extern const u32 bmp_alien_top_out_12x8[];
extern const u32 bmp_alien_middle_in_12x8[];
extern const u32 bmp_alien_middle_out_12x8[];
extern const u32 bmp_alien_bottom_in_12x8[];
extern const u32 bmp_alien_bottom_out_12x8[];
extern const u32 bmp_alien_empty[];

extern const u32 bmp_tank_15x8[];

extern const u32 bmp_alien_missile_cross1_3x5[];
extern const u32 bmp_alien_missile_cross2_3x5[];
extern const u32 bmp_alien_missile_diagonal1_3x5[];
extern const u32 bmp_alien_missile_diagonal2_3x5[];
extern const u32 bmp_bullet_straight_3x5[];
extern const u32** bmp_alien_missiles[];

// Shape of the entire bunker.
extern const u32 bmp_bunker_24x18[];

// These are the blocks that comprise the bunker and each time a bullet
// strikes one of these blocks, you erode the block as you sequence through
// these patterns.
extern const u32 bmp_bunkerDamage0_6x6[];
extern const u32 bmp_bunkerDamage1_6x6[];
extern const u32 bmp_bunkerDamage2_6x6[];
extern const u32 bmp_bunkerDamage3_6x6[];
extern const u32 bmp_bunkerDamage4_6x6[];

#endif /* BMP_H_ */
