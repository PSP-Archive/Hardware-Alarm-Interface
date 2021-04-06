	.set noreorder

#include "pspimport.s"

	IMPORT_START	"sceAudio_driver",0x00010000
	IMPORT_FUNC	"sceAudio_driver",0xE4A9D621,sceAudioChReserve
	IMPORT_FUNC	"sceAudio_driver",0xC2031226,sceAudioChRelease
	IMPORT_FUNC	"sceAudio_driver",0x798FB2A3,sceAudioOutputBlocking