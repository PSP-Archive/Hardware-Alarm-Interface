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
#include <pspaudio.h>
#include <pspaudiolib.h>
#include "mp3player.h"

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

#define RELEASE_VERSION 0
#define HAInterface main

void DuplicateTIMES(pspTime *Src,pspTime *Dst){
    Dst->hour=Src->hour;
    Dst->minutes=Src->minutes;
    Dst->seconds=Src->seconds;
}

int HAInterface() {

    #ifndef INITBlock

    SceCtrlData positive, lastpositive, presses;
  	pgeWav* AlarmSet;
  	intraFont* ltn[4];
  	Image* BGimage;

    pspTime currTime,currTime24,AlarmTime,AlarmTime24,*displayTime,*displayTime24;
    int TimeAM=0,AlarmAM=0,*displayAM;
    int AlarmSetup=0;
    int alarmsethr=0;
    int alarmsetmin=0;
    int alarmsetsec=0;

   	displayTime24 = &currTime24; //show real time
   	displayTime=&currTime;
   	displayAM = &TimeAM; //point to time AM/PM

    int RightNowAlarm=0;
    float currmillisecs=0.0f;
    char millisecStr[5]="";
    char timeStr[30]="";
    char secsStr[30]="";
    int showmillisecs=0;
    int timeDisplaymode=1;
    unsigned int timecolor=0;

    int currtimehour=0;
    int alarmhour=0;
    void* noflickerbase=0;

    int fid=0;
    int onceonly=0,copyalarmhr=0,copyalarmmin=0;
    int standbyRetcode=0,writealarmRetcode=0;

   	int i;
    int fontNums[4]={0,4,8};

    int currtimeseconds=0,alarmtimeseconds=0;
    float hoursrem=0,minutesrem=0;

	pspDebugScreenInit();
	SetupCallbacks();

	pspAudioInit();
	pgeWavInit();
	AlarmSet=pgeWavLoad("AlarmSet.wav");

	if (!AlarmSet) { pspDebugScreenPrintf(" Error Loading Audiofile... Exiting"); sceKernelExitGame(); }

	BGimage = loadImage("./BG.png");
	if (!BGimage) { pspDebugScreenPrintf(" Error Loading BG image... Exiting"); sceKernelExitGame(); }
    again:
    #ifdef RELEASE_VERSION
    pspDebugScreenClear();
    sceCtrlPeekBufferPositive(&positive,1);
    pspDebugScreenSetTextColor(0xff00ff00);
    pspDebugScreenPrintf("\n\n Syscon Alarm Interface v%d.%d bld %d Release %s/%sth/%s \n",MAJOR,MINOR,BUILD,MONTH,DATE,YEAR);
    pspDebugScreenSetTextColor(0xffffffff);
    pspDebugScreenPrintf("\n  - Vivek Javvaji (mistr305@gmail.com)\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    pspDebugScreenSetTextColor(0xff00ffcc); //ccff00
    pspDebugScreenPrintf(" If you like this app, Please donate a small amount of ~$10.\n\n (Will aid in purchase of Hardware Peripherals for Development.)");
    sceKernelDelayThread(6000000);
    if (positive.Buttons & PSP_CTRL_LTRIGGER) goto again;
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

     MP3_Init(1);
          MP3_Load("Alarm.mp3");
    #endif
	while(running) {
        { //SubInit
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

        if (RightNowAlarm != 0) { //resumed from sleep mode - Alarm should play. For now, Time Check isn't required - Just dont flick switch at the wrong time.
            MP3_Play();
            if (positive.Buttons & PSP_CTRL_CIRCLE && timeDisplaymode) {
                AlarmSetup=alarmsethr=alarmsetmin=alarmsetsec=0;
                RightNowAlarm=0;
                MP3_Pause();
            }
        }

         sceRtcGetCurrentClockLocalTime(&currTime24);

         DuplicateTIMES(&currTime24,&currTime);
         if (!timeDisplaymode) DuplicateTIMES(&AlarmTime24,&AlarmTime);

            if (displayTime24->hour>12 || (displayTime24->hour==12 && displayTime24->minutes>1)) { //Decides AM/PM
            displayTime->hour-=12;
            if (timeDisplaymode) TimeAM=0;
            else AlarmAM=0;
            }
            else {
            if (timeDisplaymode)  TimeAM=1;
            else AlarmAM=1;
            }
            if (displayTime->hour==0) displayTime->hour=12;

        { // Time/Alarm Time Constant Length Adjustment
        if (displayTime->hour<10 &&  displayTime->minutes<10) {
            sprintf(timeStr,"0%d : 0%d",displayTime->hour,displayTime->minutes);
        }
        else if (displayTime->hour<10) {
            if (displayTime->minutes<10) {
            sprintf(timeStr,"0%d : 0%d",displayTime->hour,displayTime->minutes);
            }
            else {
            sprintf(timeStr,"0%d : %d",displayTime->hour,displayTime->minutes);
            }
        }
        else if (displayTime->minutes<10) {
            sprintf(timeStr,"%d : 0%d",displayTime->hour,displayTime->minutes);
        }
        else
        sprintf(timeStr,"%d : %d",displayTime->hour,displayTime->minutes);
        } // Time/Alarm Time Constant Length Adjustment

///DEBUG
        intraFontSetStyle(ltn[8],0.6f,RGBA(0x9c,0x9c,0x9c,175),RGBA(0x00,0x00,0x00,40),0);
        intraFontPrintf(ltn[8],10,10,"C24 %d C %d A24 %d A %d",currTime24.hour,currTime.hour,AlarmTime24.hour,AlarmTime.hour);
        if (standbyRetcode!=0 || writealarmRetcode!=0) intraFontPrintf(ltn[8],10,20,"0x%X %d",writealarmRetcode);
        if (AlarmSetup) { //alarm set - alarm time display
            intraFontSetStyle(ltn[8],1.0f,RGBA(0xff,0x00,0x00,200),RGBA(0x00,0x00,0x00,70),0);
            if (AlarmSetup==1) intraFontPrintf(ltn[8],400,72,"%d:%d.%d AM",alarmsethr,alarmsetmin,alarmsetsec);
            else intraFontPrintf(ltn[8],400,72,"%d:%d.%d PM",alarmsethr,alarmsetmin,alarmsetsec);
        }

        { //Display Color
        if (timeDisplaymode)
        timecolor=0xff0cfc3d; //green
        else
        timecolor=0xff0000ff; //red
        intraFontSetStyle(ltn[4], 1.8f,timecolor,0x64000000,INTRAFONT_ALIGN_CENTER);
        } //color

        int oldsize=10;
        oldsize+=intraFontPrint(ltn[4], 240-20, 140, timeStr); //hours and minutes length adjusted print

        intraFontSetStyle(ltn[4], 1.2f,timecolor,0x64000000,0);

        if (showmillisecs && timeDisplaymode) { //msecs & secs length fix - Only for Time mode

            currmillisecs=((((currTime24.microseconds*0.001f)/1000.0f)*100.0f)/100.0f)*100.0f;
            if (currmillisecs>99.0f) currmillisecs=99.0f; //fixes alignment/non - constant length issue
            sprintf(millisecStr,"%.0f",currmillisecs);

            if (strlen(millisecStr)==1) { //millsecs less than 10
                if (currTime.seconds<10)
                    oldsize=intraFontPrintf(ltn[4], oldsize, 140, "0%d.0%s",currTime.seconds,millisecStr);
                else
                    oldsize=intraFontPrintf(ltn[4], oldsize, 140, "%d.0%s",currTime.seconds,millisecStr);
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
        else if(!showmillisecs) { //secs length
            if (displayTime->seconds<10)
                oldsize=intraFontPrintf(ltn[4], oldsize, 140, "0%d",displayTime->seconds);
            else
                oldsize=intraFontPrintf(ltn[4], oldsize, 140, "%d",displayTime->seconds);
        }

       if (timeDisplaymode) { //am + pm
        intraFontSetStyle(ltn[0],0.7f,0xff000000,RGBA(0xff,0xff,0xff,50),INTRAFONT_ALIGN_CENTER);
        intraFontPrint(ltn[0],240,230,"Time View");

        intraFontSetStyle(ltn[8],1.4f,0xff000000,RGBA(0xff,0xff,0xff,50),0);
        if (TimeAM) intraFontPrint(ltn[8],oldsize+10,140,"AM");
        else intraFontPrint(ltn[8],oldsize+10,140,"PM");

       }
       else { //alarm time setting

        intraFontSetStyle(ltn[0],0.7f,0xff000000,RGBA(0xff,0xff,0xff,50),INTRAFONT_ALIGN_CENTER);
        intraFontPrint(ltn[0],240,230,"Alarm Setup");

        currtimehour = currTime24.hour;
        alarmhour = AlarmTime24.hour;


        currtimeseconds = (currtimehour * 3600) + (currTime.minutes * 60 ) + currTime.seconds;

        if (alarmhour >= currtimehour)
        alarmtimeseconds = (alarmhour * 3600) + (AlarmTime.minutes * 60 ) + AlarmTime.seconds;
        else
        alarmtimeseconds = (alarmhour * 3600) + (AlarmTime.minutes * 60 ) + AlarmTime.seconds+(24*3600);


        hoursrem = floor((alarmtimeseconds-currtimeseconds)/3600.0f);
        minutesrem= ((alarmtimeseconds-currtimeseconds)/3600.0f - hoursrem)*60.0f;

        if (minutesrem==60.0f) { minutesrem=0.0f; hoursrem++; }

        intraFontSetStyle(ltn[0],0.9f,0xFF16D1FC,RGBA(0xff,0xff,0xff,40),INTRAFONT_ALIGN_CENTER);
        if (hoursrem<0 || minutesrem < 0)
         intraFontPrintf(ltn[0], 240,175,"%.0f hours %.0f'' remaining. (Invalid - Readjust)",hoursrem,minutesrem);
         else
        intraFontPrintf(ltn[0], 240,175,"%.0f hours %.0f'' remaining.",hoursrem,minutesrem);

        if (positive.Buttons & PSP_CTRL_UP)
        {
            if (positive.Buttons & PSP_CTRL_TRIANGLE) sceKernelDelayThread(100000);
            AlarmTime24.minutes++;
            if (AlarmTime24.hour == 24 && AlarmTime24.minutes>0) AlarmTime24.hour=0;
            if (AlarmTime24.minutes==60)  {
                AlarmTime24.minutes=0;
                AlarmTime24.hour++;
            }
        }
        else if (positive.Buttons & PSP_CTRL_DOWN)
        {
            if (positive.Buttons & PSP_CTRL_TRIANGLE) sceKernelDelayThread(100000);
            if (AlarmTime24.minutes == 0) {
                AlarmTime24.minutes=59;
                if (AlarmTime24.hour>0) AlarmTime24.hour--;
                if (AlarmTime24.hour==0) AlarmTime24.hour = 23;
            }

            AlarmTime24.minutes--;
        }

        if (presses.Buttons & PSP_CTRL_SQUARE) {
           DuplicateTIMES(&currTime24,&AlarmTime24);
           DuplicateTIMES(&AlarmTime24,&AlarmTime);

           if (currTime24.minutes<60)
           {
           AlarmTime24.minutes=currTime24.minutes+1;
           }
           else
           {
               AlarmTime24.minutes=0;
               AlarmTime24.hour++;
               if (AlarmTime24.hour==25) AlarmTime24.hour=0;
           }
           AlarmTime24.seconds=currTime24.seconds;

           displayTime24 = &AlarmTime24;
           displayTime = &AlarmTime;
        pgeWavLoop(AlarmSet,0);
        pgeWavPlay(AlarmSet);
        sceKernelDelayThread(700000);
        }

        intraFontSetStyle(ltn[8],1.4f,0xff000000,RGBA(0xff,0xff,0xff,50),0);
        if (AlarmAM)
        intraFontPrint(ltn[8],oldsize+10,140,"AM");
        else
        intraFontPrint(ltn[8],oldsize+10,140,"PM");
       }


      {// bottom info display
      intraFontSetStyle(ltn[8],0.8f,RGBA(0x9c,0x9c,0x9c,205),RGBA(0x00,0x00,0x00,40),0);
      if (timeDisplaymode)
      {
      intraFontPrint(ltn[8],355,247,"RTRIG: Alarm Set Mode");
      if (AlarmSetup) intraFontPrint(ltn[8],25,247,"START: Sleep(w/MP3 Alarm)");
      if (AlarmSetup) intraFontPrint(ltn[8],25,264,"SELECT: Poweroff(Standby) Alarm");
      intraFontPrint(ltn[8],355,264,"TRIANGLE: Millisecs");
      }
      else
      {
       intraFontPrint(ltn[8],355,247,"LTRIG: Time Display");
       intraFontPrint(ltn[8],355,264,"TRIANGLE: Slower");
       intraFontPrint(ltn[8],25,264,"CIRCLE: Set Alarm");
       intraFontPrint(ltn[8],25,247,"SQUARE: Current time");
      }

      }// bottom info display finish

      if (presses.Buttons & PSP_CTRL_TRIANGLE && timeDisplaymode) showmillisecs=!showmillisecs;
      if (presses.Buttons & PSP_CTRL_LTRIGGER) {
          timeDisplaymode=1;
          displayTime24 = &currTime24;
          displayTime = &currTime;
      }
      if (presses.Buttons & PSP_CTRL_RTRIGGER) {
            timeDisplaymode=0;

           DuplicateTIMES(&currTime24,&AlarmTime24);
           DuplicateTIMES(&AlarmTime24,&AlarmTime);

           displayTime24 = &AlarmTime24;
           displayTime = &AlarmTime;

           if (onceonly==0)
        {
        fid=sceIoOpen("ms0:/seplugins/HAPoweroffSupport.305",PSP_O_RDONLY,0777);
        if (fid>0)
        {
        char temp[6]="";

        sceIoRead(fid,&temp,2);
        copyalarmhr=AlarmTime24.hour=strtoul(temp,NULL,0);
        sceIoRead(fid,&temp,2);
        copyalarmmin=AlarmTime24.minutes=strtoul(temp,NULL,0);
        sceIoClose (fid);
        onceonly=3;
        DuplicateTIMES(&AlarmTime24,&AlarmTime);
        displayTime24 = &AlarmTime24;
           displayTime = &AlarmTime;
        }
        else
        {
        onceonly=-3;
        }
        }

           switch (onceonly) //time config doesn't exist
            {
                case -3:
                   if (currTime24.minutes<60)
                   {
                   AlarmTime24.minutes=currTime24.minutes+1;
                   AlarmTime24.hour = currTime24.hour;
                   }
                   else
                   {
                       AlarmTime24.minutes=0;
                       AlarmTime24.hour++;
                       if (AlarmTime24.hour==25) AlarmTime24.hour=0;
                   }
                   AlarmTime24.seconds=currTime24.seconds;
                   break;

                case 3:
                   AlarmTime24.hour = copyalarmhr;
                   AlarmTime24.minutes = copyalarmmin;
                   AlarmTime24.seconds=00;
                   break;
                default:
                    break;
           }

      }
      if (presses.Buttons & PSP_CTRL_CIRCLE && !timeDisplaymode ) {
        const int AlarmTimeNegotiate =  -2 * 2 ; //2 Seconds

        writealarmRetcode=writeAlarm305(AlarmTimeNegotiate+ readClock305()+\
        (((int)hoursrem*3600)+((int)minutesrem*60)+((60-currTime24.seconds)+AlarmTime24.seconds))*2);
//        (((AlarmTime24.hour * 3600) + (AlarmTime24.minutes * 60) + AlarmTime24.seconds) -\
//        ((currTime24.hour * 3600) + (currTime24.minutes * 60) + currTime24.seconds)) * 2 ) ;
//        ((AlarmTime.minutes - currTime.minutes) * 60)*2
        //WAV_Play(AlarmSet);

        pgeWavLoop(AlarmSet,0);
        pgeWavPlay(AlarmSet);
        sceKernelDelayThread(700000);

        timeDisplaymode=1;
        displayTime24 = &currTime24;
        displayTime = &currTime;

        if (AlarmAM) AlarmSetup=1; //am
        else AlarmSetup=2; //pm

        alarmsethr=AlarmTime.hour;
        alarmsetmin=AlarmTime.minutes;
        alarmsetsec=AlarmTime.seconds;

        }
      if (presses.Buttons & PSP_CTRL_START && AlarmSetup) {
        pgeWavLoop(AlarmSet,0);
        pgeWavPlay(AlarmSet);
        sceKernelDelayThread(700000);
        RightNowAlarm=25;
        scePowerRequestSuspend();
        sceKernelDelayThread(1000000);
        }

      int retcodea=standby305();
        if (presses.Buttons & PSP_CTRL_LEFT)
        {
            pspDebugScreenSetXY(0,0);
            pspDebugScreenPrintf("%d %X",retcodea,retcodea);
            sceKernelDelayThread(3000000);
        }

      if (presses.Buttons & PSP_CTRL_SELECT && AlarmSetup) {

        fid = sceIoOpen("ms0:/seplugins/HAPoweroffSupport.305",PSP_O_CREAT|PSP_O_TRUNC|PSP_O_WRONLY,0777);
        char temp[6]="";

        if (AlarmTime24.hour<10)
        sprintf(temp,"0%d",AlarmTime24.hour);
        else
        sprintf(temp,"%d",AlarmTime24.hour);

        sceIoWrite(fid,&temp,2);

        if (AlarmTime24.minutes<10)
        sprintf(temp,"0%d",AlarmTime24.minutes);
        else
        sprintf(temp,"%d",AlarmTime24.minutes);

        sceIoWrite(fid,&temp,2);
        sceIoClose (fid);
        sceKernelDelayThread(1000000);

        pgeWavLoop(AlarmSet,0);
        pgeWavPlay(AlarmSet);
        sceKernelDelayThread(700000);
        standby305();
        scePowerRequestStandby();
        sceKernelDelayThread(1000000);
        }

    sceGuFinish();
    sceGuSync(0, 0);
    noflickerbase=flipScreen();
    sceDisplayWaitVblankStart();

	} //main loop

    {//end
    intraFontUnload(ltn[0]);
    intraFontUnload(ltn[4]);
    intraFontUnload(ltn[8]);
    	intraFontShutdown();
	sceKernelExitGame();

	return 0;
    }//
}


//intraFontPrint(ltn[0], 180, 70, "regular, ");
        //intraFontPrint(ltn[4], 330, 70, "bold, ");//
        //intraFontPrint(ltn[8],  10, 70, "Latin Sans-Serif: ");//
