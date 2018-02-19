/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <stdio.h>
#include <switch.h>
#include "quakedef.h"

#define u64 uint64_t
#define u8 uint8_t

#define SAMPLE_RATE   48000
#define AUDIOSIZE   16384

u8 *audiobuffer;
uint64_t initial_tick;
int snd_inited;
int chn = -1;
int stop_audio = false;
//int update = false;
float tickRate;

bool SNDDMA_Init(void)
{
	snd_initialized = 0;

	audiobuffer = malloc(AUDIOSIZE);

	/* Fill the audio DMA information block */
	shm = &sn;
	shm->splitbuffer = 0;
	shm->samplebits = 16;
	shm->speed = SAMPLE_RATE;
	shm->channels = 1;
	shm->samples = AUDIOSIZE / (shm->samplebits / 8);
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = audiobuffer;
	
	tickRate = 5.208333f;
	
	audoutInitialize();
	audoutStartAudioOut();
    
	initial_tick = svcGetSystemTick();

	snd_initialized = 1;
	return 1;
}

int SNDDMA_GetDMAPos(void)
{
	if(!snd_initialized)
		return 0;

	uint64_t tick = svcGetSystemTick();
	const unsigned int deltaTick  = tick - initial_tick;
	const float deltaSecond = deltaTick * tickRate;
	u64 samplepos = deltaSecond * SAMPLE_RATE;
	shm->samplepos = samplepos;
	return samplepos;
}

void SNDDMA_Shutdown(void)
{
  if(snd_initialized){
	stop_audio = true;
	chn = -1;
  }
}

/*
==============
SNDDMA_Submit
Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
    AudioOutBuffer source_buffer;
    AudioOutBuffer released_buffer;
    
    source_buffer.next = 0;
    source_buffer.buffer = audiobuffer;
    source_buffer.buffer_size = AUDIOSIZE;
    source_buffer.data_size = SAMPLE_RATE / 5;
    source_buffer.data_offset = 0;

    audoutPlayBuffer(&source_buffer, &released_buffer);
}