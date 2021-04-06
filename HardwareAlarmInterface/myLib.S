	.set noreorder

#include "pspstub.s"

	STUB_START "myLib",0x40090000,0x00030005
	STUB_FUNC  0xE7F858F1,readClock305
	STUB_FUNC  0x356BD342,writeAlarm305
	STUB_FUNC  0xFE1250CA,standby305
	STUB_END
