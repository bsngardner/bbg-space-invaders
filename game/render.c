/*
 * render.c
 *
 * Handles screen drawing and updates
 *
 *  Created on: Sep 28, 2017
 *      Author:  Broderick Gardner
 *      		Benjamin Gardner
 */

#include <stdlib.h>
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

#define BIT0 0x01

#define COLOR_GREEN 0x0000FF00
#define COLOR_BLACK 0x00000000
#define COLOR_WHITE 0x00FFFFFF
#define COLOR_RED   0xFFF00000
#define BULLET_COLOR COLOR_WHITE
#define BUNKER_ROWS 3	//Bunker dimensions in blocks
#define BUNKER_COLS 4

#define SAUCER_WIDTH 16
#define SAUCER_HEIGHT 7
#define SAUCER_STARTX 321 //x position of saucer starts off the screen
#define SAUCER_STARTY 15 //y position of saucer
#define SCORE_X 20 //x pos of score
#define LIVES_X 200 //x pos of lives
#define TANK_SPACE 20 //gap between tanks
#define TOP_SCREEN 1 //y pos of all top of screen sprites
#define SCORE_GAP_1 6
#define SCORE_GAP_2 11
#define SCORE_GAP_3 16
#define SCORE_GAP_4 21

#define WORDS_W 25 //word sprite width
#define WORDS_H 8 //word sprite height
#define FRAME_BUFFER_0_ADDR 0xC1000000  // Starting location in DDR where we will store the images that we display.
static u32* frame0 = (u32*) FRAME_BUFFER_0_ADDR;
static u32* frame1 = ((u32*) FRAME_BUFFER_0_ADDR) + GAME_SCREEN_H
		* GAME_SCREEN_W;//Maybe use this later?
#define OOR -20000	//Out of range value. Guaranteed to be out of range of screen,
//#define DEBUG

//macros
#define min(x,y) (((x)<(y))?(x):(y))	//Not used anymore
#define max(x,y) (((x)>(y))?(x):(y))
#define check(byte,bit) (byte&(table_bit[bit]))

//Static function prototypes
static void draw_bitmap(const u32* bmp, u32 color, s16 bmp_x, s16 bmp_y,
		s16 bmp_w, s16 bmp_h);
static inline void set_point(s32 x, s32 y, u32 color);
static void drawGreenLine(void);
static void update_score(u32 score);
void update_bmp_row(s16 x, s16 y, u32 old, u32 new, u32 color);

//Macros
#define clr_point(x,y) set_point((x),(y),GAME_BACKGROUND)

//Static variables
static bunker_t prev_bunkers[GAME_BUNKER_COUNT];
static tank_t prev_tank;
static saucer_t prev_saucer;
static alien_block_t prev_block;
static alien_missiles_t prev_missiles[GAME_MISSILE_COUNT];

#define RESET_LIVES 0
static u8 prev_lives = RESET_LIVES;

u32* getFrame() {
	return frame0;
}
#define NUMS 4
u32 prev_numbers[NUMS];
void render_init() {
	vdma_init(frame0, frame1);
	// Just paint some large red, green, blue, and white squares in different
	// positions of the image for each frame in the buffer (framePointer0 and framePointer1).
	render_restart();

}

