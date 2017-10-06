/*
 * render.c
 *
 * Handles screen drawing and updates
 *
 *  Created on: Sep 28, 2017
 *      Author:  Broderick Gardner
 *      		Benjamin Gardner
 */

#include <stdio.h>
#include "string.h"
#include <stdbool.h>

#include "render.h"
#include "xparameters.h"
#include "xio.h"
#include "xil_types.h"

#include "vdma.h"
#include "bmp.h"
#include "table.h"
#include "game_controller.h"

#define BIT0 0x01

#define COLOR_GREEN 0x0000FF00
#define COLOR_BLACK 0x00000000
#define COLOR_WHITE 0x00FFFFFF
#define GAME_BACKGROUND COLOR_BLACK
#define BULLET_COLOR COLOR_WHITE
#define BUNKER_ROWS 3	//Bunker dimensions in blocks
#define BUNKER_COLS 4

#define SCREEN_H 480
#define SCREEN_W 640

#define ALIENS_SEPX (BMP_ALIEN_W)	//Horizontal separation between aliens
#define ALIENS_SEPY (BMP_ALIEN_W-2)	//Separation between aliens vertically
#define ALIENS_START 0
#define ALIENS_END 10
#define ALIENS_ROW_COUNT 5
#define ALIENS_MAX_Y (ALIENS_SEPY* ALIENS_ROW_COUNT)
#define ALIENS_ROW_LEN 11
#define ALIEN_COLOR 0x00FFFFFF
#define ALIENS_ROW0 0	//Refer to rows of aliens
#define ALIENS_ROW1 1
#define ALIENS_ROW2 2
#define ALIENS_ROW3 3
#define ALIENS_ROW4 4
#define ALIENS_ROW5 5	//This row only exists when shifting down
#define FRAME_BUFFER_0_ADDR 0xC1000000  // Starting location in DDR where we will store the images that we display.
static u32* frame0 = (u32*) FRAME_BUFFER_0_ADDR;
static u32* frame1 = ((u32*) FRAME_BUFFER_0_ADDR) + SCREEN_H * SCREEN_W;//Maybe use this later?

//#define DEBUG

//macros
#define min(x,y) (((x)<(y))?(x):(y))	//Not used anymore
#define max(x,y) (((x)>(y))?(x):(y))
#define check(byte,bit) (byte&(table_bit[bit]))

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

//Updates alien row when shifted, no more than 2. This limitation is from the
//	padding I could put on the bitmaps.  There is are 2 pixels on either side
//	of the bitmaps that are 0, so shifting further would cause broken sprites.
static void update_alien_row(alien_block_t* alien_block,
		alien_block_t* prev_block, const u32* prev_alien, const u32* new_alien,
		u16 row) {

	u16 y = alien_block->pos.y + row * ALIENS_SEPY;
	u16 alien_y;
	for (alien_y = 0; alien_y < BMP_ALIEN_H; alien_y++) {
		s16 offset = alien_block->pos.x - prev_block->pos.x; //shift distance. No more than 2
		u32 delta; //Holds delta between current row of drawn pixels and the shifted pixels
		u32 set_delta; //Holds pixels that need to be set
		u32 reset_delta; //Holds pixels that need to be cleared
		u32 new = new_alien[alien_y]; //pixels that will be shifted
		u32 prev = prev_alien[alien_y]; //Pixels currently drawn on screen
		if (offset > 0) { //Shift in the right direction
			new = new << offset;
		} else if (offset < 0) {
			offset = -offset;
			new = new >> offset;
		}
		delta = new ^ prev; //Compute delta

		u16 x; //Holds x coord to draw at.
		u16 n; //Current alien in the row
		for (n = ALIENS_START; n <= ALIENS_END; n++) {
			x = prev_block->pos.x + n * ALIENS_SEPX;
			if (check(alien_block->alien_status[row],n)) { //Only draw alien if not dead
				set_delta = delta & ~prev;
				reset_delta = delta & prev;
			} else {
				continue; //If the alien is dead, go to the next alien
			}

			while (set_delta || reset_delta) { //Set pixels as long as there is something to change
				if (check(set_delta, BIT0)) { //Read at bit0
					set_point(x, y + alien_y, ALIEN_COLOR); //here y is global y of alien
					// alien_y is local y of alien
				}
				if (check(reset_delta, BIT0)) { //Same as above
					clr_point (x,y + alien_y);
				}

				reset_delta >>= 1; //Shift out bit
				set_delta >>= 1;
				x++; //move x
			}
		}
	}
}

