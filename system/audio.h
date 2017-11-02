/*
 * audio.h
 *
 *  Created on: Nov 1, 2017
 *      Author: superman
 */

#ifndef AUDIO_H_
#define AUDIO_H_

enum sound_select {
	NONE = 0, SHOOT = 1, MARCH = 2, EXPLODE = 3, SAUCER = 4, DIE = 5
};

//Public prototypes
void audio_init();
void audio_interrupt_handler();
void audio_play_sound(enum sound_select select);

#endif /* AUDIO_H_ */
