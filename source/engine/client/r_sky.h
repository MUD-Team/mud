// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 1105a07dcd0cc21a32a1b4fd61bdfb7cc55cd74d $
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
//	Sky rendering.
//
//-----------------------------------------------------------------------------

#pragma once

#include "c_cvars.h"
#include "m_fixed.h"
#include "r_defs.h"

// SKY, store the number for name.
extern char SKYFLATNAME[8];

extern int32_t sky1shift;       //		[ML] 5/11/06 - remove sky2 remenants

extern texhandle_t sky1texture; //		""
extern texhandle_t sky2texture; //		""
extern fixed_t     skypos;      //		""
extern fixed_t     skytexturemid;
extern int32_t     skystretch;
extern fixed_t     skyiscale;
extern fixed_t     skyscale;
extern fixed_t     skyheight;

EXTERN_CVAR(r_stretchsky)

// Called whenever the sky changes.
void R_InitSkyMap();

void R_RenderSkyRange(visplane_t *pl);
