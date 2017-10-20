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
#define COLOR_RED   0xFFF00000
#define GAME_BACKGROUND COLOR_BLACK
#define BULLET_COLOR COLOR_WHITE
#define BUNKER_ROWS 3	//Bunker dimensions in blocks
#define BUNKER_COLS 4

#define SCREEN_H 480
#define SCREEN_W 640

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
#define SAUCER_WIDTH 16
#define SAUCER_HEIGHT 7
#define SAUCER_X 321 //x position of saucer starts off the screen
#define SAUCER_Y 15 //y position of saucer
#define SCORE_X 20 //x pos of score
#define LIVES_X 200 //x pos of lives
#define TANK_LIFE_1 230 //x pos of first life
#define TANK_LIFE_2 250 //x pos of second life
#define TANK_LIFE_3 270 //x pos of third life
#define TANK_SPACE 20 //gap between tanks
#define TOP_SCREEN 1 //y pos of all top of screen sprites
#define SCORE_GAP_1 6
#define SCORE_GAP_2 11
#define SCORE_GAP_3 16
#define SCORE_GAP_4 21

#define TANK_HEIGHT 8 //tank sprite height
#define TANK_WIDTH 15 //tank sprite width
#define WORDS_W 25 //word sprite width
#define WORDS_H 8 //word sprite height
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
static void drawGreenLine(void);
static void update_score(u32 score);

//Macros
#define clr_point(x,y) set_point((x),(y),GAME_BACKGROUND)

//Static variables
static bunker_t prev_bunkers[GAME_BUNKER_COUNT];
static point_t prev_tank_block = { RENDER_TANK_X, RENDER_TANK_Y };
static point_t prev_saucer_block = { SAUCER_X, SAUCER_Y };

