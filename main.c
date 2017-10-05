/*
 * helloworld.c: simple test application
 */

#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xaxivdma.h"
#include "xio.h"
#include "time.h"
#include "unistd.h"
#include "render.h"
#include "game_controller.h"

#define DEBUG
void print(char *str);

#define MAX_SILLY_TIMER 10000000;

int main() {
	init_platform(); // Necessary for all programs.

	render_init();

	// Oscillate between frame 0 and frame 1.
	int sillyTimer = MAX_SILLY_TIMER; // Just a cheap delay between frames.
	game_controller_init();
	while (1) {
		alien_block_t block = { { 5, 5 }, { 0 }, OUT };
		memset(block.alien_status, 1, 55);
		render(0, 0, &block, 0, 0);

		while (1) {
			while (sillyTimer)
				sillyTimer--; // Decrement the timer.
			sillyTimer = MAX_SILLY_TIMER; // Reset the timer.
			block.pos.x += 2;
			render(0, 0, &block, 0, 0);
		}
		game_controller_run();

	}

	cleanup_platform();

	return 0;
}
