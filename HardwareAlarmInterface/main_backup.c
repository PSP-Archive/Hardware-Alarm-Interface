/*
    Copyright (C) 2008  Vivek Javvaji PSP Hardware Alarm Interface

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 Code "optimized" for readability.

 Credits to Moonlight and Silverspring for maintaining LibDocs
 */


#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <time.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include "graphics.h"
#include "intraFont.h"
#include "version.h"
#include "psprtc.h"
#include "pgeWav.h"

#define RGBA(r,g,b,a) ((r) | ((g)<<8) | ((b)<<16) | ((a)<<24))

PSP_MODULE_INFO("Hardware Alarm Interface", PSP_MODULE_USER, 1, 0);
PSP_HEAP_SIZE_MAX();

static int running = 1;


int exit_callback(int arg1, int arg2, void *common) {
	running = 0;
	return 0;
}

int CallbackThread(SceSize args, void *argp) {
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

int SetupCallbacks(void) {
	int thid = sceKernelCreateThread("CallbackThread", CallbackThread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
	if (thid >= 0) sceKernelStartThread(thid, 0, 0);
	return thid;
}

#define BLACK 0xff000000
#define WHITE 0xffffffff

//#define RELEASE_VERSION 0

void DuplicateTIMES(pspTime *Src,pspTime *Dst){
    Dst->hour=Src->hour;
    Dst->minutes=Src->minutes;
    Dst->seconds=Src->seconds;
}

int main() {

    #ifndef INITBlock //for convenient folding haha!

    SceCtrlData positive, lastpositive, presses;
  	pgeWav* AlarmSet;
  	intraFont* ltn[4];
  	Image* BGimage;

	pspDebugScreenInit();
	SetupCallbacks();

	pgeWavInit();
	AlarmSet=pgeWavLoad("AlarmSet.wav");

	if (!AlarmSet) { pspDebugScreenPrintf(" Error Loading Audiofile... Exiting"); sceKernelExitGame(); }

	BGimage = loadImage("./BG.png");
	if (!BGimage) { pspDebugScreenPrintf(" Error Loading BG image... Exiting"); sceKernelExitGame(); }

	int i;
    int fontNums[4]={0,4,8};

    #ifdef RELEASE_VERSION
    pspDebugScreenSetTextColor(0xff00ff00);
    pspDebugScreenPrintf("Open Syscon Alarm Interface v%d.%d bld %d Release %sM/%s/%s \n",MAJOR,MINOR,BUILD,MONTH,DATE,YEAR);
    pspDebugScreenSetTextColor(0xffffffff);
    pspDebugScreenPrintf("\n - Mr305 (Vivek Javvaji)\n");
    sceKernelDelayThread(2000000);
    #endif

    intraFontInit();

    char file[40]="";
    pspDebugScreenClear();
    for (i=0; i <=2; i++) {
        pspDebugScreenPrintf("Loading ltn%d.pgf...\n", fontNums[i]);
        sprintf(file, "flash0:/font/ltn%d.pgf", fontNums[i]);
        ltn[fontNums[i]] = intraFontLoad(file,INTRAFONT_CACHE_ASCII);
        if (!ltn[fontNums[i]])
        {
            pspDebugScreenPrintf(" \n\n Error Loading ltn%d... Exiting\n",fontNums[i]);
        sceKernelExitGame();
        }
    }

    pspDebugScreenPrintf("Kernel Lib: %08X",pspSdkLoadStartModule("KLIB.prx",PSP_MEMORY_PARTITION_KERNEL));

	initGraphics();

   pspTime currTime;
   pspTime currTime24;
   pspTime AlarmTime;
   pspTime AlarmTime24;
   pspTime *displayTime;

   int amTime=0;
    float currmillisecs=0.0f;
    char millisecStr[5]="";
    char timeStr[30]="";
    char secsStr[30]="";
    int showmillisecs=0;
    int timeDisplaymode=1;
    unsigned int timecolor=0;

	displayTime = &currTime; //show real time

    int currtimehour=0;
    int alarmhour=0;
    void* noflickerbase=0;


    #endif //JUST for convenience

	while(running)
	{
        {//SubInit
        sceKernelDcacheWritebackAll();
        guStart();
		clearScreen(0);//(0xffff9999)

        sceCtrlPeekBufferPositive(&positive,1);
        presses.Buttons = ~lastpositive.Buttons & positive.Buttons;
        lastpositive = positive;

         sceGuTexMode(GU_PSM_8888,0,0,GU_TRUE);
        blit(BGimage,0,0);

        int tick=0;

		intraFontSetStyle(ltn[4], 1.0f,0xff78e7e8,0x96000000,INTRAFONT_ALIGN_CENTER);
		intraFontPrint(ltn[4], 240, 45,"Hardware Alarm Interface");
        } //Subinit

         sceRtcGetCurrentClockLocalTime(&currTime24);

            if (displayTime->hour>12 || (displayTime->hour==12 && displayTime->minutes>1)){ //Decides AM/PM
            displayTime->hour-=12;
            if (timeDisplaymode) amTime=0;
            }
            else
            {
            if (timeDisplaymode)  amTime=1;
            }

        if (displayTime->hour<10 &&  displayTime->minutes<10) {
            sprintf(timeStr,"0%d : 0%d",displayTime->hour,displayTime->minutes);
        }
        else if (displayTime->hour<10) {
            if (displayTime->minutes<10)
            {
            sprintf(timeStr,"0%d : 0%d",displayTime->hour,displayTime->minutes);
            }
            else
            {
            sprintf(timeStr,"0%d : %d",displayTime->hour,displayTime->minutes);
            }
        }
        else if (displayTime->minutes<10) {
            sprintf(timeStr,"%d : 0%d",displayTime->hour,displayTime->minutes);
        }
        else
        sprintf(timeStr,"%d : %d",displayTime->hour,displayTime->minutes);
        //finish
        //color
        if (timeDisplaymode)
        timecolor=0xff0cfc3d; //green
        else
        timecolor=0xff0000ff; //red
        intraFontSetStyle(ltn[4], 1.8f,timecolor,0x64000000,INTRAFONT_ALIGN_CENTER);
        //color

        int oldsize=10;
        oldsize+=intraFontPrint(ltn[4], 240-20, 140, timeStr); //hours and minutes length adjusted print

        intraFontSetStyle(ltn[4], 1.2f,timecolor,0x64000000,0);

        if (showmillisecs && timeDisplaymode) { //millisecs & secs + alignment

        currmillisecs=((((currTime.microseconds*0.001f)/1000.0f)*100.0f)/100.0f)*100.0f;
        if (currmillisecs>99.0f) currmillisecs=99.0f; //fixes alignment/non - constant length issue
        sprintf(millisecStr,"%.0f",currmillisecs);

        if (strlen(millisecStr)==1) { //millsecs less than 10
            if (currTime.seconds<10)
            {
                oldsize=intraFontPrintf(ltn[4], oldsize, 140, "0%d.0%s",currTime.seconds,millisecStr);
            }
            else
            {
            oldsize=intraFontPrintf(ltn[4], oldsize, 140, "%d.0%s",currTime.seconds,millisecStr);
            }
        }
        else {
        if (displayTime->seconds<10)
            {
                oldsize=intraFontPrintf(ltn[4], oldsize, 140, "0%d.%s",displayTime->seconds,millisecStr);
            }
            else
            {
            oldsize=intraFontPrintf(ltn[4], oldsize, 140, "%d.%s",displayTime->seconds,millisecStr);
            }
        }
        }
        else if(!showmillisecs) { //seconds only alignment
            if (displayTime->seconds<10)
            {
                oldsize=intraFontPrintf(ltn[4], oldsize, 140, "0%d",displayTime->seconds);
            }
            else
            {
            oldsize=intraFontPrintf(ltn[4], oldsize, 140, "%d",displayTime->seconds);
            }
        }

       if (timeDisplaymode) { //am + pm
        intraFontSetStyle(ltn[0],0.7f,0xff000000,RGBA(0xff,0xff,0xff,50),INTRAFONT_ALIGN_CENTER);
        intraFontPrint(ltn[0],240,230,"Time View");

        intraFontSetStyle(ltn[8],1.4f,0xff000000,RGBA(0xff,0xff,0xff,50),0);
        if (amTime) intraFontPrint(ltn[8],oldsize+10,140,"AM");
        else intraFontPrint(ltn[8],oldsize+10,140,"PM");

       }
       else { //alarm time setting
           sceRtcGetCurrentClockLocalTime(&currTime); //get time in 24hr format

        currtimehour = currTime.hour;

        if (amTime) alarmhour=AlarmTime.hour;
        else
        alarmhour=AlarmTime.hour+12;

           intraFontSetStyle(ltn[0],0.7f,0xff000000,RGBA(0xff,0xff,0xff,50),INTRAFONT_ALIGN_CENTER);
        intraFontPrint(ltn[0],240,230,"Alarm Setup");

        int currtimeseconds=0;
        int alarmtimeseconds=0;
        float hoursrem=0;
        float minutesrem=0;

        currtimeseconds = (currtimehour * 3600) + (currTime.minutes * 60 ) + currTime.seconds;
        alarmtimeseconds = (alarmhour * 3600) + (AlarmTime.minutes * 60 ) + AlarmTime.seconds;

        hoursrem = floor((alarmtimeseconds-currtimeseconds)/3600.0f);
        minutesrem= ((alarmtimeseconds-currtimeseconds)/3600.0f - hoursrem)*60.0f;

        if (minutesrem==60.0f) { minutesrem=0.0f; hoursrem++; }

        //if (displayTime->hour==12 && currTime.hour<12 ) hoursrem-=12; //Hours Remaining Correction
        pspDebugScreenSetXY(0,0);
        pspDebugScreenSetOffset(noflickerbase); //no flicker for debug texxt
        pspDebugScreenPrintf(" A %d C %d",alarmhour,currtimehour);
        if (currtimehour == alarmhour && alarmhour == 24) hoursrem-=12;
        intraFontSetStyle(ltn[0],0.9f,0xFF16D1FC,RGBA(0xff,0xff,0xff,40),INTRAFONT_ALIGN_CENTER);
        intraFontPrintf(ltn[0], 240,175,"%.0f' %.0f'' remaining.",hoursrem,minutesrem);

        if (positive.Buttons & PSP_CTRL_UP)
        {
        AlarmTime.minutes++;
        if (AlarmTime.minutes==60)  {
            AlarmTime.minutes=0;
            AlarmTime.hour++;
            if (AlarmTime.hour == 12) amTime=!amTime;
            if (AlarmTime.hour == 13) { AlarmTime.hour=1; }
            }
            sceKernelDelayThread(100000);
        }
        else if (positive.Buttons & PSP_CTRL_DOWN)
        {
        if (AlarmTime.minutes == 0) {
            AlarmTime.minutes=59;
            if (AlarmTime.hour == 11 && AlarmTime.minutes==59 || AlarmTime.minutes==58 || AlarmTime.minutes==57) { amTime=!amTime; }
            if (AlarmTime.hour >0) AlarmTime.hour--;
            if (AlarmTime.hour ==0) AlarmTime.hour=12;
     }

        AlarmTime.minutes--;
        sceKernelDelayThread(100000);
        }

        intraFontSetStyle(ltn[8],1.4f,0xff000000,RGBA(0xff,0xff,0xff,50),0);
        if (amTime)
        intraFontPrint(ltn[8],oldsize+10,140,"AM");
        else
        intraFontPrint(ltn[8],oldsize+10,140,"PM");
       }

      // bottom info display

      intraFontSetStyle(ltn[8],0.8f,RGBA(0x9c,0x9c,0x9c,175),RGBA(0x00,0x00,0x00,40),0);
      if (timeDisplaymode) intraFontPrint(ltn[8],355,247,"TRIANGLE: Millisecs");
      else intraFontPrint(ltn[8],355,247,"SQUARE: AM/PM");
      intraFontPrint(ltn[8],355,264,"L/R: Switch Modes");
      // bottom info display finish

      if (presses.Buttons & PSP_CTRL_TRIANGLE && timeDisplaymode) showmillisecs=!showmillisecs;
      if (presses.Buttons & PSP_CTRL_SQUARE && !timeDisplaymode) amTime=!amTime;
      if (presses.Buttons & PSP_CTRL_LTRIGGER) {
          timeDisplaymode=1;
          displayTime = &currTime;
      }
      if (presses.Buttons & PSP_CTRL_RTRIGGER) {
           timeDisplaymode=0;
           displayTime = &AlarmTime;
           //if (amTime) AlarmTime.hour= currTime.hour;
           //else
           AlarmTime.hour= currTime.hour;//+12;

           if (currTime.minutes!=60)
           {
           AlarmTime.minutes=currTime.minutes+1;
           }
           else
           {
               AlarmTime.minutes=0;
               AlarmTime.hour++;
               if (AlarmTime.hour==13) AlarmTime.hour=1;
           }
           AlarmTime.seconds=currTime.seconds;
            }

        if (presses.Buttons & PSP_CTRL_CIRCLE && !timeDisplaymode )
        {

        writeAlarm305(readClock305()+ ((AlarmTime.minutes - currTime.minutes) * 60)*2  ) ; //2*3 = 3 seconds Correction
        //WAV_Play(AlarmSet);
        pgeWavPlay(AlarmSet);
        sceKernelDelayThread(100000);

        timeDisplaymode=1;
        displayTime = &currTime;

        scePowerRequestSuspend();
        }


    sceGuFinish();
    sceGuSync(0, 0);
    noflickerbase=flipScreen();
    sceDisplayWaitVblankStart();

	} //main loop


    intraFontUnload(ltn[0]);
    intraFontUnload(ltn[4]);
    intraFontUnload(ltn[8]);
    	intraFontShutdown();
	sceKernelExitGame();

	return 0;
}


//intraFontPrint(ltn[0], 180, 70, "regular, ");
        //intraFontPrint(ltn[4], 330, 70, "bold, ");//
        //intraFontPrint(ltn[8],  10, 70, "Latin Sans-Serif: ");//
