// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 95cf672ca6afd942a6a65e9f094e7026e45f8440 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	GI
//
//-----------------------------------------------------------------------------

#pragma once

#include "olumpname.h"
#include "s_sound.h"

#define GI_NOCRAZYDEATH 0x00000080

typedef struct
{
    uint8_t offset;
    uint8_t size;
    char    tl[8];
    char    t[8];
    char    tr[8];
    char    l[8];
    char    r[8];
    char    bl[8];
    char    b[8];
    char    br[8];
} gameborder_t;

typedef struct
{
    int32_t       flags;
    OLumpName     titleMusic;
    char          chatSound[MAX_SNDNAME + 1];
    char          quitSound[MAX_SNDNAME + 1];
    int32_t       maxSwitch;
    char          borderFlat[8];
    gameborder_t *border;

    char titleString[64];
} gameinfo_t;

extern gameinfo_t gameinfo;