#define OOR -20000	//Out of range value. Guaranteed to be out of range of screen,
//Important because coordinates can go negative. Just not this negative

//Update the whole alien block.  This shifts left or right, down, or deletes an alien. Only 1 at a time
static void update_alien_block(alien_block_t* alien_block) {
	//Init prev_block to something reasonable and detectable for first update
	static alien_block_t prev_block = { { OOR, OOR }, 0, 0, { 0 }, OUT };
	bool first = false; //Makes sure an empty bitmap is passed as the previous block
	//So the the whole alien is drawn on the first update
	if (prev_block.pos.x == OOR) {
		prev_block.pos.x = alien_block->pos.x - 1;
		prev_block.pos.y = alien_block->pos.y;
		first = true;
	}

	//If x value is different. TODO check for different y values, since this would be bad
	if (alien_block->pos.x != prev_block.pos.x) {

		//Call update on each row
		update_alien_row(
				alien_block,
				&prev_block,
				(first ? bmp_alien_empty
						: bmp_aliens[prev_block.legs][ALIENS_ROW0]),
				bmp_aliens[alien_block->legs][ALIENS_ROW0], ALIENS_ROW0);
		update_alien_row(
				alien_block,
				&prev_block,
				(first ? bmp_alien_empty
						: bmp_aliens[prev_block.legs][ALIENS_ROW1]),
				bmp_aliens[alien_block->legs][ALIENS_ROW1], ALIENS_ROW1);
		update_alien_row(
				alien_block,
				&prev_block,
				(first ? bmp_alien_empty
						: bmp_aliens[prev_block.legs][ALIENS_ROW2]),
				bmp_aliens[alien_block->legs][ALIENS_ROW2], ALIENS_ROW2);
		update_alien_row(
				alien_block,
				&prev_block,
				(first ? bmp_alien_empty
						: bmp_aliens[prev_block.legs][ALIENS_ROW3]),
				bmp_aliens[alien_block->legs][ALIENS_ROW3], ALIENS_ROW3);
		update_alien_row(
				alien_block,
				&prev_block,
				(first ? bmp_alien_empty
						: bmp_aliens[prev_block.legs][ALIENS_ROW4]),
				bmp_aliens[alien_block->legs][ALIENS_ROW4], ALIENS_ROW4);

	} //If y value is different. x values guaranteed to be the same
	else if (alien_block->pos.y != prev_block.pos.y) {

		u16 x = prev_block.pos.x;
		u16 n;
		for (n = 0; n < ALIENS_ROW_LEN; n++) { //Iterate through columns
			//In each column, draw alien in each row

			//First row, erase old row
			if (check(prev_block.alien_status[ALIENS_ROW0],n)) {
				draw_bitmap(bmp_aliens[prev_block.legs][ALIENS_ROW0],
						GAME_BACKGROUND, x,
						prev_block.pos.y + ALIENS_ROW0 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}

			//Row 1	- Erase old, draw new
			if (check(prev_block.alien_status[ALIENS_ROW1],n)) {
				draw_bitmap(bmp_aliens[prev_block.legs][ALIENS_ROW1],
						GAME_BACKGROUND, x,
						prev_block.pos.y + ALIENS_ROW1 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			if (check(alien_block->alien_status[ALIENS_ROW0],n)) {
				draw_bitmap(bmp_aliens[alien_block->legs][ALIENS_ROW0],
						ALIEN_COLOR, x,
						alien_block->pos.y + ALIENS_ROW0 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}

			//Row 2	- Erase old, draw new
			if (check(prev_block.alien_status[ALIENS_ROW2],n)) {
				draw_bitmap(bmp_aliens[prev_block.legs][ALIENS_ROW2],
						GAME_BACKGROUND, x,
						prev_block.pos.y + ALIENS_ROW2 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			if (check(alien_block->alien_status[ALIENS_ROW1],n)) {
				draw_bitmap(bmp_aliens[alien_block->legs][ALIENS_ROW1],
						ALIEN_COLOR, x,
						alien_block->pos.y + ALIENS_ROW1 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}

			//Row 3	- Erase old, draw new
			if (check(prev_block.alien_status[ALIENS_ROW3],n)) {
				draw_bitmap(bmp_aliens[prev_block.legs][ALIENS_ROW3],
						GAME_BACKGROUND, x,
						prev_block.pos.y + ALIENS_ROW3 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			if (check(alien_block->alien_status[ALIENS_ROW2],n)) {
				draw_bitmap(bmp_aliens[alien_block->legs][ALIENS_ROW2],
						ALIEN_COLOR, x,
						alien_block->pos.y + ALIENS_ROW2 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}

			//Row 4	- Erase old, draw new
			if (check(prev_block.alien_status[ALIENS_ROW4],n)) {
				draw_bitmap(bmp_aliens[prev_block.legs][ALIENS_ROW4],
						GAME_BACKGROUND, x,
						prev_block.pos.y + ALIENS_ROW4 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			if (check(alien_block->alien_status[ALIENS_ROW3],n)) {
				draw_bitmap(bmp_aliens[alien_block->legs][ALIENS_ROW3],
						ALIEN_COLOR, x,
						alien_block->pos.y + ALIENS_ROW3 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			//Row 5	- just draw new; there is nothing there (except bunkers?)
			if (check(alien_block->alien_status[ALIENS_ROW4],n)) {
				draw_bitmap(bmp_aliens[alien_block->legs][ALIENS_ROW4],
						ALIEN_COLOR, x,
						alien_block->pos.y + ALIENS_ROW4 * ALIENS_SEPY,
						BMP_ALIEN_W, BMP_ALIEN_H);
			}
			x += ALIENS_SEPX; //x increases by the separation between aliens each column
		}

	} else {
		//blocks are the same
		u16 i;
		for (i = 0; i < ALIENS_ROW_COUNT; i++) {
			//Assuming aliens only die and do not resurrect
			//If there is a change, then something died
			u16 bit = prev_block.alien_status[i] ^ alien_block->alien_status[i];
			u16 n = 0;
			if (bit) {
				while (bit >>= 1) { //Which bit is it?
					n++;
				}
				//Draw over alien to delete
				draw_bitmap(bmp_aliens[prev_block.legs][i], GAME_BACKGROUND,
						prev_block.pos.x + n * ALIENS_SEPX,
						alien_block->pos.y + i * ALIENS_SEPY, BMP_ALIEN_W,
						BMP_ALIEN_H);
			}
		}
	}
	//prev_block must be now equal to alien_block, so copy it over
	memcpy(&prev_block, alien_block, sizeof(alien_block_t));
}

//Update tank position
static void update_tank(point_t* tank_block) {
	//Tank is initially drawn in init, so prev_tank can be initialized to
	//	 the start location of the tank
	static point_t prev_tank_block = { RENDER_TANK_X, RENDER_TANK_Y };
	const u32* tank_bmp = bmp_tank_15x8; //Get tank bitmap
	u16 y = tank_block->y; //This doesnt change
	u16 tank_y; //This does, as we move down each pixel row in the tank
	for (tank_y = 0; tank_y < BMP_TANK_H; tank_y++) { //Iterate over pixel rows in tank
		s16 offset = tank_block->x - prev_tank_block.x; //Shift distance
		u32 delta; //These are the same as in update_alien_row
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

//Update bunkers when eroded
static void update_bunkers(u16* bunkerStates) {
	static u16 prev_bunker_state[GAME_BUNKER_COUNT] = { BMP_BUNKER_MAX,
			BMP_BUNKER_MAX, BMP_BUNKER_MAX, BMP_BUNKER_MAX };

	//Bunker blocks for calculating change
	const u32* bmp_bunker_blocks[BUNKER_ROWS * BUNKER_COLS] =
			{ bmp_bunker2_6x6, bmp_bunker1_6x6, bmp_bunker1_6x6,
					bmp_bunker0_6x6, bmp_bunker1_6x6, bmp_bunker5_6x6,
					bmp_bunker3_6x6, bmp_bunker1_6x6, bmp_bunker1_6x6,
					bmp_bunker4_6x6, bmp_bunker4_6x6, bmp_bunker1_6x6 };

	//Erosion states
	const u32* erosion_states_bmp[BMP_BUNKER_STATES] = { bmp_bunkerDamage0_6x6,
			bmp_bunkerDamage1_6x6, bmp_bunkerDamage2_6x6,
			bmp_bunkerDamage3_6x6, bmp_bunkerDamage4_6x6 };

	u16 bunker_num;
	//Iterate over bunkers
	for (bunker_num = 0; bunker_num < GAME_BUNKER_COUNT; bunker_num++) {
		if (bunkerStates[bunker_num] != prev_bunker_state[bunker_num]) {
			//Load the erosion state for the given bunker
			const u32* new_erosion_state_bmp =
					erosion_states_bmp[bunkerStates[bunker_num]];

			u16 row;
			u16 column;
			//For each row and column of each bunker, calculate change in erosion
			// and apply damage
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
			//Update previous bunker state to current
			memcpy(&prev_bunker_state, bunkerStates, sizeof(u16));
		}
	}
}

#define MISSILE_SHIFT 2

//Update missile sprites, tank and alien missiles
static void update_missiles(tank_t * tank, alien_missiles_t* alien_missiles) {
	u16 py; //old y
	u16 n;
	//Iterate over missiles
	for (n = 0; n < GAME_CONTROLLER_MISSILES; n++) {
		py = alien_missiles[n].pos.y - MISSILE_SHIFT; //Previous missile was back a bit
		//Clear old missile
		draw_bitmap(
				bmp_alien_missiles[alien_missiles[n].type][!alien_missiles[n].state],
				GAME_BACKGROUND, alien_missiles[n].pos.x, py, BMP_BULLET_W,
				BMP_BULLET_H);
		//If active, draw new missile
		if (alien_missiles[n].active)
			draw_bitmap(
					bmp_alien_missiles[alien_missiles[n].type][alien_missiles[n].state],
					BULLET_COLOR, alien_missiles[n].pos.x,
					alien_missiles[n].pos.y, BMP_BULLET_W, BMP_BULLET_H);
	}
	py = tank->missile.pos.y + MISSILE_SHIFT;
	//Clear old missile
	draw_bitmap(bmp_bullet_straight_3x5, GAME_BACKGROUND, tank->missile.pos.x,
			py, BMP_BULLET_W, BMP_BULLET_H);
	//If active, draw new missile
	if (tank->missile.active)
		draw_bitmap(bmp_bullet_straight_3x5, BULLET_COLOR, tank->missile.pos.x,
				tank->missile.pos.y, BMP_BULLET_W, BMP_BULLET_H);
}

//Externally accessible render function. Calls local functions to render graphics
void render(tank_t* tank, alien_block_t* alienBlock,
		alien_missiles_t* alien_missiles, u16* bunkerStates) {

	update_alien_block(alienBlock);

	update_tank(&tank->pos);

	update_missiles(tank, alien_missiles);

	update_bunkers(bunkerStates);
}

#define RES_SCALE 2
//Utility function, converts between game resolution and screen resolution
static inline void set_point(s32 x, s32 y, u32 color) {
	if (x < 0 || x >= GAME_W || y < 0 || y >= GAME_H)
		return; //IF coordinates are outside bounds, do not draw
	//Scale x and y to screen coordinates
	x *= RES_SCALE;
	y *= RES_SCALE * SCREEN_W;
	//Draw 2x2 game pixel
	frame0[y + x] = color;
	frame0[y + x + 1] = color;
	y += SCREEN_W;
	frame0[y + x] = color;
	frame0[y + x + 1] = color;
}

//Utility function for drawing bitmaps on game screen.
static void draw_bitmap(const u32* bmp, u32 color, s16 bmp_x, s16 bmp_y,
		s16 bmp_w, s16 bmp_h) {
	u32 bmp_row; //pixel row of bitmap
	s16 y; //local y
	for (y = 0; y < bmp_h; y++) {
		bmp_row = bmp[y];
		s16 x = bmp_x;
		//Shift out pixels
		for (; bmp_row; bmp_row >>= 1) {
			if (bmp_row & BIT0) {
				set_point(x, y + bmp_y, color);
			}
			x++;//Move along (All American Rejects)
		}
	}
}
