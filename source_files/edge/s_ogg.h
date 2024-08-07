//----------------------------------------------------------------------------
//  EDGE OGG Music Player (HEADER)
//----------------------------------------------------------------------------
//
//  Copyright (c) 2004-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
// -ACB- 2004/08/18 Written
//

#pragma once

#include "epi_filesystem.h"
#include "s_music.h"
#include "snd_data.h"

AbstractMusicPlayer *PlayOGGMusic(epi::File *song, bool looping);

bool LoadOGGSound(SoundData *buf, const uint8_t *data, int length);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
