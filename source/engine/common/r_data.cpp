// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 78554932593a4dead8644d10dd1689c12c64f358 $
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
// Revision 1.3  1997/01/29 20:10
// DESCRIPTION:
//		Preparation of data for rendering,
//		generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------

#include "r_data.h"

#include <ctype.h>

#include <algorithm>

#include "cmdlib.h"
#include "i_system.h"
#include "m_fileio.h"
#include "mud_includes.h"
#include "r_local.h"
#include "v_palette.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
//
// R_InitData
// Locates all the lumps
//	that will be used by all views
// Must be called after W_Init.
//
void R_InitData()
{
    // haleyjd 01/28/10: also initialize tantoangle_acc table
    Table_InitTanToAngle();
}

// Utility function,
//	called by R_PointToAngle.
uint32_t SlopeDiv(uint32_t num, uint32_t den)
{
    uint32_t ans;

    if (den < 512)
        return SLOPERANGE;

    ans = (num << 3) / (den >> 8);

    return ans <= SLOPERANGE ? ans : SLOPERANGE;
}

VERSION_CONTROL(r_data_cpp, "$Id: 78554932593a4dead8644d10dd1689c12c64f358 $")
