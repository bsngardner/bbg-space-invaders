/*
 * bmp.c
 *
 *  Created on: Sep 28, 2017
 *      Author: superman
 */

#include <stdint.h>
#include "bmp.h"

//Macros
//Utilities for packing bits into bitmaps

#define packword32(b31,b30,b29,b28,b27,b26,b25,b24,b23,b22,b21,b20,b19,b18,b17,b16,b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1,b0) ( \
(b31 << 31) | (b30 << 30) | (b29 << 29) | (b28 << 28) | (b27 << 27) | (b26 << 26) | (b25 << 25) | (b24 << 24) |						  \
 (b23 << 23) | (b22 << 22) | (b21 << 21) | (b20 << 20) | (b19 << 19) | (b18 << 18) | (b17 << 17) | (b16 << 16) |						  \
 (b15 << 15) | (b14 << 14) | (b13 << 13) | (b12 << 12) | (b11 << 11) | (b10 << 10) | (b9  << 9 ) | (b8  << 8 ) |						  \
 (b7  << 7 ) | (b6  << 6 ) | (b5  << 5 ) | (b4  << 4 ) | (b3  << 3 ) | (b2  << 2 ) | (b1  << 1 ) | (b0  << 0 )							\
 )

#define packword24(b23,b22,b21,b20,b19,b18,b17,b16,b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1,b0)	(	\
(b23 << 23) | (b22 << 22) | (b21 << 21) | (b20 << 20) | (b19 << 19) | (b18 << 18) | (b17 << 17) | (b16 << 16) |						  \
 (b15 << 15) | (b14 << 14) | (b13 << 13) | (b12 << 12) | (b11 << 11) | (b10 << 10) | (b9  << 9 ) | (b8  << 8 ) |						  \
 (b7  << 7 ) | (b6  << 6 ) | (b5  << 5 ) | (b4  << 4 ) | (b3  << 3 ) | (b2  << 2 ) | (b1  << 1 ) | (b0  << 0 )							\
 )

#define packword16(b15,b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1,b0) (\
 ((b15 << 15) | (b14 << 14) | (b13 << 13) | (b12 << 12) | (b11 << 11) | (b10 << 10) | (b9  << 9 ) | (b8  << 8 ) |						  \
 (b7  << 7 ) | (b6  << 6 ) | (b5  << 5 ) | (b4  << 4 ) | (b3  << 3 ) | (b2  << 2 ) | (b1  << 1 ) | (b0  << 0 ) 	\
 )<< 4)

#define packword15(b14,b13,b12,b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1,b0) ((		\
(b14 << 14) | (b13 << 13) | (b12 << 12) | (b11 << 11) | (b10 << 10) | (b9  << 9 ) | (b8  << 8 ) |						  \
 (b7  << 7 ) | (b6  << 6 ) | (b5  << 5 ) | (b4  << 4 ) | (b3  << 3 ) | (b2  << 2 ) | (b1  << 1 ) | (b0  << 0 ) 	\
 ) << 2)	//Padding
//Modified to pad bitmap!
#define packword12(b11,b10,b9,b8,b7,b6,b5,b4,b3,b2,b1,b0)	((	\
(b11 << 11) | (b10 << 10) | (b9  << 9 ) | (b8  << 8 ) |						  \
 (b7  << 7 ) | (b6  << 6 ) | (b5  << 5 ) | (b4  << 4 ) | (b3  << 3 ) | (b2  << 2 ) | (b1  << 1 ) | (b0  << 0 ) 	\
 ) << 3)	//Padding
#define packword6(b5,b4,b3,b2,b1,b0)	(	\
(b5  << 5 ) | (b4  << 4 ) | (b3  << 3 ) | (b2  << 2 ) | (b1  << 1 ) | (b0  << 0 ) 	\
 )

#define packword4(b3,b2,b1,b0) ( \
(b3 << 3) | (b2 << 2) | (b1 << 1) | (b0 << 0)	)

