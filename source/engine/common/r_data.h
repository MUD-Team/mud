// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 7a92e336fa5cd33a558521448f5830ed9bd919ae $
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
//  Refresh module, data I/O, caching, retrieval of graphics
//  by name.
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_defs.h"
#include "r_state.h"

// Retrieve column data for span blitting.
tallpost_t *R_GetPatchColumn(patch_t *patch, int32_t colnum);
tallpost_t *R_GetTextureColumn(texhandle_t texnum, int32_t colnum);

// I/O, setting up the stuff.
void R_InitData(void);
void R_PrecacheLevel(void);

uint32_t SlopeDiv(uint32_t num, uint32_t den);