void render_restart() {
	int row = 0, col = 0;
	//Init screen to background color

	for (row = 0; row < GAME_SCREEN_H; row++) {
		for (col = 0; col < GAME_SCREEN_W; col++) {
			frame0[row * GAME_SCREEN_W + col] = GAME_BACKGROUND;
			frame1[row * GAME_SCREEN_W + col] = GAME_BACKGROUND;
		}
	}

	//Draw all bunkers
	u16 bunker_num;
	for (bunker_num = 0; bunker_num < GAME_BUNKER_COUNT; bunker_num++) {

		prev_bunkers[bunker_num].changed = 1;
		for (int block_num = 0; block_num < GAME_BUNKER_BLOCK_COUNT; block_num++) {
			prev_bunkers[bunker_num].block[block_num].changed = 1;
			prev_bunkers[bunker_num].block[block_num].block_health = 0;
		}
	}

	prev_saucer.active = 0;
	prev_saucer.pos.x = SAUCER_STARTX;
	prev_saucer.pos.y = SAUCER_STARTY;

	prev_tank.pos.x = GAME_TANK_STARTX;
	prev_tank.pos.y = GAME_TANK_STARTY;
	prev_tank.state = EMPTY;
	prev_lives = 0;

	prev_tank.missile.pos.x = OOR;
	prev_tank.missile.pos.y = OOR;
	prev_tank.missile.active = 0;

	//Draw Score
	draw_bitmap(bmp_word_score_27x8, COLOR_WHITE, SCORE_X, TOP_SCREEN, WORDS_W,
			WORDS_H);
	u32 offset = WORDS_W + SCORE_X;
	draw_bitmap(number_zero_4x7, COLOR_GREEN, offset + SCORE_GAP_1, TOP_SCREEN,
			BMP_NUMBER_W, BMP_NUMBER_H);
	draw_bitmap(number_zero_4x7, COLOR_GREEN, offset + SCORE_GAP_2, TOP_SCREEN,
			BMP_NUMBER_W, BMP_NUMBER_H);
	draw_bitmap(number_zero_4x7, COLOR_GREEN, offset + SCORE_GAP_3, TOP_SCREEN,
			BMP_NUMBER_W, BMP_NUMBER_H);
	draw_bitmap(number_zero_4x7, COLOR_GREEN, offset + SCORE_GAP_4, TOP_SCREEN,
			BMP_NUMBER_W, BMP_NUMBER_H);

	//Draw Live
	draw_bitmap(bmp_word_lives_27x8, COLOR_WHITE, LIVES_X, TOP_SCREEN, WORDS_W,
			WORDS_H);

	//Draw green line
	drawGreenLine();
}

#define GAME_X 130 //start x pos for game word
#define GAME_Y 120 //start y pos
#define OVER_X 160 //start x pos for over word
void render_gameover() {
	draw_bitmap(bmp_word_game_27x8, COLOR_WHITE, GAME_X, GAME_Y, WORDS_W,
			WORDS_H);
	draw_bitmap(bmp_word_over_27x8, COLOR_WHITE, OVER_X, GAME_Y, WORDS_W,
			WORDS_H);
}

#define NUMBER_1 1
#define NUMBER_2 2
#define NUMBER_3 3
#define THOUSAND 1000
#define HUNDRED 100
#define TEN 10
static void update_score(u32 score) {
	static u32 prev_score = 0;
	if (score == prev_score)
		return;

	u32 thousand = score / THOUSAND;
	u32 hundred = (score - thousand * THOUSAND) / HUNDRED;
	u32 ten = (score - thousand * THOUSAND - hundred * HUNDRED) / TEN;

	u32 offset = WORDS_W + SCORE_X;
	if (prev_numbers[NUMBER_3] != thousand) {
		draw_bitmap(bmp_numbers[prev_numbers[NUMBER_3]], COLOR_BLACK,
				offset + SCORE_GAP_1, TOP_SCREEN, BMP_NUMBER_W, BMP_NUMBER_H);
		draw_bitmap(bmp_numbers[thousand], COLOR_GREEN, offset + SCORE_GAP_1,
				TOP_SCREEN, BMP_NUMBER_W, BMP_NUMBER_H);
		prev_numbers[NUMBER_3] = thousand;
	}
	if (prev_numbers[NUMBER_2] != hundred) {
		draw_bitmap(bmp_numbers[prev_numbers[NUMBER_2]], COLOR_BLACK,
				offset + SCORE_GAP_2, TOP_SCREEN, BMP_NUMBER_W, BMP_NUMBER_H);
		draw_bitmap(bmp_numbers[hundred], COLOR_GREEN, offset + SCORE_GAP_2,
				TOP_SCREEN, BMP_NUMBER_W, BMP_NUMBER_H);
		prev_numbers[NUMBER_2] = hundred;
	}
	if (prev_numbers[NUMBER_1] != ten) {
		draw_bitmap(bmp_numbers[prev_numbers[NUMBER_1]], COLOR_BLACK,
				offset + SCORE_GAP_3, TOP_SCREEN, BMP_NUMBER_W, BMP_NUMBER_H);
		draw_bitmap(bmp_numbers[ten], COLOR_GREEN, offset + SCORE_GAP_3,
				TOP_SCREEN, BMP_NUMBER_W, BMP_NUMBER_H);
		prev_numbers[NUMBER_1] = ten;
	}
	prev_score = score;
}

