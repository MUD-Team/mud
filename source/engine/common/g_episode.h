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

#pragma once

#define MAX_EPISODES 8

#include "doomtype.h"
#include "olumpname.h"

struct EpisodeInfo
{
    std::string name;
    char        key;
    bool        fulltext;
    bool        noskillmenu;

    EpisodeInfo() : name(""), key('\0'), fulltext(false), noskillmenu(false)
    {
    }
};

extern OLumpName   EpisodeMaps[MAX_EPISODES];
extern EpisodeInfo EpisodeInfos[MAX_EPISODES];
extern uint8_t     episodenum;
extern bool        episodes_modified; // Used by UMAPINFO only
