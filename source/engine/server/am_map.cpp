// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 239170ceb814bf1330a6394d9f463474baeb69d9 $
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
//  AutoMap module.
//
//-----------------------------------------------------------------------------

#include "am_map.h"

#include "mud_includes.h"

am_default_colors_t AutomapDefaultColors;
am_colors_t         AutomapDefaultCurrentColors;
int32_t                 am_cheating = 0;

bool automapactive = false;

bool AM_ClassicAutomapVisible()
{
    return automapactive && !viewactive;
}

bool AM_OverlayAutomapVisible()
{
    return automapactive && viewactive;
}

void AM_SetBaseColorDoom()
{
}

void AM_SetBaseColorRaven()
{
}

void AM_SetBaseColorStrife()
{
}

void AM_Start()
{
}

bool AM_Responder(event_t *ev)
{
    return false;
}

void AM_Drawer()
{
}

VERSION_CONTROL(am_map_cpp, "$Id: 239170ceb814bf1330a6394d9f463474baeb69d9 $")
