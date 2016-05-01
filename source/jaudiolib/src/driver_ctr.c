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


 /* Fixes needed */

#include <3ds.h>
#include "inttypes.h"

#ifndef UNREFERENCED_PARAMETER
# define UNREFERENCED_PARAMETER(x) x=x
#endif

#define TICKS_PER_SEC 268111856LL

static char *MixBuffer = NULL;
static int32_t MixBufferSize = 0;

static int32_t Initialized = 0;
static int32_t Playing = 0;
static int32_t nSamples = 0;

void ( *MixCallBack )( void ) = 0;

uint64_t lastMix =0;
CSND_ChnInfo chninfo;

ndspWaveBuf waveBuf[2];
uint8_t curBuffer;

void callbackFunc(void *data) {
    if(MixCallBack != 0 ){
        if(waveBuf[curBuffer].status == NDSP_WBUF_DONE){
            FillBuffer(waveBuf[curBuffer].data_pcm16, MixBufferSize);
            ndspChnWaveBufAdd(0, &waveBuf[curBuffer]);
            curBuffer = !curBuffer;
        }
    }
}

void FillBuffer(void *audioBuffer, size_t size) {
    if(MixCallBack == 0)
        return;

    MixCallBack();
    memcpy(audioBuffer, MixBuffer, size);
    DSP_FlushDataCache(audioBuffer, size);
};

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
    if(ndspInit() != 0)
        return;

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, NDSP_INTERP_NONE);
    ndspSetClippingMode(NDSP_CLIP_NORMAL);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetRate(0, 44100.0f);
    ndspSetCallback(&callbackFunc, 0);

    Initialized = 1;

    UNREFERENCED_PARAMETER(numchannels);
    UNREFERENCED_PARAMETER(initdata);
    return 0;
}

void CTRDrv_PCM_Shutdown(void)
{
    if(!Initialized)
        return;

    ndspExit();
    Initialized = 0;
}

int32_t CTRDrv_PCM_BeginPlayback(char *BufferStart, int32_t BufferSize,
						int32_t NumDivisions, void ( *CallBackFunc )( void ) )
{
    if(!Initialized)
        return;

    MixBuffer = BufferStart;
    MixBufferSize = BufferSize;

    MixCallBack = CallBackFunc;
    MixCallBack();

    nSamples = BufferSize/4;

    memset(waveBuf,0,sizeof(waveBuf));

    waveBuf[0].data_vaddr = linearAlloc(BufferSize);
    waveBuf[0].nsamples = nSamples;
    waveBuf[1].data_vaddr = linearAlloc(BufferSize);
    waveBuf[1].nsamples = nSamples;

    memset(waveBuf[0].data_pcm16, 0, BufferSize);
    memset(waveBuf[0].data_pcm16, 0, BufferSize);
    curBuffer = 0;
    ndspChnWaveBufAdd(0, &waveBuf[0]);
    ndspChnWaveBufAdd(0, &waveBuf[1]);

    UNREFERENCED_PARAMETER(NumDivisions);
    
    return 0;
}

void CTRDrv_PCM_StopPlayback(void)
{
    if(!Initialized)
        return;

    ndspChnWaveBufClear(0);
    linearFree(waveBuf[0].data_vaddr);
    linearFree(waveBuf[1].data_vaddr);
    MixCallBack = 0;
}

void CTRDrv_PCM_Lock(void)
{
}

void CTRDrv_PCM_Unlock(void)
{
}
