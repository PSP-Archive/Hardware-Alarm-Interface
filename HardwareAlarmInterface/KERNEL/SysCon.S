   .set noreorder

#include "pspstub.s"

   STUB_START   "sceSyscon_driver",0x00010000,0x00020005
	STUB_FUNC	0x6C911742,_sysconWriteAlarm
	STUB_FUNC	0xC4D66C1D,_sysconReadClock
	STUB_FUNC	0xC8439C57,_sysconStandBy
   STUB_END

