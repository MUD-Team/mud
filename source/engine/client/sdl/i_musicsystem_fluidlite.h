// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
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
//  Plays music utilizing the FluidLite music library.
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_musicsystem.h"

#include <fluidlite.h>

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

	virtual void startSong(byte *data, size_t length, bool loop);
	virtual void stopSong();
	virtual void pauseSong();
	virtual void resumeSong();
	virtual void playChunk() { }

	virtual void setVolume(float volume);

	virtual bool isInitialized() const { return m_isInitialized; }

	MidiSequencer *m_sequencer;

  private: 
	bool m_isInitialized;

	fluid_synth_t *m_synth;
	fluid_settings_t *m_synthSettings;
	fluid_sfloader_t *m_soundfontLoader;
	MidiRealTimeInterface *m_interface;
};