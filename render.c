/*
 * render.c
 *
 *  Created on: Sep 28, 2017
 *      Author: superman
 */

#include <stdio.h>
#include "string.h"

#include "render.h"
#include "xparameters.h"
#include "xio.h"
#include "xil_types.h"

#include "vdma.h"
#include "bmp.h"

#define BIT0 0x01

#define COLOR_GREEN 0x0000FF00
#define COLOR_BLACK 0x00000000
#define COLOR_WHITE 0x00FFFFFF
#define GAME_BACKGROUND COLOR_BLACK
#define BUNKER_ROWS 3
#define BUNKER_COLS 4

#define FRAME_BUFFER_0_ADDR 0xC1000000  // Starting location in DDR where we will store the images that we display.
static u32* frame0 = (u32*) FRAME_BUFFER_0_ADDR;
static u32* frame1 = ((u32*) FRAME_BUFFER_0_ADDR) + SCREEN_H * SCREEN_W;

//macros
#define min(x,y) (((x)<(y))?(x):(y))
#define max(x,y) (((x)>(y))?(x):(y))

//Static function prototypes
static void draw_bitmap(const u32* bmp, u32 color, s16 bmp_x, s16 bmp_y,
		s16 bmp_w, s16 bmp_h);
static inline void set_point(s32 x, s32 y, u32 color);

//Macros
#define clr_point(x,y) set_point((x),(y),GAME_BACKGROUND)

void render_init() {
	vdma_init(frame0, frame1);
	// Just paint some large red, green, blue, and white squares in different
	// positions of the image for each frame in the buffer (framePointer0 and framePointer1).
	const u32* bunker_bmp = bmp_bunker_24x18;
	const u32* const tank_bmp = bmp_tank_15x8;

	int row = 0, col = 0;
	//Init screen to background color
	for (row = 0; row < 480; row++) {
		for (col = 0; col < 640; col++) {
			frame0[row * 640 + col] = GAME_BACKGROUND;
			frame1[row * 640 + col] = GAME_BACKGROUND;
		}
	}

	//Draw all bunkers
	u16 bunker_num;
	for (bunker_num = 0; bunker_num < GAME_BUNKER_COUNT; bunker_num++) {
		draw_bitmap(bunker_bmp, COLOR_GREEN,
				GAME_BUNKER_POS + GAME_BUNKER_SEP * bunker_num, GAME_BUNKER_Y,
				BMP_BUNKER_W, BMP_BUNKER_H);
	}

	//Draw tank TODO this should not happen until the controller calls
	//	render the first time
	draw_bitmap(tank_bmp, COLOR_GREEN, RENDER_TANK_X, RENDER_TANK_Y,
			BMP_TANK_W, BMP_TANK_H);

}

#define ALIENS_SEPX (BMP_ALIEN_W)	//X and Y for now
#define ALIENS_SEPY (BMP_ALIEN_W-2)	//X and Y for now
#define ALIENS_START 0
#define ALIENS_END 10
#define ALIENS_ROW_COUNT 5
#define ALIENS_ROWS_TO_UPDATE (ALIENS_ROW_COUNT)
#define ALIENS_MAX_Y ALIENS_SEPY* ALIENS_ROW_COUNT
#define ALIENS_ROW_LEN 11
#define ALIEN_COLOR 0x00FFFFFF
#define ALIENS_ROW0 0
#define ALIENS_ROW1 1
#define ALIENS_ROW2 2
#define ALIENS_ROW3 3
#define ALIENS_ROW4 4
#define ALIENS_ROW5 5

static void update_alien_row(alien_block_t* alien_block,
		alien_block_t* prev_block, const u32* prev_alien, const u32* new_alien,
		u16 row) {

	u16 y = alien_block->pos.y + row * ALIENS_SEPY;
	u16 alien_y;
	for (alien_y = 0; alien_y < BMP_ALIEN_H; alien_y++) {
		s16 offset = alien_block->pos.x - prev_block->pos.x;
		u32 delta;
		u32 set_delta;
		u32 reset_delta;
		u32 new = new_alien[alien_y];
		u32 prev = prev_alien[alien_y];
		if (offset > 0) {
			new = new << offset;
		} else if (offset < 0) {
			offset = -offset;
			new = new >> offset;
		}
		delta = new ^ prev;

		u16 x;
		u16 n;
		for (n = ALIENS_START; n <= ALIENS_END; n++) {
			x = prev_block->pos.x + n * ALIENS_SEPX;
			if (alien_block->alien_status[n + row * ALIENS_ROW_LEN]) {
				set_delta = delta & ~prev;
				reset_delta = delta & prev;
			} else if (prev_block->alien_status[n + row * ALIENS_ROW_LEN]) {
				reset_delta = prev;
				set_delta = 0;
			}

			while (set_delta || reset_delta) {
				if (set_delta & BIT0) {
					set_point(x, y + alien_y, ALIEN_COLOR);
				}
				if (reset_delta & BIT0) {
					clr_point (x,y + alien_y);
				}

				reset_delta >>= 1;
				set_delta >>= 1;
				x++;
			}
		}
	}
}

