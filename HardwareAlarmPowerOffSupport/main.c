#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <pspaudio.h>

PSP_MODULE_INFO("HardwareAlarmPoweroffSupport", 0x1000, 1, 0);
PSP_MAIN_THREAD_ATTR(0);

#define sceCtrlPeekBufferPositive sceCtrl_driver_C4AAD55F
#define sceCtrlSetSamplingMode sceCtrl_driver_28E71A16

int main_thread(SceSize args, void *argp)
{
    sceKernelDelayThread(7000000);
    pspTime timea;
    int fid=0;
    char temp[5]="";
    int alarmnumhrs=0;
    int alarmnummins=0;
    fid = sceIoOpen("ms0:/seplugins/HAPoweroffSupport.305",PSP_O_RDONLY,0777);
    if (fid<0) sceKernelExitDeleteThread(0);
    sceIoLseek(fid,0,SEEK_SET);
    sceIoRead(fid,&temp,2);
    alarmnumhrs=strtoul(temp,NULL,0);
    sceIoRead(fid,&temp,2);
    alarmnummins=strtoul(temp,NULL,0);
    sceIoClose (fid);

  unsigned char Sample[1770] __attribute__((aligned(64)));

    int filesize=0;
    fid = sceIoOpen("ms0:/seplugins/alarm.wav", PSP_O_RDONLY, 0777);
    filesize = sceIoLseek(fid, 0, SEEK_END);
    sceIoLseek(fid, 0, SEEK_SET);
    sceIoRead( fid,Sample, filesize);
    sceRtcGetCurrentClockLocalTime(&timea);
   sceIoClose (fid); //Close the file
   SceCtrlData pad;
   int audichannel=0;
   int i=0;
    unsigned int* frambuf;
    int bufferwidth=0;
	int pixelformat=0;
    int unkas=0;
    int x,y=0;
    int doneres=0;
    static int ret=0;

   while (1)
   {
       sceCtrlPeekBufferPositive(&pad,1);
        if ((alarmnummins == (int)(timea.minutes + 1) ) || (alarmnummins == (int)timea.minutes ))
        {
            if (alarmnumhrs == (int)timea.hour) {
            if (doneres==0) {
                sceKernelDelayThread(100000);
                audichannel=sceAudioChReserve(-1,1152,0x10);
                doneres = 1;
                sceDisplay_driver_E56B11BA(&frambuf,&bufferwidth,&pixelformat,&unkas);
                }

    //no optimization -- written for easy interpretation & customization

    for(x=220;x<260;x++) //up
                    {
                    for(y=116;y<119;y++)
                      {
                        frambuf[y*512+x]=0xFF00FF00;
                      }
                    }
            for(x=220;x<223;x++) //left
                    {
                    for(y=116;y<151;y++)
                      {
                        frambuf[y*512+x]=0xFF00FF00;
                      }
                    }
            for(x=220;x<260;x++) //base
                    {
                    for(y=148;y<151;y++)
                      {
                        frambuf[y*512+x]=0xFF00FF00;
                      }
                    }
            for(x=257;x<260;x++) //right
                    {
                    for(y=116;y<151;y++)
                      {
                        frambuf[y*512+x]=0xFF00FF00;
                      }
                    }
            for(x=240;x<241;x++) //minutes hand
                    {
                    for(y=121;y<135;y++)
                      {
                        frambuf[y*512+x]=0xFF00FF00;
                      }
                   }
                for(x=240;x<250;x++) //hours hand
                    {
                    for(y=134;y<136;y++)
                      {
                        frambuf[y*512+x]=0xFF00FF00;
                      }
                    }
                for(x=236;x<245;x++) //Alarm button-ish
                    {
                    for(y=111;y<116;y++)
                      {
                        frambuf[y*512+x]=0xFF00FF00;
                      }
                    }

        i++;
        if (i<35)
        {
                sceAudioOutputBlocking(audichannel,0x8000,Sample);
        }
        if (i==35) {
            sceKernelDelayThread(500000);
            i=0;
        }
        if (pad.Buttons & PSP_CTRL_SQUARE) {  sceAudioChRelease(audichannel); sceKernelExitDeleteThread(0); }

        }
        }
         else
        {
       sceKernelExitDeleteThread(0);
        }
    sceKernelDelayThread(10);

    } //loop
}

int module_start(SceSize args, void *argp)
{
   SceUID th = sceKernelCreateThread("main_thread", main_thread, 0x20, 0x10000, 0, NULL);
	if (th >= 0)
	{
		sceKernelStartThread(th, args, argp);
	}
	return 0;
}

int module_stop()
{
    return 1;
}

//_sysconReadClock(&ret);
 //               if (pad.Buttons & PSP_CTRL_TRIANGLE) { doneres=_sysconWriteAlarm(ret+0x78); sceKernelDelayThread(1000000); _sysconStandBy(); }
