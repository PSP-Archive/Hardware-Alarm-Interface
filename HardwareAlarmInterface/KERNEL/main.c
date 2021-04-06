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
 Credits to Moonlight and Silverspring for maintaining LibDocs
 */

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspdisplay_kernel.h>

PSP_MODULE_INFO("myLib", 0x1006, 1, 0);
PSP_MAIN_THREAD_ATTR(0);
//PSP_NO_CREATE_MAIN_THREAD();

int module_start(SceSize args, void *argp)
{
   return 0;
}

int writeAlarm305(int unk)
{
     u32 k1;

        k1 = pspSdkSetK1(0);
    int retcode=0;
    retcode=_sysconWriteAlarm(unk);

    pspSdkSetK1(k1);
    return retcode;

}

int standby305()
{
    u32 k1;

        k1 = pspSdkSetK1(0);
    int retcode=0;
    retcode=_sysconStandBy();
    pspSdkSetK1(k1);
    return retcode;

}

int readClock305(void)
{
     u32 k1;

        k1 = pspSdkSetK1(0);
    int retcode=0;
    _sysconReadClock(&retcode);

    pspSdkSetK1(k1);

    return retcode;
}

int module_stop()
{
   return 0;
}