#define ALIENS_START 0
#define ALIENS_END 10
#define ALIENS_ROW_COUNT 5
#define ALIENS_MAX_Y (ALIENS_SEPY* ALIENS_ROW_COUNT)
#define ALIENS_ROW_LEN 11
#define ALIEN_BLOCK_H (GAME_ALIEN_SEPY * GAME_ALIEN_ROWS)
#define ALIEN_BLOCK_W (GAME_ALIEN_SEPX * GAME_ALIEN_COLS)
#define ALIEN_X_SHIFT 2
#define ALIEN_COLOR 0x00FFFFFF
#define ALIENS_ROW0 0	//Refer to rows of aliens
#define ALIENS_ROW1 1
#define ALIENS_ROW2 2
#define ALIENS_ROW3 3
#define ALIENS_ROW4 4
#define ALIENS_ROW5 5	//This row only exists when shifting down
//Updates alien row when shifted, no more than 2. This limitation is from the
//	padding I could put on the bitmaps.  There is are 2 pixels on either side
//	of the bitmaps that are 0, so shifting further would cause broken sprites.
static void update_alien_row(alien_block_t* alien_block,
		alien_block_t* prev_block, const u32* prev_alien, const u32* new_alien,
		u16 row) {

	u16 y = alien_block->pos.y + row * GAME_ALIEN_SEPY;
	u16 alien_y;
	for (alien_y = 0; alien_y < BMP_ALIEN_H; alien_y++) {
		s16 offset = alien_block->pos.x - prev_block->pos.x; //shift distance. No more than 2
		u32 new = new_alien[alien_y]; //pixels that will be shifted
		u32 prev = prev_alien[alien_y]; //Pixels currently drawn on screen
		new = (offset > 0) ? (new << offset) : (new >> -offset);

		u16 x = prev_block->pos.x; //Holds x coord to draw at.
		u16 n; //Current alien in the row
		for (n = ALIENS_START; n <= ALIENS_END; n++, x += GAME_ALIEN_SEPX) {
			if (alien_block->row_col.x == n && alien_block->row_col.y == row) {
				u32 lnew = (offset > 0) ? (bmp_alien_explosion_12x10[alien_y]
						<< offset) : (bmp_alien_explosion_12x10[alien_y]
						>> -offset);
				u32 lprev;
				if (prev_block->row_col.x == n && prev_block->row_col.y == row) {
					lprev = bmp_alien_explosion_12x10[alien_y];
				} else {
					lprev = prev;
				}
				update_bmp_row(x, y + alien_y, lprev, lnew, ALIEN_COLOR);
			} else if (check(alien_block->alien_status[row],n)) { //Only draw alien if not dead
				update_bmp_row(x, y + alien_y, prev, new, ALIEN_COLOR);
			} else if (check(prev_block->alien_status[row],n)) {
				update_bmp_row(x, y + alien_y,
						bmp_alien_explosion_12x10[alien_y], 0, ALIEN_COLOR);
			}
		}

	}
}

//Important because coordinates can go negative. Just not this negative

