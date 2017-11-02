/*
 * polling.c
 *
 *  Created on: Nov 1, 2017
 *      Author: superman
 */
#include "xac97_l.h"
#include "xparameters.h"
#include "sounds.h"
#include <stdio.h>
extern int32_t tankFireSound[];
//#define TRUE 1

void playAudio(Xuint32 BaseAddress, Xuint32 StartAddress, Xuint32 EndAddress);

int main() {
	XAC97_HardReset(XPAR_AXI_AC97_0_BASEADDR);
	XAC97_InitAudio(XPAR_AXI_AC97_0_BASEADDR, 0);
	//XAC97_mSetControl(XPAR_AXI_AC97_0_BASEADDR, 1);
	XAC97_WriteReg(XPAR_AXI_AC97_0_BASEADDR, AC97_ExtendedAudioStat, 1);
	XAC97_WriteReg(XPAR_AXI_AC97_0_BASEADDR, AC97_PCM_DAC_Rate, 11127);
	//xil_printf("tank sound address: %04x\r\n", tankFireSound);

	while (TRUE) {
		playAudio(XPAR_AXI_AC97_0_BASEADDR, &tankFireSound,
				&tankFireSound[4080]);

		XAC97_Delay(1000000);
	}
}

void playAudio(Xuint32 BaseAddress, Xuint32 StartAddress, Xuint32 EndAddress) {
	Xuint32 i;
	Xuint32 sample;
	volatile Xuint32 *sound_ptr = (Xuint32*) StartAddress;

	/** Wait for the ready signal **/
	XAC97_AwaitCodecReady(BaseAddress);

	/** Disable VRA Mode **/
	XAC97_WriteReg(BaseAddress, AC97_ExtendedAudioStat, 1);

	/** Play Volume Settings **/
	XAC97_WriteReg(BaseAddress, AC97_MasterVol, AC97_VOL_MID);
	XAC97_WriteReg(BaseAddress, AC97_AuxOutVol, AC97_VOL_MID);
	XAC97_WriteReg(BaseAddress, AC97_MasterVolMono, AC97_VOL_MID);
	XAC97_WriteReg(BaseAddress, AC97_PCBeepVol, AC97_VOL_MID);
	XAC97_WriteReg(BaseAddress, AC97_PCMOutVol, AC97_VOL_MID);
	XAC97_WriteReg(BaseAddress, AC97_LineInVol, AC97_VOL_MID);
	XAC97_WriteReg(BaseAddress, AC97_MicVol, AC97_VOL_MID);

	/** Clear FIFOs **/
	XAC97_ClearFifos(BaseAddress);

	while (sound_ptr < (Xuint32*) EndAddress) {
		sample = *sound_ptr;
		sound_ptr = sound_ptr + 1;
		XAC97_WriteFifo(BaseAddress, sample);
	}

	XAC97_ClearFifos(BaseAddress);

} // end XAC97_PlayAudio()