static void update_alien_block(alien_block_t* alien_block) {
	static alien_block_t prev_block = { { -1, -1 }, { 0 } };
	if (prev_block.pos.x < 0) {
		prev_block.pos.x = alien_block->pos.x - 1;
		prev_block.pos.y = alien_block->pos.y;
	}

	if (alien_block->pos.x != prev_block.pos.x) {

		update_alien_row(alien_block, &prev_block,
				bmp_aliens[prev_block.legs][0],
				bmp_aliens[alien_block->legs][0], 0);
		update_alien_row(alien_block, &prev_block,
				bmp_aliens[prev_block.legs][1],
				bmp_aliens[alien_block->legs][1], 1);
		update_alien_row(alien_block, &prev_block,
				bmp_aliens[prev_block.legs][2],
				bmp_aliens[alien_block->legs][2], 2);
		update_alien_row(alien_block, &prev_block,
				bmp_aliens[prev_block.legs][3],
				bmp_aliens[alien_block->legs][3], 3);
		update_alien_row(alien_block, &prev_block,
				bmp_aliens[prev_block.legs][4],
				bmp_aliens[alien_block->legs][4], 4);

	} else if (alien_block->pos.y != prev_block.pos.y) {
		//Iterate through rows.

		u16 x = prev_block.pos.x;
		u16 n = ALIENS_ROW_LEN;
		for (; n-- > 0;) {
			if (prev_block.alien_status[n]) {
				draw_bitmap(bmp_aliens[prev_block.legs][ALIENS_ROW0],
						GAME_BACKGROUND, x,
						prev_block.pos.y + ALIENS_ROW0 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			//Row 1
			if (prev_block.alien_status[n]) {
				draw_bitmap(bmp_aliens[prev_block.legs][ALIENS_ROW1],
						GAME_BACKGROUND, x,
						prev_block.pos.y + ALIENS_ROW1 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			if (alien_block->alien_status[n]) {
				draw_bitmap(bmp_aliens[alien_block->legs][ALIENS_ROW0],
						ALIEN_COLOR, x,
						alien_block->pos.y + ALIENS_ROW0 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			//Row 2
			if (prev_block.alien_status[n]) {
				draw_bitmap(bmp_aliens[prev_block.legs][ALIENS_ROW2],
						GAME_BACKGROUND, x,
						prev_block.pos.y + ALIENS_ROW2 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			if (alien_block->alien_status[n]) {
				draw_bitmap(bmp_aliens[alien_block->legs][ALIENS_ROW1],
						ALIEN_COLOR, x,
						alien_block->pos.y + ALIENS_ROW1 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			//Row 3
			if (prev_block.alien_status[n]) {
				draw_bitmap(bmp_aliens[prev_block.legs][ALIENS_ROW3],
						GAME_BACKGROUND, x,
						prev_block.pos.y + ALIENS_ROW3 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			if (alien_block->alien_status[n]) {
				draw_bitmap(bmp_aliens[alien_block->legs][ALIENS_ROW2],
						ALIEN_COLOR, x,
						alien_block->pos.y + ALIENS_ROW2 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			//Row 4
			if (prev_block.alien_status[n]) {
				draw_bitmap(bmp_aliens[prev_block.legs][ALIENS_ROW4],
						GAME_BACKGROUND, x,
						prev_block.pos.y + ALIENS_ROW4 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			if (alien_block->alien_status[n]) {
				draw_bitmap(bmp_aliens[alien_block->legs][ALIENS_ROW3],
						ALIEN_COLOR, x,
						alien_block->pos.y + ALIENS_ROW3 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			//Row 5
			if (alien_block->alien_status[n]) {
				draw_bitmap(bmp_aliens[alien_block->legs][ALIENS_ROW4],
						ALIEN_COLOR, x,
						alien_block->pos.y + ALIENS_ROW4 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			x += ALIENS_SEPX;
		}

	} else {
		//blocks are the same
	}
	memcpy(&prev_block, alien_block, sizeof(alien_block_t));
}

static void update_tank(point_t* tank_block) {
	static point_t prev_tank_block = { RENDER_TANK_X, RENDER_TANK_Y };
	const u32* tank_bmp = bmp_tank_15x8;
	u16 y = tank_block->y;
	u16 tank_y;
	for (tank_y = 0; tank_y < BMP_TANK_H; tank_y++) {
		s16 offset = tank_block->x - prev_tank_block.x;
		u32 delta;
		u32 set_delta;
		u32 reset_delta;
		u32 new = tank_bmp[tank_y];
		u32 prev = tank_bmp[tank_y];
		if (offset > 0) {
			new = new << offset;
		} else if (offset < 0) {
			offset = -offset;
			new = new >> offset;
		}
		delta = new ^ prev;

		u16 x;
		x = prev_tank_block.x;
		set_delta = delta & ~prev;
		reset_delta = delta & prev;

		while (set_delta || reset_delta) {
			if (set_delta & BIT0) {
				set_point(x, y + tank_y, COLOR_GREEN);
			}
			if (reset_delta & BIT0) {
				clr_point (x,y+tank_y);
			}

			reset_delta >>= 1;
			set_delta >>= 1;
			x++;
		}
	}
	memcpy(&prev_tank_block, tank_block, sizeof(point_t));

}

static void update_bunkers(u16* bunkerStates) {
	static u16 prev_bunker_state[GAME_BUNKER_COUNT] = { BMP_BUNKER_MAX,
			BMP_BUNKER_MAX, BMP_BUNKER_MAX, BMP_BUNKER_MAX };

	const u32* bmp_bunker_blocks[BUNKER_ROWS * BUNKER_COLS] =
			{ bmp_bunker2_6x6, bmp_bunker1_6x6, bmp_bunker1_6x6,
					bmp_bunker0_6x6, bmp_bunker1_6x6, bmp_bunker5_6x6,
					bmp_bunker3_6x6, bmp_bunker1_6x6, bmp_bunker1_6x6,
					bmp_bunker4_6x6, bmp_bunker4_6x6, bmp_bunker1_6x6 };

	const u32* erosion_states_bmp[BMP_BUNKER_STATES] =
			{ bmp_bunkerDamage0_6x6, bmp_bunkerDamage1_6x6,
					bmp_bunkerDamage2_6x6, bmp_bunkerDamage3_6x6,
					bmp_bunkerDamage4_6x6 };

	u16 bunker_num;
	for (bunker_num = 0; bunker_num < GAME_BUNKER_COUNT; bunker_num++) {
		if (bunkerStates[bunker_num] != prev_bunker_state[bunker_num]) {
			const u32* new_erosion_state_bmp =
					erosion_states_bmp[bunkerStates[bunker_num]];

			u16 row;
			u16 column;
			for (row = 0; row < BUNKER_ROWS; row++) {
				u16 y = GAME_BUNKER_Y + BMP_EROSION_H * row;
				for (column = 0; column < BUNKER_COLS; column++) {

					u16 x = GAME_BUNKER_POS + GAME_BUNKER_SEP * bunker_num
							+ BMP_EROSION_H * column;
					const u32* prev_erosion_state_bmp = bmp_bunker_blocks[row
							* BUNKER_COLS + column];

					u16 bits;
					u32 erosion_bmp[BMP_EROSION_BITS];
					for (bits = 0; bits < BMP_EROSION_BITS; bits++) {
						erosion_bmp[bits] = new_erosion_state_bmp[bits]
								& prev_erosion_state_bmp[bits];
					}
					draw_bitmap(bmp_bunkerDamage4_6x6, COLOR_BLACK, x, y,
							BMP_EROSION_H, BMP_EROSION_H);
					draw_bitmap(erosion_bmp, COLOR_GREEN, x, y, BMP_EROSION_H,
							BMP_EROSION_H);
				}
			}
			memcpy(&prev_bunker_state, bunkerStates, sizeof(u16));
		}
	}
}

static void update_bullet(point_t* tankBulletPos) {

	return;
}

void render(point_t* tankPos, point_t* tankBulletPos, u8 tank_bullet_flag,
		alien_block_t* alienBlock, alien_bullet_t* alienBullets,
		u8* alien_missile_flag, u16* bunkerStates) {

	update_alien_block(alienBlock);

	update_tank(tankPos);

	update_bunkers(bunkerStates);

	if (tankBulletPos->y == (tankPos->y - BMP_BULLET_OFF) && tank_bullet_flag) {
		const u32* bullet_bmp = bmp_bullet_straight_3x5;
		draw_bitmap(bullet_bmp, COLOR_WHITE, tankBulletPos->x,
				tankBulletPos->y, BMP_BULLET_W, BMP_BULLET_H);
	}
	u8 alien_missiles = 0;
	for (alien_missiles; alien_missiles < BMP_MISSILES; alien_missiles++) {

		if (alienBullets[alien_missiles].y == (alienBlock->pos.y
				+ (BMP_ALIEN_H) * 9) && alien_missile_flag[alien_missiles]) {
			const u32* alien_missile_bmp = bmp_alien_missile_cross2_3x5;
			draw_bitmap(alien_missile_bmp, COLOR_WHITE,
					alienBullets[alien_missiles].x,
					alienBullets[alien_missiles].y, BMP_BULLET_W, BMP_BULLET_H);
		}
	}
}

static inline void set_point(s32 x, s32 y, u32 color) {
	x *= 2;
	y *= 2 * 640;
	frame0[y + x] = color;
	frame0[y + x + 1] = color;
	y += SCREEN_W;
	frame0[y + x] = color;
	frame0[y + x + 1] = color;
}

static void draw_bitmap(const u32* bmp, u32 color, s16 bmp_x, s16 bmp_y,
		s16 bmp_w, s16 bmp_h) {
	u32 bmp_row;
	s16 y;
	for (y = 0; y < bmp_h; y++) {
		bmp_row = bmp[y];
		s16 x = bmp_x;
		for (; bmp_row; bmp_row >>= 1) {
			if (bmp_row & BIT0) {
				set_point(x, y + bmp_y, color);
			} else {
			}
			x++;
		}
	}
}
