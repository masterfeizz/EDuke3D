//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

/*
 * A reimplementation of Jim Dose's FX_MAN routines, using  SDL_mixer 1.2.
 *   Whee. FX_MAN is also known as the "Apogee Sound System", or "ASS" for
 *   short. How strangely appropriate that seems.
 */

#include "compat.h"

#include <stdio.h>
#include <errno.h>

#include "duke3d.h"
#include "cache1d.h"

#include "ctrlayer.h"
#include "music.h"

#if !defined _WIN32 && !defined(GEKKO)
//# define FORK_EXEC_MIDI 1
#endif

#if defined FORK_EXEC_MIDI  // fork/exec based external midi player
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
static char **external_midi_argv;
static pid_t external_midi_pid=-1;
static int8_t external_midi_restart=0;
#endif


static char *external_midi_tempfn = "eduke32-music.mid";


static int32_t external_midi = 0;

int32_t MUSIC_ErrorCode = MUSIC_Ok;

static char warningMessage[80];
static char errorMessage[80];

static int32_t music_initialized = 0;
static int32_t music_context = 0;
static int32_t music_loopflag = MUSIC_PlayOnce;

static void setErrorMessage(const char *msg)
{
    Bstrncpyz(errorMessage, msg, sizeof(errorMessage));
}

// The music functions...

const char *MUSIC_ErrorString(int32_t ErrorNumber)
{
    switch (ErrorNumber)
    {
    case MUSIC_Warning:
        return(warningMessage);

    case MUSIC_Error:
        return(errorMessage);

    case MUSIC_Ok:
        return("OK; no error.");

    case MUSIC_ASSVersion:
        return("Incorrect sound library version.");

    case MUSIC_SoundCardError:
        return("General sound card error.");

    case MUSIC_InvalidCard:
        return("Invalid sound card.");

    case MUSIC_MidiError:
        return("MIDI error.");

    case MUSIC_MPU401Error:
        return("MPU401 error.");

    case MUSIC_TaskManError:
        return("Task Manager error.");

        //case MUSIC_FMNotDetected:
        //    return("FM not detected error.");

    case MUSIC_DPMI_Error:
        return("DPMI error.");

    default:
        return("Unknown error.");
    } // switch

    return(NULL);
} // MUSIC_ErrorString

int32_t MUSIC_Init(int32_t SoundCard, int32_t Address)
{
    music_initialized = 1;
	return(MUSIC_Ok);
} // MUSIC_Init


int32_t MUSIC_Shutdown(void)
{
    MUSIC_StopSong();
    music_context = 0;
    music_initialized = 0;
    music_loopflag = MUSIC_PlayOnce;

    return(MUSIC_Ok);
} // MUSIC_Shutdown


void MUSIC_SetMaxFMMidiChannel(int32_t channel)
{
    UNREFERENCED_PARAMETER(channel);
} // MUSIC_SetMaxFMMidiChannel


void MUSIC_SetVolume(int32_t volume)
{
    volume = max(0, volume);
    volume = min(volume, 255);

    //Mix_VolumeMusic(volume >> 1);  // convert 0-255 to 0-128.
} // MUSIC_SetVolume


void MUSIC_SetMidiChannelVolume(int32_t channel, int32_t volume)
{
    UNREFERENCED_PARAMETER(channel);
    UNREFERENCED_PARAMETER(volume);
} // MUSIC_SetMidiChannelVolume


void MUSIC_ResetMidiChannelVolumes(void)
{
} // MUSIC_ResetMidiChannelVolumes


int32_t MUSIC_GetVolume(void)
{
    return 255;  // convert 0-128 to 0-255.
} // MUSIC_GetVolume


void MUSIC_SetLoopFlag(int32_t loopflag)
{
    music_loopflag = loopflag;
} // MUSIC_SetLoopFlag


int32_t MUSIC_SongPlaying(void)
{
    return(FALSE);
} // MUSIC_SongPlaying


void MUSIC_Continue(void)
{

} // MUSIC_Continue


void MUSIC_Pause(void)
{

} // MUSIC_Pause

int32_t MUSIC_StopSong(void)
{
    return(MUSIC_Ok);
} // MUSIC_StopSong


// Duke3D-specific.  --ryan.
// void MUSIC_PlayMusic(char *_filename)
int32_t MUSIC_PlaySong(char *song, int32_t loopflag)
{
    return MUSIC_Ok;
}


void MUSIC_SetContext(int32_t context)
{
    music_context = context;
} // MUSIC_SetContext


int32_t MUSIC_GetContext(void)
{
    return(music_context);
} // MUSIC_GetContext


void MUSIC_SetSongTick(uint32_t PositionInTicks)
{
    UNREFERENCED_PARAMETER(PositionInTicks);
} // MUSIC_SetSongTick


void MUSIC_SetSongTime(uint32_t milliseconds)
{
    UNREFERENCED_PARAMETER(milliseconds);
}// MUSIC_SetSongTime


void MUSIC_SetSongPosition(int32_t measure, int32_t beat, int32_t tick)
{
    UNREFERENCED_PARAMETER(measure);
    UNREFERENCED_PARAMETER(beat);
    UNREFERENCED_PARAMETER(tick);
} // MUSIC_SetSongPosition


void MUSIC_GetSongPosition(songposition *pos)
{
    UNREFERENCED_PARAMETER(pos);
} // MUSIC_GetSongPosition


void MUSIC_GetSongLength(songposition *pos)
{
    UNREFERENCED_PARAMETER(pos);
} // MUSIC_GetSongLength


int32_t MUSIC_FadeVolume(int32_t tovolume, int32_t milliseconds)
{
    UNREFERENCED_PARAMETER(tovolume);
    UNREFERENCED_PARAMETER(milliseconds);
    return(MUSIC_Ok);
} // MUSIC_FadeVolume


int32_t MUSIC_FadeActive(void)
{
    return(FALSE);
} // MUSIC_FadeActive


void MUSIC_StopFade(void)
{
} // MUSIC_StopFade


void MUSIC_RerouteMidiChannel(int32_t channel, int32_t (*function)(int32_t, int32_t, int32_t))
{
    UNREFERENCED_PARAMETER(channel);
    UNREFERENCED_PARAMETER(function);
} // MUSIC_RerouteMidiChannel


void MUSIC_RegisterTimbreBank(char *timbres)
{
    UNREFERENCED_PARAMETER(timbres);
} // MUSIC_RegisterTimbreBank


void MUSIC_Update(void)
{}
