	.set noreorder

#include "pspimport.s"

	IMPORT_START	"sceCtrl_driver",0x00010000
	IMPORT_FUNC	"sceCtrl_driver",0xC4AAD55F,sceCtrl_driver_C4AAD55F // sceCtrlPeekBufferPositive
	IMPORT_FUNC	"sceCtrl_driver",0x454455AC,sceCtrl_driver_454455AC // sceCtrlReadBufferPositive 
	IMPORT_FUNC	"sceCtrl_driver",0x28E71A16,sceCtrl_driver_28E71A16 //sceCtrlSetSamplingMode 
	IMPORT_FUNC	"sceCtrl_driver",0x8182D8A0,sceCtrl_driver_8182D8A0 //PeekNeg
	IMPORT_FUNC	"sceCtrl_driver",0xFF847C31,sceCtrl_driver_FF847C31 //Readneg
