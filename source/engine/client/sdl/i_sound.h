// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: b9f66be2a84938e8e94714c0cd3c104447c8764c $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//
// DESCRIPTION:
//	System interface, sound.
//	[Odamex] Fitted to work with SDL
//
//-----------------------------------------------------------------------------

#pragma once

#include "s_sound.h"

// Init at program start...
void I_InitSound();

// ... shut down and relase at program termination.
void STACK_ARGS I_ShutdownSound(void);

void I_SetSfxVolume(float volume);

//
//	SFX I/O
//

// Initialize channels
void I_SetChannels(int32_t);

// load a sound from disk
void I_LoadSound(struct sfxinfo_struct *sfx);

// Starts a sound in a particular sound channel.
int32_t I_StartSound(int32_t id, float vol, int32_t sep, int32_t pitch, bool loop);

// Stops a sound channel.
void I_StopSound(int32_t handle);

// Called by S_*() functions
//	to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int32_t I_SoundIsPlaying(int32_t handle);

// Updates the volume, separation,
//	and pitch of a sound channel.
void I_UpdateSoundParams(int32_t handle, float vol, int32_t sep, int32_t pitch);
