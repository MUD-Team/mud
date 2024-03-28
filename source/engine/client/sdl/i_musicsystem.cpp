// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 5b841515c04cc52828b06e466cf3f0fc9cce8fa0 $
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
//
// Music player classes for the supported music libraries
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <math.h>

#include "i_music.h"
#include "i_musicsystem.h"
#include "i_sdl.h"
#include "i_system.h"

extern MusicSystem *musicsystem;

// ============================================================================
//
// MusicSystem base class functions
//
// ============================================================================

void MusicSystem::startSong(byte *data, size_t length, bool loop)
{
    m_isPlaying = true;
    m_isPaused  = false;
}

void MusicSystem::stopSong()
{
    m_isPlaying = false;
    m_isPaused  = false;
}

void MusicSystem::pauseSong()
{
    m_isPaused = m_isPlaying;
}

void MusicSystem::resumeSong()
{
    m_isPaused = false;
}

void MusicSystem::setTempo(float tempo)
{
    if (tempo > 0.0f)
        m_tempo = tempo;
}

void MusicSystem::setVolume(float volume)
{
    m_volume = clamp(volume, 0.0f, 1.0f);
}

VERSION_CONTROL(i_musicsystem_cpp, "$Id: 5b841515c04cc52828b06e466cf3f0fc9cce8fa0 $")
