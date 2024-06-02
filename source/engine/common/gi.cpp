// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: bb8881dbc0490c572c584cc210225fa323737593 $
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

#include "gi.h"

#include "mud_includes.h"

gameinfo_t gameinfo;

static gameborder_t DoomBorder = {8,        8,        "brdr_tl", "brdr_t", "brdr_tr",
                                  "brdr_l", "brdr_r", "brdr_bl", "brdr_b", "brdr_br"};

gameinfo_t CommercialGameInfo = {0, "D_DM2TTL", "misc/chat", "menu/quit2",
                                 3, "GRNROCK",  &DoomBorder, "DOOM 2: Hell on Earth"};

VERSION_CONTROL(gi_cpp, "$Id: bb8881dbc0490c572c584cc210225fa323737593 $")