#define packword3(b2,b1,b0) ( \
(b2 << 2) | (b1 << 1) | (b0 << 0) \
)

//define packword 27, for the words score and lives.
#define packword27(b26, b25, b24, b23, b22, b21, b20, b19, b18, b17, b16, b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0)\
		((b26 << 26) |(b25 << 25) |(b24 << 24) |(b23 << 23) | (b22 << 22) | (b21 << 21) | (b20 << 20) | (b19 << 19) | (b18 << 18) | (b17  << 17 ) | (b16  << 16 ) |\
				(b15 << 15) | (b14 << 14) | (b13 << 13) | (b12 << 12) | (b11 << 11) | (b10 << 10) | (b9  << 9 ) | (b8  << 8 ) |\
				(b7  << 7 ) | (b6  << 6 ) | (b5  << 5 ) | (b4  << 4 ) | (b3  << 3 ) | (b2  << 2 ) | (b1  << 1 ) | (b0  << 0 ) )

// Must define packword for each of the different bit-widths.
const u32 bmp_saucer_16x7[] = { packword16(0, 0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 0, 0, 0, 0, 0), packword16(0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0), packword16(0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0),
		packword16(0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0), packword16(
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), packword16(0,
				0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0), packword16(0, 0,
				0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0) };

//Alien explosion
const u32 bmp_alien_explosion_12x10[] = { packword12(0, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0), packword12(0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0),
		packword12(1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0), packword12(0, 1, 0, 0,
				1, 0, 0, 0, 1, 0, 0, 0), packword12(0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 1, 1), packword12(1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
		packword12(0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0), packword12(0, 0, 1, 0,
				0, 0, 0, 0, 1, 0, 0, 1), packword12(0, 1, 0, 0, 0, 1, 0, 0, 1,
				0, 0, 0), packword12(0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0) };

//Alien top in
const u32 bmp_alien_top_in_12x8[] = { packword12(0, 0, 0, 0, 0, 1, 1,
		0, 0, 0, 0, 0), packword12(0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0),
		packword12(0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0), packword12(0, 0, 1, 1,
				0, 1, 1, 0, 1, 1, 0, 0), packword12(0, 0, 1, 1, 1, 1, 1, 1, 1,
				1, 0, 0), packword12(0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0),
		packword12(0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0), packword12(0, 0, 0, 1,
				0, 0, 0, 0, 1, 0, 0, 0) };

//Alien top out
const u32 bmp_alien_top_out_12x8[] = { packword12(0, 0, 0, 0, 0, 1, 1,
		0, 0, 0, 0, 0), packword12(0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0),
		packword12(0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0), packword12(0, 0, 1, 1,
				0, 1, 1, 0, 1, 1, 0, 0), packword12(0, 0, 1, 1, 1, 1, 1, 1, 1,
				1, 0, 0), packword12(0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0),
		packword12(0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0), packword12(0, 0, 1, 0,
				1, 0, 0, 1, 0, 1, 0, 0) };

//Alien middle in
const u32 bmp_alien_middle_in_12x8[] = { packword12(0, 0, 0, 1, 0, 0,
		0, 0, 0, 1, 0, 0), packword12(0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0),
		packword12(0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0), packword12(0, 0, 1, 1,
				0, 1, 1, 1, 0, 1, 1, 0), packword12(0, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1), packword12(0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
		packword12(0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1), packword12(0, 0, 0, 0,
				1, 1, 0, 1, 1, 0, 0, 0) };

//Alien middle out
const u32 bmp_alien_middle_out_12x8[] = { packword12(0, 0, 0, 1, 0, 0,
		0, 0, 0, 1, 0, 0), packword12(0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1),
		packword12(0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1), packword12(0, 1, 1, 1,
				0, 1, 1, 1, 0, 1, 1, 1), packword12(0, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1), packword12(0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0),
		packword12(0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0), packword12(0, 0, 1, 0,
				0, 0, 0, 0, 0, 0, 1, 0) };

//Alien bottom in
const u32 bmp_alien_bottom_in_12x8[] = { packword12(0, 0, 0, 0, 1, 1,
		1, 1, 0, 0, 0, 0), packword12(0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0),
		packword12(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), packword12(1, 1, 1, 0,
				0, 1, 1, 0, 0, 1, 1, 1), packword12(1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1), packword12(0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0),
		packword12(0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0), packword12(0, 0, 1, 1,
				0, 0, 0, 0, 1, 1, 0, 0) };

//Alien bottom out
const u32 bmp_alien_bottom_out_12x8[] = { packword12(0, 0, 0, 0, 1, 1,
		1, 1, 0, 0, 0, 0), packword12(0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0),
		packword12(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), packword12(1, 1, 1, 0,
				0, 1, 1, 0, 0, 1, 1, 1), packword12(1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1), packword12(0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0),
		packword12(0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0), packword12(1, 1, 0, 0,
				0, 0, 0, 0, 0, 0, 1, 1) };

//Empty alien
const u32 bmp_alien_empty[8] = { 0 };

//Lookup tables for easy bitmap selection
const u32* bmp_aliens_out[] = { bmp_alien_top_out_12x8,
		bmp_alien_middle_out_12x8, bmp_alien_middle_out_12x8,
		bmp_alien_bottom_out_12x8, bmp_alien_bottom_out_12x8 };
const u32* bmp_aliens_in[] = { bmp_alien_top_in_12x8, bmp_alien_middle_in_12x8,
		bmp_alien_middle_in_12x8, bmp_alien_bottom_in_12x8,
		bmp_alien_bottom_in_12x8 };

const u32** bmp_aliens[] = { bmp_aliens_out, bmp_aliens_in };

//Tank bitmap
const u32 bmp_tank_15x8[] = { packword15(0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0),
		packword15(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0), packword15(0,
				0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0), packword15(0, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0), packword15(1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1), packword15(1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1), packword15(1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1), packword15(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1) };

// Shape of the entire bunker.
const u32 bmp_bunker_24x18[] = { packword24(0, 0, 0, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0), packword24(0, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0), packword24(0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0),
		packword24(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1), packword24(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), packword24(1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
		packword24(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1), packword24(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), packword24(1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
		packword24(1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
				1, 1, 1, 1), packword24(1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
				0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1), packword24(1, 1, 1, 1, 1, 1,
				1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1),
		packword24(1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
				1, 1, 1, 1), packword24(1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1), packword24(1, 1, 1, 1, 1, 1,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1),
		packword24(1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
				1, 1, 1, 1), packword24(1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1), packword24(1, 1, 1, 1, 1, 1,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1) };

const u32 bmp_bunker0_6x6[] = { packword6(0,0,0,1,1,1), packword6(0,0,1,1,1,1),
		packword6(0,1,1,1,1,1), packword6(1,1,1,1,1,1), packword6(1,1,1,1,1,1),
		packword6(1,1,1,1,1,1) };
const u32 bmp_bunker1_6x6[] = { packword6(1,1,1,1,1,1), packword6(1,1,1,1,1,1),
		packword6(1,1,1,1,1,1), packword6(1,1,1,1,1,1), packword6(1,1,1,1,1,1),
		packword6(1,1,1,1,1,1) };
const u32 bmp_bunker2_6x6[] = { packword6(1,1,1,0,0,0), packword6(1,1,1,1,0,0),
		packword6(1,1,1,1,1,0), packword6(1,1,1,1,1,1), packword6(1,1,1,1,1,1),
		packword6(1,1,1,1,1,1) };
const u32 bmp_bunker3_6x6[] = { packword6(1,1,1,1,1,1), packword6(1,1,1,1,0,0),
		packword6(1,1,1,0,0,0), packword6(1,1,0,0,0,0), packword6(1,0,0,0,0,0),
		packword6(0,0,0,0,0,0) };
const u32 bmp_bunker4_6x6[] = { packword6(0,0,0,0,0,0), packword6(0,0,0,0,0,0),
		packword6(0,0,0,0,0,0), packword6(0,0,0,0,0,0), packword6(0,0,0,0,0,0),
		packword6(0,0,0,0,0,0) };
const u32 bmp_bunker5_6x6[] = { packword6(1,1,1,1,1,1), packword6(0,0,1,1,1,1),
		packword6(0,0,0,1,1,1), packword6(0,0,0,0,1,1), packword6(0,0,0,0,0,1),
		packword6(0,0,0,0,0,0) };

const u32* bmp_bunker_blocks[] = { bmp_bunker2_6x6, bmp_bunker1_6x6,
		bmp_bunker1_6x6, bmp_bunker0_6x6, bmp_bunker1_6x6, bmp_bunker5_6x6,
		bmp_bunker3_6x6, bmp_bunker1_6x6, bmp_bunker1_6x6, bmp_bunker4_6x6,
		bmp_bunker4_6x6, bmp_bunker1_6x6 };

// These are the blocks that comprise the bunker and each time a bullet
// strikes one of these blocks, you erode the block as you sequence through
// these patterns.
const u32 bmp_bunkerDamage0_6x6[] = { packword6(0,0,0,0,0,0),
		packword6(0,0,0,0,0,0), packword6(0,0,0,0,0,0), packword6(0,0,0,0,0,0),
		packword6(0,0,0,0,0,0), packword6(0,0,0,0,0,0) };
const u32 bmp_bunkerDamage1_6x6[] = { packword6(0, 1, 1, 0, 0, 0),
		packword6(0, 0, 0, 0, 0, 1), packword6(1, 1, 0, 1, 0, 0), packword6(1,
				0, 0, 0, 0, 0), packword6(0, 0, 1, 1, 0, 0), packword6(0, 0, 0,
				0, 1, 0) };
const u32 bmp_bunkerDamage2_6x6[] = { packword6(1, 1, 1, 0, 1, 0),
		packword6(1, 0, 1, 0, 0, 1), packword6(1, 1, 0, 1, 1, 1), packword6(1,
				0, 0, 0, 0, 0), packword6(0, 1, 1, 1, 0, 1), packword6(0, 1, 1,
				0, 1, 0) };
const u32 bmp_bunkerDamage3_6x6[] = { packword6(1, 1, 1, 1, 1, 1),
		packword6(1, 0, 1, 1, 0, 1), packword6(1, 1, 0, 1, 1, 1), packword6(1,
				1, 0, 1, 1, 0), packword6(0, 1, 1, 1, 0, 1), packword6(1, 1, 1,
				1, 1, 1) };
const u32 bmp_bunkerDamage4_6x6[] = { packword6(1, 1, 1, 1, 1, 1),
		packword6(1, 1, 1, 1, 1, 1), packword6(1, 1, 1, 1, 1, 1), packword6(1,
				1, 1, 1, 1, 1), packword6(1, 1, 1, 1, 1, 1), packword6(1, 1, 1,
				1, 1, 1) };
const u32* bmp_bunker_damages[] = { bmp_bunkerDamage0_6x6,
		bmp_bunkerDamage1_6x6, bmp_bunkerDamage2_6x6, bmp_bunkerDamage3_6x6,
		bmp_bunkerDamage4_6x6 };

//Missile bitmaps
const u32
		bmp_alien_missile_cross2_3x5[] =
				{ packword3(0, 1, 0), packword3(0, 1,
						0), packword3(0, 1, 0), packword3(1, 1, 1),
						packword3(0, 1, 0) };
const u32
		bmp_alien_missile_cross1_3x5[] =
				{ packword3(0, 1, 0), packword3(1, 1,
						1), packword3(0, 1, 0), packword3(0, 1, 0),
						packword3(0, 1, 0) };
const u32
		bmp_alien_missile_diagonal1_3x5[] = { packword3(1, 0, 0),
				packword3(0, 1,
						0), packword3(0, 0, 1), packword3(0, 1, 0),
				packword3(1, 0, 0) };
const u32
		bmp_alien_missile_diagonal2_3x5[] = { packword3(0, 0, 1),
				packword3(0, 1,
						0), packword3(1, 0, 0), packword3(0, 1, 0),
				packword3(0, 0, 1) };
const u32* bmp_alien_missiles_cross[] = { bmp_alien_missile_cross2_3x5,
		bmp_alien_missile_cross1_3x5 };
const u32* bmp_alien_missiles_diagonal[] = { bmp_alien_missile_diagonal1_3x5,
		bmp_alien_missile_diagonal2_3x5 };
const u32** bmp_alien_missiles[] = { bmp_alien_missiles_cross,
		bmp_alien_missiles_diagonal };

const u32 bmp_bullet_straight_3x5[] = { packword3(0, 1, 0), packword3(0, 1,
		0), packword3(0, 1, 0), packword3(0, 1, 0), packword3(0, 1, 0),
		packword3(0, 1, 0) };

const u32 number_one_4x7[] = {
		packword4(0,0,1,0),
		packword4(0,0,1,0),
		packword4(0,0,1,0),
		packword4(0,0,1,0),
		packword4(0,0,1,0),
		packword4(0,0,1,0),
		packword4(0,0,1,0)
};
const u32 number_two_4x7[] = {
		packword4(0,1,1,0),
		packword4(1,0,0,1),
		packword4(1,0,0,0),
		packword4(1,1,1,0),
		packword4(0,0,0,1),
		packword4(0,0,0,1),
		packword4(1,1,1,1)
};
const u32 number_three_4x7[] = {
		packword4(0,1,1,0),
		packword4(1,0,0,1),
		packword4(1,0,0,0),
		packword4(1,1,1,0),
		packword4(1,0,0,0),
		packword4(1,0,0,1),
		packword4(0,1,1,0)
};
const u32 number_four_4x7[] = {
		packword4(1,0,0,1),
		packword4(1,0,0,1),
		packword4(1,0,0,1),
		packword4(1,1,1,1),
		packword4(1,0,0,0),
		packword4(1,0,0,0),
		packword4(1,0,0,0)
};
const u32 number_five_4x7[] = {
		packword4(1,1,1,1),
		packword4(0,0,0,1),
		packword4(0,0,0,1),
		packword4(0,1,1,1),
		packword4(1,0,0,0),
		packword4(1,0,0,1),
		packword4(0,1,1,0)
};
const u32 number_six_4x7[] = {
		packword4(0,1,1,0),
		packword4(0,0,0,1),
		packword4(0,0,0,1),
		packword4(0,1,1,1),
		packword4(1,0,0,1),
		packword4(1,0,0,1),
		packword4(0,1,1,0)
};
const u32 number_seven_4x7[] = {
		packword4(1,1,1,1),
		packword4(1,0,0,0),
		packword4(1,0,0,0),
		packword4(1,0,0,0),
		packword4(1,0,0,0),
		packword4(1,0,0,0),
		packword4(1,0,0,0)
};
const u32 number_eight_4x7[] = {
		packword4(0,1,1,0),
		packword4(1,0,0,1),
		packword4(1,0,0,1),
		packword4(1,1,1,1),
		packword4(1,0,0,1),
		packword4(1,0,0,1),
		packword4(0,1,1,0)
};
const u32 number_nine_4x7[] = {
		packword4(0,1,1,0),
		packword4(1,0,0,1),
		packword4(1,0,0,1),
		packword4(1,1,1,1),
		packword4(1,0,0,0),
		packword4(1,0,0,0),
		packword4(0,1,1,0)
};

const u32 number_zero_4x7[] = {
		packword4(0,1,1,0),
		packword4(1,0,0,1),
		packword4(1,0,0,1),
		packword4(1,0,0,1),
		packword4(1,0,0,1),
		packword4(1,0,0,1),
		packword4(0,1,1,0)
};

#define NUMBERS 10
const u32* bmp_numbers[NUMBERS] = { number_zero_4x7, number_one_4x7, number_two_4x7,
		number_three_4x7, number_four_4x7, number_five_4x7, number_six_4x7,
		number_seven_4x7, number_eight_4x7, number_nine_4x7 };
// This is for the drawing of the alien saucer sprite.
const u32 saucer_16x7[] = { packword16(0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0),
		packword16(0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0),
		packword16(0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0),
		packword16(0,1,1,0,1,1,0,1,1,0,1,1,0,1,1,0),
		packword16(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1),
		packword16(0,0,1,1,1,0,0,1,1,0,0,1,1,1,0,0),
		packword16(0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0) };

//This is for drawing the words
const u32 word_lives_27x8[] = {
		packword27(0,1,1,1,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,1,0,0),
		packword27(0,0,0,0,1,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,1,0,0),
		packword27(0,0,0,0,1,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,1,0,0),
		packword27(0,0,1,1,0,0,0,1,1,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0,1,0,0),
		packword27(0,1,0,0,0,0,0,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,0,1,0,0),
		packword27(0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,0,1,0,0),
		packword27(0,0,1,1,1,0,1,1,1,1,0,0,0,0,1,0,0,0,0,1,0,1,1,1,1,0,0),
		packword27(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0) };

const u32 word_score_27x8[] = {
		packword27(1,1,1,1,0,0,1,1,1,0,1,1,1,1,0,1,1,1,0,0,1,1,1,0,0,0,0),
		packword27(0,0,0,1,0,1,0,0,1,0,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0),
		packword27(0,0,0,1,0,1,0,0,1,0,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0),
		packword27(0,1,1,1,0,0,1,1,1,0,1,0,0,1,0,0,0,0,1,0,0,1,1,0,0,0,0),
		packword27(0,0,0,1,0,1,0,0,1,0,1,0,0,1,0,0,0,0,1,0,1,0,0,0,0,0,0),
		packword27(0,0,0,1,0,1,0,0,1,0,1,0,0,1,0,0,0,0,1,0,1,0,0,0,0,0,0),
		packword27(1,1,1,1,0,1,0,0,1,0,1,1,1,1,0,1,1,1,0,0,0,1,1,1,0,0,0),
		packword27(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0) };

const u32 word_game_27x8[]  = {
		packword27(1,1,1,1,0,1,0,0,0,0,0,1,0,0,1,1,1,0,0,0,1,1,1,1,0,0,0),
		packword27(0,0,0,1,0,1,1,0,0,0,1,1,0,1,0,0,0,1,0,0,0,0,0,0,1,0,0),
		packword27(0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,0,0,0,0,0,1,0,0),
		packword27(0,1,1,1,0,1,0,0,1,0,0,1,0,1,1,1,1,1,0,1,1,1,1,0,1,0,0),
		packword27(0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,0,1,0,0),
		packword27(0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,0,1,0,0),
		packword27(1,1,1,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0,0,1,1,1,1,0,0,0),
		packword27(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0) };

const u32 word_over_27x8[] = {
		packword27(0,1,1,1,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,1,1,1,0,0,0,0),
		packword27(1,0,0,1,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0,0,0,0),
		packword27(1,0,0,1,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0,0,0,0),
		packword27(0,1,1,1,0,0,1,1,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0,0,0,0),
		packword27(1,0,0,1,0,0,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,0,0,0,0),
		packword27(1,0,0,1,0,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0,0),
		packword27(1,0,0,1,0,1,1,1,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1,0,0,0,0),
		packword27(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0) };


