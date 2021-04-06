TARGET = HardwareAlarmInterface
OBJS = HaInterface.o graphics.o framebuffer.o intraFont.o myLib.o pgeWav.o mp3player.o

BUILD_PRX = 1

INCDIR =
CFLAGS = -O2 -G0 -Wall -g
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS = -lpspgum -lpspgu -lpng -lz -lm -lpsprtc -lmad -lpspaudiolib -lpspaudio -lpspvram -lpsppower

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_ICON= ICON0.PNG
PSP_EBOOT_TITLE = PSP Hardware Alarm Interface

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
