TARGET = HarwareAlarmPoweroffSupport
OBJS = main.o exports.o sceCtrl_driver.o sceDisplay_driver.o
BUILD_PRX = 1
PRX_EXPORTS = exports.exp
USE_KERNEL_LIBS = 1
USE_KERNEL_LIBC = 1

PSP_FW_VERSION = 390

INCDIR =
CFLAGS = -O2 -G0 -Wall -fno-strict-aliasing -fno-builtin-printf
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpsprtc -lpspaudio
LDFLAGS = -mno-crt0 -nostartfiles

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
