/*
 * main.c: simple test application
 *
 * main file for startup. Inits platform and calls game_controller
 *
 *  Broderick Gardner
 *  Benjamin Gardner
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

int main() {
	init_platform(); // Necessary for all programs.

	render_init();

	// Oscillate between frame 0 and frame 1.
	// Just a cheap delay between frames.

	game_controller_init();
	while (1) {

		game_controller_run();
	}

	cleanup_platform();

	return 0;
}
