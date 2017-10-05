
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
	//int sillyTimer = MAX_SILLY_TIMER; // Just a cheap delay between frames.
	game_controller_init();
	while (1) {
		game_controller_run();
		//while (sillyTimer)
		//	sillyTimer--; // Decrement the timer.
		//sillyTimer = MAX_SILLY_TIMER; // Reset the timer.
		render(0, 0, 0, 0, 0);

	}

	cleanup_platform();

	return 0;
}
