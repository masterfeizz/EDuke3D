/*
 Copyright (C) 2015 Felipe Izzo
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 
 See the GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
 */


 /* Everything in here must be redone, this is just a duct tape solution and should be destroyed as soon as possible*/

#include <3ds.h>
#include "inttypes.h"

#ifndef UNREFERENCED_PARAMETER
# define UNREFERENCED_PARAMETER(x) x=x
#endif

static int32_t Mixrate;
static int32_t Initialized = 0;
static int32_t Playing = 0;
static int32_t nSamples = 0;
static int32_t delay = 550000;

void ( *MixCallBack )( void ) = 0;

uint64_t lastMix =0;

void processAudio(void){
    if(MixCallBack != 0  && (svcGetSystemTick() - lastMix > delay )){
        lastMix = svcGetSystemTick();
        MixCallBack();
    }
}

int32_t CTRDrv_GetError(void)
{
    return 0;
}

const char *CTRDrv_ErrorString( int32_t ErrorNumber )
{
    UNREFERENCED_PARAMETER(ErrorNumber);
    return "No sound, Ok.";
}

int32_t CTRDrv_PCM_Init(int32_t *mixrate, int32_t *numchannels, void * initdata)
{
    if(csndInit() != 0)
        return;

    Mixrate = *mixrate;
    Initialized = 1;
    int isN3DS;
    APT_CheckNew3DS(&isN3DS);

    if(isN3DS)
        delay = 2200000;

    UNREFERENCED_PARAMETER(numchannels);
    UNREFERENCED_PARAMETER(initdata);
    return 0;
}

void CTRDrv_PCM_Shutdown(void)
{
    if(!Initialized)
        return;

    csndExit();
    Initialized = 0;
}

int32_t CTRDrv_PCM_BeginPlayback(char *BufferStart, int32_t BufferSize,
						int32_t NumDivisions, void ( *CallBackFunc )( void ) )
{
    if(!Initialized)
        return;


    MixCallBack = CallBackFunc;
    MixCallBack();

    nSamples = BufferSize/2;

    csndPlaySound(0x10, SOUND_REPEAT | SOUND_FORMAT_16BIT, Mixrate, 0.75f, 0.0f, (u32*)BufferStart, (u32*)BufferStart, BufferSize*NumDivisions);
    UNREFERENCED_PARAMETER(NumDivisions);
    
    return 0;
}

void CTRDrv_PCM_StopPlayback(void)
{
    if(!Initialized)
        return;

    CSND_SetPlayState(0x10, 0);
    csndExecCmds(false);
}

void CTRDrv_PCM_Lock(void)
{
}

void CTRDrv_PCM_Unlock(void)
{
}