//Update the whole alien block.  This shifts left or right, down, or deletes an alien. Only 1 at a time
static void update_alien_block(alien_block_t* alien_block) {
	//Init prev_block to something reasonable and detectable for first update
	static alien_block_t prev_block = { { OOR, OOR }, 0, 0, { 0 }, OUT };
	if (alien_block->changed == 0)
		return;
	bool first = false; //Makes sure an empty bitmap is passed as the previous block
	//So the the whole alien is drawn on the first update
	if (prev_block.pos.x == OOR) {
		prev_block.pos.x = alien_block->pos.x - 1;
		prev_block.pos.y = alien_block->pos.y;
		first = true;
	}

	if (abs(alien_block->pos.x - prev_block.pos.x) <= ALIEN_X_SHIFT
			&& (alien_block->pos.y == prev_block.pos.y)) {

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
	else if (alien_block->pos.y != prev_block.pos.y && (alien_block->pos.x
			== prev_block.pos.x)) {

		s16 y = prev_block.pos.y;
		s16 dy = GAME_ALIEN_DROP;
		u16 x = prev_block.pos.x;

		u16 alien_row;
		u16 rowy = 0;
		for (alien_row = 0; alien_row < GAME_ALIEN_ROWS; alien_row++, rowy
				+=GAME_ALIEN_SEPY) {

			const u32* old_bmp = bmp_aliens[prev_block.legs][alien_row];
			const u32* new_bmp = bmp_aliens[alien_block->legs][alien_row];
			s16 row;
			s16 rowp;
			for (row = 0, rowp = -dy; row < BMP_ALIEN_H + dy; row++, rowp++) {
				u32 old_brow;
				u32 new_brow;
				if (rowp < 0) {
					new_brow = 0;
				} else {
					new_brow = new_bmp[rowp];
				}

				if (row >= BMP_ALIEN_H) {
					old_brow = 0;
				} else {
					old_brow = old_bmp[row];
				}
				u16 lx = 0;
				u16 alien_x = 0;
				for (alien_x = 0; alien_x < GAME_ALIEN_COLS; alien_x++, lx
						+=GAME_ALIEN_SEPX) {
					u32 new;
					u32 old;
					if (alien_block->row_col.x == alien_x
							&& alien_block->row_col.y == alien_row) {

						new = (rowp < 0) ? (0)
								: (bmp_alien_explosion_12x10[rowp]);
						old = old_brow;

					} else if (!check(alien_block->alien_status[alien_row],alien_x)) { //Only draw alien if not dead
						new = 0;
						old = new = (rowp < 0) ? (0)
								: (bmp_alien_explosion_12x10[rowp]);
					} else {
						new = new_brow;
						old = old_brow;
					}

					update_bmp_row(x + lx, y + rowy + row, old, new,
							ALIEN_COLOR);

				}
			}
		}

	}
// If block hasnt moved, just check for killed aliens
    else if (alien_block->pos.x == prev_block.pos.x && alien_block->pos.y
			== prev_block.pos.y) {
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
				draw_bitmap(bmp_alien_explosion_12x10, GAME_BACKGROUND,
						prev_block.pos.x + n * GAME_ALIEN_SEPX,
						alien_block->pos.y + i * GAME_ALIEN_SEPY, BMP_ALIEN_W,
						BMP_ALIEN_H);
			}
        }
    }
    //If the block is moving too far or in both x and y directions, do a full erase and write
	else {
		const u32* bmp;
		s16 x;
		s16 y = prev_block.pos.y;
		for (u16 row = 0; row < GAME_ALIEN_ROWS; row++, y += GAME_ALIEN_SEPY) {

			bmp = bmp_aliens[prev_block.legs][row];
			x = prev_block.pos.x;
			for (u16 col = 0; col < GAME_ALIEN_COLS; col++, x
					+= GAME_ALIEN_SEPX) {
				draw_bitmap(bmp, GAME_BACKGROUND, x, y, BMP_ALIEN_W,
						BMP_ALIEN_H);
			}
		}

		y = alien_block->pos.y;
		for (u16 row = 0; row < GAME_ALIEN_ROWS; row++, y += GAME_ALIEN_SEPY) {
			bmp = bmp_aliens[alien_block->legs][row];
			x = alien_block->pos.x;
			for (u16 col = 0; col < GAME_ALIEN_COLS; col++, x
					+=GAME_ALIEN_SEPX) {
				draw_bitmap(bmp, ALIEN_COLOR, x, y, BMP_ALIEN_W, BMP_ALIEN_H);
			}
		}
	}
	//prev_block must be now equal to alien_block, so copy it over
	memcpy(&prev_block, alien_block, sizeof(alien_block_t));
}

#define MAX_TANK_DISTANCE 2
//Update tank position
static void update_tank(tank_t* tank) {

	if (!tank->changed)
		return;
	//Tank is initially drawn in init, so prev_tank can be initialized to
	//	 the start location of the tank
	const u32* old_bmp = bmp_tanks[prev_tank.state];
	const u32* tank_bmp = bmp_tanks[tank->state]; //Get tank bitmap

	u16 tank_y = tank->pos.y; //This doesnt change
	u16 y; //This does, as we move down each pixel row in the tank
	s16 offset = tank->pos.x - prev_tank.pos.x; //Shift distance

	if (abs(offset) <= MAX_TANK_DISTANCE) {

		for (y = 0; y < BMP_TANK_H; y++) { //Iterate over pixel rows in tank

			u32 new = tank_bmp[y];
			u32 prev = old_bmp[y];
			new = (offset > 0) ? (new << offset) : (new >> -offset);

			update_bmp_row(prev_tank.pos.x, y + tank_y, prev, new, COLOR_GREEN);
		}
	} else {
		draw_bitmap(old_bmp, GAME_BACKGROUND, prev_tank.pos.x, prev_tank.pos.y,
				bmp_tank_dim.x, bmp_tank_dim.y);
		draw_bitmap(tank_bmp, COLOR_GREEN, tank->pos.x, tank->pos.y,
				bmp_tank_dim.x, bmp_tank_dim.y);

	}
	tank->changed = 0;
	prev_tank.pos.x = tank->pos.x;
	prev_tank.state = tank->state;
}

