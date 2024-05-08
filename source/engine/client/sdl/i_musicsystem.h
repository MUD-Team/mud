// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 51cbdb2b0234e59659315d6f294f814e596e7f45 $
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
//	Music player classes for the supported music libraries
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomtype.h"

/**
 * @brief Abstract base class that provides an interface for inheriting
 *        classes as well as default implementations for several functions.
 */
class MusicSystem
{
  public:
    MusicSystem() : m_isPlaying(false), m_isPaused(false), m_tempo(120.0f), m_volume(1.0f)
    {
    }
    virtual ~MusicSystem()
    {
    }

    virtual void startSong(uint8_t *data, size_t length, bool loop);
    virtual void stopSong();
    virtual void pauseSong();
    virtual void resumeSong();
    virtual void playChunk() = 0;

    virtual void setVolume(float volume);
    float        getVolume() const
    {
        return m_volume;
    }
    virtual void setTempo(float tempo);
    float        getTempo() const
    {
        return m_tempo;
    }

    virtual bool isInitialized() const = 0;
    bool         isPlaying() const
    {
        return m_isPlaying;
    }
    bool isPaused() const
    {
        return m_isPaused;
    }

  private:
    bool m_isPlaying;
    bool m_isPaused;

    float m_tempo;
    float m_volume;
};

/**
 * @brief This music system does not play any music.  It can be selected
 *        when the user wishes to disable music output.
 */
class SilentMusicSystem : public MusicSystem
{
  public:
    SilentMusicSystem()
    {
        Printf(PRINT_WARNING, "I_InitMusic: Music playback disabled.\n");
    }

    virtual void startSong(uint8_t *data, size_t length, bool loop)
    {
    }
    virtual void stopSong()
    {
    }
    virtual void pauseSong()
    {
    }
    virtual void resumeSong()
    {
    }
    virtual void playChunk()
    {
    }
    virtual void setVolume(float volume) const
    {
    }

    virtual bool isInitialized() const
    {
        return true;
    }
};