#include <stdlib.h>
#include <malloc.h>
#include <psputils.h>
#include "mikmod.h"

#include "wavloader.h"

#define MAX_MUSIC_CHAN 128
#define MAX_SFX_CHAN 32

void the_mikmod_error_handler(void)
{
	printf("_mm_critical %d\n", _mm_critical);
	printf("_mm_errno %d\n", _mm_errno);
	printf("%s\n", _mm_errmsg[_mm_errno]);
	return;
}

void WAV_Init() {
	_mm_RegisterErrorHandler(the_mikmod_error_handler);
	MikMod_RegisterAllLoaders();
	MikMod_RegisterAllDrivers();
	md_mode = DMODE_16BITS|DMODE_STEREO|DMODE_SOFT_SNDFX|DMODE_SOFT_MUSIC; 
	md_reverb = 0;
	md_pansep = 128;
	md_volume = 128;
	md_sndfxvolume = 128;
	MikMod_Init();
	MikMod_SetNumVoices(MAX_MUSIC_CHAN, MAX_SFX_CHAN);
	Player_Start(0);
}

void WAV_End() {
	Player_Stop();
	MikMod_Exit();
}

WAV* WAV_Load(char* filename) {
	WAV* wavfile = (WAV*) malloc(sizeof(WAV));
	if (!wavfile) return NULL;
        wavfile->sample = WAV_LoadFN(filename);
        wavfile->panning = 128;
        wavfile->volume = 255;
        return wavfile;
}

void WAV_Unload(WAV* wavfile) {
	WAV_Free(wavfile->sample);
	free(wavfile);
}

void WAV_Play(WAV* wavfile) {
	if(wavfile) {
		wavfile->voice = MikMod_PlaySample(wavfile->sample,0,0);
		Voice_SetPanning(wavfile->voice, wavfile->panning);
		Voice_SetVolume(wavfile->voice, wavfile->volume);
	}
}

void WAV_Stop(WAV* wavfile) {
        if(wavfile) {
                Voice_Stop(wavfile->voice);
        }
}

void WAV_SetVolume(WAV* wavfile, int vol) {
	if(vol > 255) vol = 255;
        else if(vol < 0) vol = 0;
	wavfile->volume = vol;
}

void WAV_SetPan(WAV* wavfile, int pan) {
	if(pan > 255) pan = 255;
	else if(pan < 0) pan = 0;
	wavfile->panning = pan;
}