#define SAUCER_X 321
#define SAUCER_Y 15
#define HUNDRED 100
#define ZERO 0
//Update tank position
static void update_saucer(saucer_t* saucer) {
	//saucer is initially drawn in init, so prev_saucer can be initialized to
	//	 the start location of the saucer

	if (prev_saucer.alive && !saucer->alive) {

		draw_bitmap(bmp_saucer_16x7, COLOR_BLACK, saucer->pos.x, saucer->pos.y,
				BMP_SAUCER_W, BMP_SAUCER_H);
		u16 hundred = 0;
		if (saucer->points >= HUNDRED) {
			hundred = saucer->points / HUNDRED;
			draw_bitmap(bmp_numbers[hundred], COLOR_WHITE, saucer->pos.x,
					saucer->pos.y, BMP_NUMBER_W, BMP_NUMBER_H);
		}
		u16 ten = (saucer->points - hundred * HUNDRED) / TEN;
		draw_bitmap(bmp_numbers[ten], COLOR_WHITE, saucer->pos.x + SCORE_GAP_1,
				saucer->pos.y, BMP_NUMBER_W, BMP_NUMBER_H);
		draw_bitmap(bmp_numbers[ZERO], COLOR_WHITE,
				saucer->pos.x + SCORE_GAP_2, saucer->pos.y, BMP_NUMBER_W,
				BMP_NUMBER_H);
		memcpy(&prev_saucer, saucer, sizeof(saucer_t));
		return;
	} else if (!prev_saucer.alive && saucer->alive) {

		u16 hundred = 0;
		if (prev_saucer.points >= HUNDRED) {
			hundred = prev_saucer.points / HUNDRED;
			draw_bitmap(bmp_numbers[hundred], COLOR_BLACK, prev_saucer.pos.x,
					prev_saucer.pos.y, BMP_NUMBER_W, BMP_NUMBER_H);
		}
		u16 ten = (prev_saucer.points - hundred * HUNDRED) / TEN;
		draw_bitmap(bmp_numbers[ten], COLOR_BLACK,
				prev_saucer.pos.x + SCORE_GAP_1, prev_saucer.pos.y,
				BMP_NUMBER_W, BMP_NUMBER_H);
		draw_bitmap(bmp_numbers[ZERO], COLOR_BLACK,
				prev_saucer.pos.x + SCORE_GAP_2, prev_saucer.pos.y,
				BMP_NUMBER_W, BMP_NUMBER_H);

		memcpy(&prev_saucer, saucer, sizeof(saucer_t));
	}

	if (saucer->pos.x == prev_saucer.pos.x || !saucer->alive)
		return;
	const u32* saucer_bmp = bmp_saucer_16x7; //Get saucer bitmap
	u16 y = saucer->pos.y; //This doesnt change
	u16 saucer_y; //This does, as we move down each pixel row in the saucer
	for (saucer_y = 0; saucer_y < BMP_SAUCER_H; saucer_y++) { //Iterate over pixel rows in saucer
		s16 offset = saucer->pos.x - prev_saucer.pos.x; //Shift distance
		u32 delta; //These are the same as in update_alien_row
		u32 set_delta;
		u32 reset_delta;
		u32 new = saucer_bmp[saucer_y];
		u32 prev = saucer_bmp[saucer_y];
		if (offset > 0) {
			new = new << offset;
		} else if (offset < 0) {
			offset = -offset;
			new = new >> offset;
		}
		delta = new ^ prev;

		u16 x;
		x = prev_saucer.pos.x;
		set_delta = delta & ~prev;
		reset_delta = delta & prev;

		while (set_delta || reset_delta) {
			if (set_delta & BIT0) {
				set_point(x, y + saucer_y, COLOR_RED);
			}
			if (reset_delta & BIT0) {
				clr_point (x,y+saucer_y);
			}

			reset_delta >>= 1;
			set_delta >>= 1;
			x++;
		}
	}
	memcpy(&prev_saucer, saucer, sizeof(saucer_t));

}

