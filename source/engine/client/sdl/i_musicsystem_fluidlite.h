// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
//
// Copyright (C) 2024 by The MUD Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  Plays music utilizing the FluidLite music library.
//
//-----------------------------------------------------------------------------

#pragma once

#include <fluidlite.h>

#include "i_musicsystem.h"

class MidiRealTimeInterface;
class MidiSequencer;

/**
 * @brief Plays music utilizing the FluidLite music library.
 */
class FluidLiteMusicSystem : public MusicSystem
{
  public:
    FluidLiteMusicSystem();
    virtual ~FluidLiteMusicSystem();

    virtual void startSong(uint8_t *data, size_t length, bool loop);
    virtual void stopSong();
    virtual void pauseSong();
    virtual void resumeSong();
    virtual void playChunk()
    {
    }

    virtual void setVolume(float volume);

    virtual bool isInitialized() const
    {
        return m_isInitialized;
    }
    virtual bool isLooping() const
    {
        return m_loopSong;
    }

    MidiSequencer *m_sequencer;

  private:
    bool m_isInitialized;
    bool m_loopSong;

    fluid_synth_t         *m_synth;
    fluid_settings_t      *m_synthSettings;
    fluid_sfloader_t      *m_soundfontLoader;
    MidiRealTimeInterface *m_interface;
};