u32* getFrame() {
	return frame0;
}
#define NUMS 4
u32 prev_numbers[NUMS];
void render_init() {
	vdma_init(frame0, frame1);
	// Just paint some large red, green, blue, and white squares in different
	// positions of the image for each frame in the buffer (framePointer0 and framePointer1).
	const u32* bunker_bmp = bmp_bunker_24x18;

	int row = 0, col = 0;
	//Init screen to background color
	for (row = 0; row < SCREEN_H; row++) {
		for (col = 0; col < SCREEN_W; col++) {
			frame0[row * SCREEN_W + col] = GAME_BACKGROUND;
			frame1[row * SCREEN_W + col] = GAME_BACKGROUND;
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
		//
		//		draw_bitmap(bunker_bmp, COLOR_GREEN,
		//				GAME_BUNKER_POS + GAME_BUNKER_SEP * bunker_num, GAME_BUNKER_Y,
		//				BMP_BUNKER_W, BMP_BUNKER_H);
	}

	//Draw tank TODO this should not happen until the controller calls
	//	render the first time
	render_tank();

	//Draw Red Saucer
	render_saucer();

	//Draw little tanks for score
	draw_bitmap(bmp_tank_15x8, COLOR_GREEN, TANK_LIFE_1, TOP_SCREEN,
			TANK_WIDTH, TANK_HEIGHT);
	draw_bitmap(bmp_tank_15x8, COLOR_GREEN, TANK_LIFE_2, TOP_SCREEN,
			TANK_WIDTH, TANK_HEIGHT);
	draw_bitmap(bmp_tank_15x8, COLOR_GREEN, TANK_LIFE_3, TOP_SCREEN,
			TANK_WIDTH, TANK_HEIGHT);

	//Draw Score
	draw_bitmap(word_score_27x8, COLOR_WHITE, SCORE_X, TOP_SCREEN, WORDS_W,
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
	draw_bitmap(word_lives_27x8, COLOR_WHITE, LIVES_X, TOP_SCREEN, WORDS_W,
			WORDS_H);

	//Draw green line
	drawGreenLine();

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
#define GAME_X 130 //start x pos for game word
#define GAME_Y 120 //start y pos
#define OVER_X 160 //start x pos for over word
void render_game_over(void) {
	draw_bitmap(word_game_27x8, COLOR_WHITE, GAME_X, GAME_Y, WORDS_W, WORDS_H);
	draw_bitmap(word_over_27x8, COLOR_WHITE, OVER_X, GAME_Y, WORDS_W, WORDS_H);
	char input;
	input = getchar();
}

void render_tank(void) {
	draw_bitmap(bmp_tank_15x8, COLOR_GREEN, RENDER_TANK_X, RENDER_TANK_Y,
				BMP_TANK_W, BMP_TANK_H);
	point_t tank_block = { RENDER_TANK_X, RENDER_TANK_Y };
	memcpy(&prev_tank_block, &tank_block, sizeof(point_t));
}

void render_saucer(void) {
	draw_bitmap(saucer_16x7, COLOR_RED, SAUCER_X, SAUCER_Y, SAUCER_WIDTH,
				SAUCER_HEIGHT);
	point_t saucer_block = { SAUCER_X, SAUCER_Y };
	memcpy(&prev_saucer_block, &saucer_block, sizeof(point_t));
}

void render_saucer_death(u16 x, u16 y) {
	xil_printf("POSITION %d %d\r\n", x, y);
	draw_bitmap(saucer_16x7, COLOR_BLACK, x, y, SAUCER_WIDTH,
				SAUCER_HEIGHT);
}

void render_saucer_points(u16 x, u16 y, u16 points, u8 on) {
	u32 color;
	if(on)
		color = COLOR_WHITE;
	else
		color = COLOR_BLACK;

	u32 hundred = (points) / HUNDRED;
	u32 ten = (points-hundred*HUNDRED) / TEN;
	u16 offset = BMP_NUMBER_W + NUMBER_2;
	if(hundred!=0) {
		draw_bitmap(bmp_numbers[hundred], color, x, y, BMP_NUMBER_W, BMP_NUMBER_H);
	}
	draw_bitmap(bmp_numbers[ten], color, x + offset, y, BMP_NUMBER_W, BMP_NUMBER_H);
	draw_bitmap(bmp_numbers[ten], color, x + offset + offset, y, BMP_NUMBER_W, BMP_NUMBER_H);
}

void render_explosion_1(u16 x, u16 y) {
	xil_printf("EXPLODE ONCE %d %d\r\n", x,y);
	draw_bitmap(bmp_tank_15x8, COLOR_BLACK, x, y, BMP_TANK_W, BMP_TANK_H);
	draw_bitmap(bmp_tank_explode1_15x8, COLOR_GREEN, x, y, BMP_TANK_W, BMP_TANK_H);
}

void render_explosion_2(u16 x, u16 y) {
	xil_printf("EXPLODE TWICE %d %d\r\n", x,y);
	draw_bitmap(bmp_tank_explode1_15x8, COLOR_BLACK, x, y, BMP_TANK_W, BMP_TANK_H);
	draw_bitmap(bmp_tank_explode2_15x8, COLOR_GREEN, x, y, BMP_TANK_W, BMP_TANK_H);
}

void render_explosion_3(u16 x, u16 y) {
	xil_printf("EXPLODE THREE %d %d\r\n", x,y);
	draw_bitmap(bmp_tank_explode2_15x8, COLOR_BLACK, x, y, BMP_TANK_W, BMP_TANK_H);
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
				if (check(set_delta, 0)) { //Read at bit0
					set_point(x, y + alien_y, ALIEN_COLOR); //here y is global y of alien
					// alien_y is local y of alien
				}
				if (check(reset_delta, 0)) { //Same as above
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

	if (tank_block->x == prev_tank_block.x)
		return;
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
#define SAUCER_X 321
#define SAUCER_Y 15
//Update tank position
static void update_saucer(point_t* saucer_block) {
	//saucer is initially drawn in init, so prev_saucer can be initialized to
	//	 the start location of the saucer

	if (saucer_block->x == prev_saucer_block.x)
		return;
	const u32* saucer_bmp = bmp_saucer_16x7; //Get saucer bitmap
	u16 y = saucer_block->y; //This doesnt change
	u16 saucer_y; //This does, as we move down each pixel row in the saucer
	for (saucer_y = 0; saucer_y < BMP_SAUCER_H; saucer_y++) { //Iterate over pixel rows in saucer
		s16 offset = saucer_block->x - prev_saucer_block.x; //Shift distance
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
		x = prev_saucer_block.x;
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
	memcpy(&prev_saucer_block, saucer_block, sizeof(point_t));

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

		reset_delta >>= 1;
		set_delta >>= 1;
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
	u16 py; //old y
	u16 n;
	//Iterate over missiles
	for (n = 0; n < GAME_CONTROLLER_MISSILES; n++) {
		py = alien_missiles[n].pos.y - MISSILE_SHIFT; //Previous missile was back a bit
		//Clear old missile
		draw_bitmap(
				bmp_alien_missiles[alien_missiles[n].type][!alien_missiles[n].guise],
				GAME_BACKGROUND, alien_missiles[n].pos.x, py, BMP_BULLET_W,
				BMP_BULLET_H);
		//If active, draw new missile
		if (alien_missiles[n].active)
			draw_bitmap(
					bmp_alien_missiles[alien_missiles[n].type][alien_missiles[n].guise],
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

#define LIFE_COUNT 3 //number of tank lives
#define LIFE_DEC -1
#define LIFE_INC 1
void update_tank_life_draw(u8 tank_lives) {
	static u8 prev_lives = 0;
	if (tank_lives == prev_lives)
		return;
	u16 life = prev_lives;
	s16 delta = (tank_lives > prev_lives) ? (LIFE_INC) : (LIFE_DEC);
	u16 offset; //create the offset for the tanks
	for (; life != tank_lives; life += delta) {
		offset = TANK_LIFE_1 + life * TANK_SPACE; //calc x pos offset
		//draw black
		if (delta < 0) {
			draw_bitmap(bmp_tank_15x8, COLOR_BLACK, offset, TOP_SCREEN,
					TANK_WIDTH, TANK_HEIGHT);
		} else {
			draw_bitmap(bmp_tank_15x8, COLOR_GREEN, offset, TOP_SCREEN,
					TANK_WIDTH, TANK_HEIGHT);
		}
	}
}

//Externally accessible render function. Calls local functions to render graphics
void render(tank_t* tank, alien_block_t* alienBlock,
		alien_missiles_t* alien_missiles, bunker_t* bunkers, saucer_t* saucer,
		u8 tank_lives, u32 score) {

	update_alien_block(alienBlock);
	if(!game_controller_tank_life())
		update_tank(&tank->pos);

	update_missiles(tank, alien_missiles);

	update_bunkers(bunkers);

	update_saucer(&saucer->pos);

	update_tank_life_draw(tank_lives);

	update_score(score);
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

static bool check_point(s16 x, s16 y) {
	if (x < 0 || x >= GAME_W || y < 0 || y >= GAME_H)
		return false; //IF coordinates are outside bounds, do not draw
	//Scale x and y to screen coordinates
	s16 lx = x * RES_SCALE;
	s16 ly = y * RES_SCALE * SCREEN_W;
	u32 framepoint = frame0[lx + ly];
	xil_printf("Point hit: (%d,%d) %08x\n\r", x, y, framepoint);
	return !!frame0[lx + ly];
}

bool render_detect_collision(const u32* bmp, s16 x, s16 y, u16 h) {
	u16 local_y = h;
	u16 local_x;
	while (local_y-- > 0) {
		local_x = x;
		u32 bmp_row = bmp[local_y];
		for (; bmp_row; bmp_row >>= 1) {
			if ((bmp_row & BIT0) && check_point(local_x, local_y + y)) {
				return true;
			}
			local_x++;
		}
	}
	return false;
}

