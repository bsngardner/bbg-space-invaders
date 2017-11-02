/*
 * sounds.h
 *
 *  Created on: Nov 1, 2017
 *      Author: superman
 */

#ifndef SOUNDS_H_
#define SOUNDS_H_

#include "xil_types.h"

struct sound {
	u16 rate;
	u16 frame_count;
	u8* data;
};

extern struct sound* sounds[];

#endif /* SOUNDS_H_ */
