	.set noreorder

#include "pspimport.s"

	IMPORT_START	"sceDisplay_driver",0x00010000
	IMPORT_FUNC	"sceDisplay_driver",0xE56B11BA,sceDisplay_driver_E56B11BA
	IMPORT_FUNC	"sceDisplay_driver",0xDEA197D4,sceDisplay_driver_DEA197D4 //sceDisplayGetMode
