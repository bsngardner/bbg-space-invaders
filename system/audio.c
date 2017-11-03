/*
 * audio.c
 *
 *  Created on: Nov 1, 2017
 *      Author: superman
 */
#include <stdio.h>
#include "xil_types.h"
#include "xac97_l.h"
#include "xparameters.h"
#include "audio.h"
#include "sounds.h"

#define set_sample_rate(x) XAC97_WriteReg(XPAR_AXI_AC97_0_BASEADDR, AC97_PCM_DAC_Rate, x);
#define BASE XPAR_AXI_AC97_0_BASEADDR

#define MAX_AUDIO_STREAMS 5

//local prototypes
static void refresh_stream();

struct audio_stream {
	enum sound_select select;
	u32 index;
	struct sound* sound;
	struct audio_stream* next;
};

struct audio_stream stream_list[MAX_AUDIO_STREAMS];

struct audio_stream* active_stream;
struct audio_stream* inactive_stream;

static u32 sample_rate = 0;

void audio_init() {
	XAC97_HardReset(BASE);
	XAC97_InitAudio(BASE, 0);
	//XAC97_mSetControl(XPAR_AXI_AC97_0_BASEADDR, 1);
	XAC97_WriteReg(BASE, AC97_ExtendedAudioStat, 1);

	active_stream = 0;
	inactive_stream = stream_list;
	u16 i;
	for (i = 0; i < MAX_AUDIO_STREAMS; i++) {
		stream_list[i].index = 0;
		stream_list[i].select = 0;
		stream_list[i].sound = 0;
		stream_list[i].next = &stream_list[i + 1];
	}
	stream_list[i - 1].next = 0;

	XAC97_AwaitCodecReady(BASE);

	/** Play Volume Settings **/
	XAC97_WriteReg(BASE, AC97_MasterVol, AC97_VOL_MID);
	XAC97_WriteReg(BASE, AC97_AuxOutVol, AC97_VOL_MID);
	XAC97_WriteReg(BASE, AC97_MasterVolMono, AC97_VOL_MID);
	XAC97_WriteReg(BASE, AC97_PCBeepVol, AC97_VOL_MID);
	XAC97_WriteReg(BASE, AC97_PCMOutVol, AC97_VOL_MID);
	XAC97_WriteReg(BASE, AC97_LineInVol, AC97_VOL_MID);
	XAC97_WriteReg(BASE, AC97_MicVol, AC97_VOL_MID);

	/** Clear FIFOs **/
	XAC97_ClearFifos(BASE);
	XAC97_mSetControl(XPAR_AXI_AC97_0_BASEADDR, AC97_ENABLE_IN_FIFO_INTERRUPT);
}

void audio_play_sound(enum sound_select select) {
	struct audio_stream* current;
	struct audio_stream* stream = inactive_stream;
	if (stream == 0) {
		//Bad!
		print("Ran out of inactive sound streams!\n\r");
		return;
	}

	inactive_stream = inactive_stream->next;
	stream->index = 0;
	stream->select = select;
	stream->sound = sounds[select];

	current = active_stream;
	if (!current || stream->select > current->select) {
		stream->next = current;
		active_stream = stream;
		return;
	}
	while (current->next && (stream->select < current->select))
		current = current->next;
	stream->next = current->next;
	current->next = stream;
	return;
}

void audio_loop_sound(enum sound_select select) {

}

void audio_interrupt_handler() {

	struct audio_stream* stream = active_stream;
	xil_printf("FIFO: %d, %d\n\r",
			XAC97_getInFIFOLevel(XPAR_AXI_AC97_0_BASEADDR),
			(stream > 0) ? (1) : (0));

	if (stream && sample_rate != stream->sound->rate) {
		set_sample_rate(stream->sound->rate);
		sample_rate = stream->sound->rate;
	}

	xil_printf("stream: %d, full: %d\n\r", stream, !!XAC97_isInFIFOFull(BASE));
	while (!XAC97_isInFIFOFull(BASE)) {
		if (!stream) {
			XAC97_mSetInFifoData(BASE,0);
		} else {
			XAC97_mSetInFifoData(BASE,stream->sound->data[stream->index] << 8);
		}
		refresh_stream();
		stream = active_stream;

	}
}

static void refresh_stream() {
	struct audio_stream* stream = active_stream;
	struct audio_stream* prev = 0;
	while (stream) {
		stream->index++;
		if (stream->index >= stream->sound->frame_count) {
			if (prev == 0) {
				active_stream = stream->next;
				stream->next = inactive_stream;
				inactive_stream = stream;
				stream = active_stream;
				xil_printf("stream: %d, active_stream: %d, prev: %d\n\r",
						stream, active_stream, prev);
			} else {
				prev->next = stream->next;
				stream->next = inactive_stream;
				inactive_stream = stream;
				stream = prev->next;
			}
		} else {
			prev = stream;
			stream = stream->next;
		}
	}
}
