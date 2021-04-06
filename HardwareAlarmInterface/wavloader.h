#ifndef __WAVLOADER__
#define __WAVLOADER__

// SDK includes
#include <psptypes.h>

// MikMod includes
#include "mikmod.h"

typedef struct
{
  SAMPLE* sample;
  int voice;
  int panning;
  int volume;
} WAV;

extern int _mm_errno;
extern BOOL _mm_critical;
extern char *_mm_errmsg[];

extern UWORD md_mode;
extern UBYTE md_sndfxvolume;
extern UBYTE md_reverb;
extern UBYTE md_pansep;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WAV_Init
//
// Usage: WAV_Init();
// Initialises MikMod
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void WAV_Init();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WAV_End
//
// Usage: WAV_End;
// De-Initialises MikMod. Call before you exit your program
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void WAV_End();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WAV_Load
//
// Usage: WAV* ourWav = WAV_Load("./sound.wav");
// Returns a pointer to a newly created WAV struct
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern WAV* WAV_Load(char* filename);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WAV_Unload
//
// Usage: WAV_Unload(ourWav);
// Requires a valid pointer to a WAV struct
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void WAV_Unload(WAV* wavfile);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WAV_Play
//
// Usage: WAV_Play(ourWav);
// Requires a valid pointer to a WAV struct, MikMod only supports MONO files!
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void WAV_Play(WAV* wavfile);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WAV_Stop
//
// Usage: WAV_Stop(ourWav);
// Requires a valid pointer to a WAV struct
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void WAV_Stop(WAV* wavfile);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WAV_SetVolume
//
// Usage: WAV_SetVolume(ourWav, 255);
// Requires a valid pointer to a WAV struct
// vol is an integer 0-255, 0 being the lowest and 255 being the highest
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void WAV_SetVolume(WAV* wavfile, int vol);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WAV_SetPan
//
// Usage: WAV_SetPan(ourWav, 127);
// Requires a valid pointer to a WAV struct
// pan is an integer 0-255, 0 being fully left, 255 being fully right, 128 being centred
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern void WAV_SetPan(WAV* wavfile, int pan);

#endif
