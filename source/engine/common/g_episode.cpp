// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
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
// DESCRIPTION:
//   Episode data for defining new episodes.
//
//-----------------------------------------------------------------------------

#include "g_episode.h"

#include "mud_includes.h"

OLumpName   EpisodeMaps[MAX_EPISODES];
EpisodeInfo EpisodeInfos[MAX_EPISODES];
uint8_t        episodenum        = 0;
bool        episodes_modified = false;
