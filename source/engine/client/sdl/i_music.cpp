// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: a972ca6fc2ddbc9792920cf2f8aae539843af6d6 $
//
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	SDL music handler
//
//-----------------------------------------------------------------------------

#include "i_music.h"

#include "i_musicsystem.h"
#include "i_musicsystem_fluidlite.h"
#include "i_musicsystem_sdl.h"
#include "i_system.h"
#include "m_argv.h"
#include "odamex.h"

MusicSystem    *musicsystem              = NULL;
MusicSystemType current_musicsystem_type = MS_NONE;

void S_StopMusic();
void S_ChangeMusic(std::string musicname, int32_t looping);

EXTERN_CVAR(snd_musicvolume)
EXTERN_CVAR(snd_musicsystem)

std::string currentmusic;

//
// S_MusicIsMus()
//
// Determines if a music lump is in the MUS format based on its header.
//
bool S_MusicIsMus(uint8_t *data, size_t length)
{
    if (length > 4 && data[0] == 'M' && data[1] == 'U' && data[2] == 'S' && data[3] == 0x1A)
        return true;

    return false;
}

//
// S_MusicIsMidi()
//
// Determines if a music lump is in the MIDI format based on its header.
//
bool S_MusicIsMidi(uint8_t *data, size_t length)
{
    if (length > 4 && data[0] == 'M' && data[1] == 'T' && data[2] == 'h' && data[3] == 'd')
        return true;

    return false;
}

//
// I_UpdateMusic()
//
// Play the next chunk of music for the current gametic
//
void I_UpdateMusic()
{
    if (musicsystem)
        musicsystem->playChunk();
}

// [Russell] - A better name, since we support multiple formats now
void I_SetMusicVolume(float volume)
{
    if (musicsystem)
        musicsystem->setVolume(volume);
}

void I_InitMusic(MusicSystemType musicsystem_type)
{
    I_ShutdownMusic();

    if (I_IsHeadless() || Args.CheckParm("-nosound") || Args.CheckParm("-nomusic") || snd_musicsystem == MS_NONE)
    {
        // User has chosen to disable music
        musicsystem              = new SilentMusicSystem();
        current_musicsystem_type = MS_NONE;
        return;
    }

    switch ((int32_t)musicsystem_type)
    {
    case MS_FLUIDLITE:
        musicsystem = new FluidLiteMusicSystem();
        break;

    case MS_SDLMIXER: // fall through
    default:
        musicsystem = new SdlMixerMusicSystem();
        break;
    }

    current_musicsystem_type = musicsystem_type;
}

void STACK_ARGS I_ShutdownMusic(void)
{
    if (musicsystem)
    {
        delete musicsystem;
        musicsystem = NULL;
    }
}

CVAR_FUNC_IMPL(snd_musicsystem)
{
    if ((int32_t)current_musicsystem_type == snd_musicsystem.asInt())
        return;

    if (musicsystem)
    {
        I_ShutdownMusic();
        S_StopMusic();
    }
    I_InitMusic();

    if (level.music.empty())
        S_ChangeMusic(currentmusic.c_str(), true);
    else
        S_ChangeMusic(std::string(level.music.c_str(), 8), true);
}

//
// I_SelectMusicSystem
//
// Takes the data and length of a song and determines which music system
// should be used to play the song, based on user preference and the song
// type.
//
static MusicSystemType I_SelectMusicSystem(uint8_t *data, size_t length)
{
    // Always honor the no-music preference
    if (snd_musicsystem == MS_NONE)
        return MS_NONE;

    bool ismidi = (S_MusicIsMus(data, length) || S_MusicIsMidi(data, length));

    if (ismidi)
        return MS_FLUIDLITE;

    // Non-midi music always uses SDL_Mixer (for now at least)
    return MS_SDLMIXER;
}

void I_PlaySong(uint8_t *data, size_t length, bool loop)
{
    if (!musicsystem)
        return;

    MusicSystemType newtype = I_SelectMusicSystem(data, length);
    if (newtype != current_musicsystem_type)
    {
        if (musicsystem)
        {
            I_ShutdownMusic();
            S_StopMusic();
        }
        I_InitMusic(newtype);
    }

    musicsystem->startSong(data, length, loop);

    I_SetMusicVolume(snd_musicvolume);
}

void I_PauseSong()
{
    if (musicsystem)
        musicsystem->pauseSong();
}

void I_ResumeSong()
{
    if (musicsystem)
        musicsystem->resumeSong();
}

void I_StopSong()
{
    if (musicsystem)
        musicsystem->stopSong();
}

bool I_QrySongPlaying(int32_t handle)
{
    if (musicsystem)
        return musicsystem->isPlaying();

    return false;
}

VERSION_CONTROL(i_music_cpp, "$Id: a972ca6fc2ddbc9792920cf2f8aae539843af6d6 $")