void update_bmp_row(s16 x, s16 y, u32 old, u32 new, u32 color) {
	u32 delta = old ^ new;
	u32 set_delta = delta & ~old;
	u32 reset_delta = delta & old;

	s16 lx = x;
	while (set_delta || reset_delta) {
		if (set_delta & BIT0) {
			set_point(lx, y, color);
		}
		if (reset_delta & BIT0) {
			clr_point (lx, y);
		}

#define SHIFT1 1
		reset_delta >>= SHIFT1;    
		set_delta >>= SHIFT1;
		lx++;
	}
}

//Update bunkers when eroded
static void update_bunkers(bunker_t* bunkers) {

	bunker_t* bunker;
	bunker_t* prev_bunker;
	u16 bunker_num;
	//Iterate over bunkers
	for (bunker_num = 0; bunker_num < GAME_BUNKER_COUNT; bunker_num++) {
		bunker = bunkers + bunker_num;
		prev_bunker = prev_bunkers + bunker_num;

		if (!bunker->changed)
			continue;

		u16 bunker_x = GAME_BUNKER_POS + GAME_BUNKER_SEP * bunker_num;
		u16 bunker_y = GAME_BUNKER_Y;
		u16 block_num;
		for (block_num = 0; block_num < GAME_BUNKER_BLOCK_COUNT; block_num++) {
			if (!bunker->block[block_num].changed)
				continue;
			//xil_printf("\tblock %d changed\n\r", block_num);
			u16 x = bunker_x + BMP_BUNKER_BLOCK_W * (block_num
					% GAME_BUNKER_WIDTH);
			u16 y = bunker_y + BMP_BUNKER_BLOCK_H * (block_num
					/ GAME_BUNKER_WIDTH);

			u16 block_row;
			for (block_row = 0; block_row < BMP_BUNKER_BLOCK_H; block_row++) {
				//draw_bitmap
				u32
						old_row =
								bmp_bunker_blocks[block_num][block_row]
										& bmp_bunker_damages[prev_bunker->block[block_num].block_health][block_row];
				u32
						new_row =
								bmp_bunker_blocks[block_num][block_row]
										& bmp_bunker_damages[bunker->block[block_num].block_health][block_row];

				update_bmp_row(x, y, old_row, new_row, COLOR_GREEN);
				y++;
			}

			bunker->block[block_num].changed = 0;
		}
		bunker->changed = 0;
		memcpy(prev_bunker, bunker, sizeof(bunker_t));
	}
}

#define MISSILE_SHIFT 2

//Update missile sprites, tank and alien missiles
static void update_missiles(tank_t * tank, alien_missiles_t* alien_missiles) {
	do {
		u16 py; //old y
		u16 n;
		//Iterate over missiles
		for (n = 0; n < GAME_MISSILE_COUNT; n++) {

			alien_missiles_t* prev_missile = prev_missiles + n;
			alien_missiles_t* missile = alien_missiles + n;
			//Tank bullet
			if (missile->pos.xy == prev_missile->pos.xy && missile->active
					== prev_missile->active)
				continue;
			if (prev_missile->active == 0) {
				prev_missile->pos.xy = missile->pos.xy;
				prev_missile->guise = missile->guise;
				prev_missile->type = missile->type;
			}
			s16 y = prev_missile->pos.y;
			s16 dy = missile->pos.y - prev_missile->pos.y;
			u16 x = prev_missile->pos.x;

			const u32* old_bmp;
			const u32* new_bmp;
			if (prev_missile->active)
				old_bmp
						= bmp_alien_missiles[prev_missile->type][prev_missile->guise];
			else
				old_bmp = bmp_empty_projectile;
			if (missile->active)
				new_bmp = bmp_alien_missiles[missile->type][missile->guise];
			else
				new_bmp = bmp_empty_projectile;
			s16 row; //from 0 to bmp height + dy. When less than bmp_height, is row index for non shifted bitmap
			s16 rowp; //from -dy to bmp_height + dy. When positive, is bitmap row index for shifted bitmap
			for (row = 0, rowp = -dy; row < BMP_BULLET_H + dy; row++, rowp++) {
				u32 old_brow;
				u32 new_brow;
				if (rowp < 0) {
					new_brow = 0;
				} else {
					new_brow = new_bmp[rowp];
				}
				if (row >= BMP_BULLET_H) {
					old_brow = 0;
				} else {
					old_brow = old_bmp[row];
				}

				update_bmp_row(x, y + row, old_brow, new_brow, COLOR_WHITE);
			}
			//update prev state to current state
			prev_missile->guise = missile->guise;
			prev_missile->pos.xy = missile->pos.xy;
			prev_missile->active = missile->active;

		}

		//Tank bullet
		if (prev_tank.missile.pos.xy == tank->missile.pos.xy
				&& prev_tank.missile.active == tank->missile.active)
			break;
		if (prev_tank.missile.active == 0) {
			prev_tank.missile.pos.xy = tank->missile.pos.xy;
		}
		s16 y = tank->missile.pos.y;
		s16 dy = prev_tank.missile.pos.y - tank->missile.pos.y;
		u16 x = prev_tank.missile.pos.x;

		const u32* old_bmp;
		const u32* new_bmp;
		if (prev_tank.missile.active)
			old_bmp = bmp_bullet_straight_3x5;
		else
			old_bmp = bmp_empty_projectile;
		if (tank->missile.active)
			new_bmp = bmp_bullet_straight_3x5;
		else
			new_bmp = bmp_empty_projectile;
		s16 row; //Same as above but in the reverse direction. row is index for shifted bitmap (shifted up) 
		s16 rowp; //while rowp is index for unshifted bitmap (old)
		for (row = 0, rowp = -dy; row < BMP_BULLET_H + dy; row++, rowp++) {
			u32 old_brow;
			u32 new_brow;
			if (row >= BMP_BULLET_H) {
				new_brow = 0;
			} else {
				new_brow = new_bmp[row];
			}

			if (rowp < 0) {
				old_brow = 0;
			} else {
				old_brow = old_bmp[rowp];
			}

			update_bmp_row(x, y + row, old_brow, new_brow, COLOR_WHITE);
		}
		prev_tank.missile.pos.xy = tank->missile.pos.xy;
		prev_tank.missile.active = tank->missile.active;
	} while (false);//do{}while(0); is to allow breaking out
	return;
}

