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

#define FRAME_BUFFER_0_ADDR 0xC1000000  // Starting location in DDR where we will store the images that we display.
static u32* frame0 = (u32*) FRAME_BUFFER_0_ADDR;
static u32* frame1 = ((u32*) FRAME_BUFFER_0_ADDR) + SCREEN_H * SCREEN_W;

//macros
#define min(x,y) (((x)<(y))?(x):(y))
#define max(x,y) (((x)>(y))?(x):(y))

static u8 frameIndex = 0;

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
#define ALIENS_MAX_Y ALIENS_SEPY* ALIENS_ROW_COUNT
#define ALIENS_ROW_LEN 11
#define ALIEN_COLOR 0x00FFFFFF

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
		prev_block.pos.x = alien_block->pos.y;
	}

	const u32* new_alien;
	const u32* prev_alien;
	if (alien_block->pos.x != prev_block.pos.x) {
		if (alien_block->legs == OUT) {
			new_alien = bmp_alien_top_out_12x8;
		} else {
			new_alien = bmp_alien_top_in_12x8;
		}
		//		if (prev_block.pos.y < 0) {
		//			prev_alien = bmp_alien_empty;
		//		} else
		if (prev_block.legs == OUT) {
			prev_alien = bmp_alien_top_out_12x8;
		} else {
			prev_alien = bmp_alien_top_in_12x8;
		}

		update_alien_row(alien_block, &prev_block, prev_alien, new_alien, 0);

		if (alien_block->legs == OUT) {
			new_alien = bmp_alien_middle_out_12x8;
		} else {
			new_alien = bmp_alien_middle_in_12x8;
		}
		if (prev_block.legs == OUT) {
			prev_alien = bmp_alien_middle_out_12x8;
		} else {
			prev_alien = bmp_alien_middle_in_12x8;
		}

		update_alien_row(alien_block, &prev_block, prev_alien, new_alien, 1);
		update_alien_row(alien_block, &prev_block, prev_alien, new_alien, 2);

		if (alien_block->legs == OUT) {
			new_alien = bmp_alien_bottom_out_12x8;
		} else {
			new_alien = bmp_alien_bottom_in_12x8;
		}
		if (prev_block.legs == OUT) {
			prev_alien = bmp_alien_bottom_out_12x8;
		} else {
			prev_alien = bmp_alien_bottom_in_12x8;
		}

		update_alien_row(alien_block, &prev_block, prev_alien, new_alien, 3);
		update_alien_row(alien_block, &prev_block, prev_alien, new_alien, 4);

	} else if (alien_block->pos.y != prev_block.pos.y) {
		//Iterate through rows.
		u32 delta;
		u32 set_delta;
		u32 reset_delta;
		s16 py = prev_block.pos.y;
		s16 ny = alien_block->pos.y;
		u32 pr, nr;
		u16 y;
		u16 maxy;
		if (py < ny) {
			y = py;
			maxy = ny + ALIENS_MAX_Y;
		} else {
			y = ny;
			maxy = py + ALIENS_MAX_Y;
		}
		for (; y < maxy; y++) {
			py = y - py;
			ny = y - ny;

			pr
					= bmp_aliens[prev_block.legs][((py >= 0) && (py
							< ALIENS_MAX_Y)) ? (py / 16) : 5][py % 5];
			nr
					= bmp_aliens[prev_block.legs][((ny >= 0) && (ny
							< ALIENS_MAX_Y)) ? (ny / 16) : 5][ny % 5];
			delta = pr ^ nr;
			set_delta = delta & ~pr;
			reset_delta = delta & pr;
			u16 x = 0;
			while (x < BMP_ALIEN_W) {
				u16 n;
				u16 pn = py / 16 * 11;
				u16 nn = ny / 16 * 11;
				for (n = 0; n < 11; n++) { //TODO REPLACE MAGIC NUMBER
					if (set_delta & BIT0 || prev_block.alien_status[pn + n]) {
						set_point(x, y, ALIEN_COLOR);
					}
					if (reset_delta & BIT0 || alien_block->alien_status[nn + n]) {
						clr_point (x,y);
					}

					nn++;
					pn++;
				}
				reset_delta >>= 1;
				set_delta >>= 1;
				x++;
			}
		}

	} else {
		//blocks are the same
	}
	memcpy(&prev_block, alien_block, sizeof(alien_block_t));
}

void render(point_t* tankPos, point_t* tankBulletPos,
		alien_block_t* alienBlock, alien_bullet_t* alienBullets,
		u16* bunkerStates) {

	update_alien_block(alienBlock);
	//frameIndex = (frameIndex + 1) % 2; // Alternate between frame 0 and frame 1.

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
	bmp_x += bmp_w;
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
			x--;
		}
	}
}

#if 0
static void draw_bunker(s16 bunker_x, s16 bunker_y) {
	if (bunker_x < 0 || bunker_y < 0) {
		//undraw bunker?
	}
	const u32* bunker_bmp = bitmaps_getBunkerBMP();
	bunker_x += BMP_BUNKER_WIDTH;
	u32 bunker_row;
	s16 y;
	for (y = 0; y < BMP_BUNKER_HEIGHT; y++) {
		bunker_row = bunker_bmp[y];
		s16 x = bunker_x;
		for (; bunker_row; bunker_row >>= 1) {
			if (bunker_row & BIT0) {
				set_point(x, y + bunker_y, COLOR_GREEN);
			} else {
			}
			x--;
		}
	}
}
static void draw_tank(s16 tank_x, s16 tank_y) {
	const u32* const tank_bmp = bitmaps_getTankBMP();
	tank_x += BMP_TANK_WIDTH;
	u32 tank_row;
	s16 y;
	for (y = 0; y < BMP_TANK_HEIGHT; y++) {
		tank_row = tank_bmp[y];
		s16 x = tank_x;
		for (; tank_row; tank_row >>= 1) {
			if (tank_row & BIT0) {
				set_point(x, y + tank_y, COLOR_GREEN);
			} else {
			}
			x--;
		}
	}
}
#endif