#define LIFE_X 230
#define LIFE_COUNT 3 //number of tank lives
#define LIFE_DEC -1
#define LIFE_INC 1
void update_tank_life(u8 tank_lives) {

	if (tank_lives == prev_lives)
		return;
	u16 life = prev_lives;
	s16 delta = (tank_lives > prev_lives) ? (LIFE_INC) : (LIFE_DEC);
	u16 offset; //create the offset for the tanks
	for (; life != tank_lives; life += delta) {

		//draw black
		if (delta < 0) {
			offset = LIFE_X + (life + LIFE_DEC) * TANK_SPACE; //calc x pos offset
			draw_bitmap(bmp_tank_15x8, COLOR_BLACK, offset, TOP_SCREEN,
					BMP_TANK_W, BMP_TANK_H);
		} else {
			offset = LIFE_X + life * TANK_SPACE; //calc x pos offset
			draw_bitmap(bmp_tank_15x8, COLOR_GREEN, offset, TOP_SCREEN,
					BMP_TANK_W, BMP_TANK_H);
		}
	}
	prev_lives = tank_lives;
}

//Externally accessible render function. Calls local functions to render graphics
void render(tank_t* tank, alien_block_t* alienBlock,
		alien_missiles_t* alien_missiles, bunker_t* bunkers, saucer_t* saucer,
		u32 score) {

	update_missiles(tank, alien_missiles);
	update_tank_life(tank->lives);
	update_score(score);
	update_bunkers(bunkers);
	update_tank(tank);
	update_saucer(saucer);
	update_alien_block(alienBlock);
}

#define RES_SCALE 2
//Utility function, converts between game resolution and screen resolution
static inline void set_point(s32 x, s32 y, u32 color) {
	if (x < 0 || x >= GAME_W || y < 0 || y >= GAME_H)
		return; //IF coordinates are outside bounds, do not draw
	//Scale x and y to screen coordinates
	x *= RES_SCALE;
	y *= RES_SCALE * GAME_SCREEN_W;
	//Draw 2x2 game pixel
	frame0[y + x] = color;
	frame0[y + x + 1] = color;
	y += GAME_SCREEN_W;
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

//Utility function for drawing a horizontal green line on game screen

static void drawGreenLine() {
	u32 row;
	u32 col;
	for (row = 0; row < 480; row++) {
		for (col = 0; col < 640; col++) {
			if (row >= 450 && row <= 452)
				frame0[row * 640 + col] = COLOR_GREEN;
		}
	}
}

