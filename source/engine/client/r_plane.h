// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 0199a26d4018b9af2e365ca4a4b844ce6e803bb3 $
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
//	Refresh, visplane stuff (floor, ceilings).
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_data.h"
#include "res_texture.h"

// Visplane related.
extern int32_t *lastopening;

typedef void (*planefunction_t)(int32_t top, int32_t bottom);

extern planefunction_t floorfunc;
extern planefunction_t ceilingfunc_t;

extern int32_t *floorclip;
extern int32_t *ceilingclip;
extern int32_t *floorclipinitial;
extern int32_t *ceilingclipinitial;

extern fixed_t *yslope;

void R_InitPlanes(void);
void R_ClearPlanes(void);

void R_MapPlane(int32_t y, int32_t x1, int32_t x2);

void R_MakeSpans(int32_t x, int32_t t1, int32_t b1, int32_t t2, int32_t b2);

void R_DrawPlanes(void);

visplane_t *R_FindPlane(plane_t secplane, texhandle_t picnum, int32_t lightlevel,
                        fixed_t xoffs, // killough 2/28/98: add x-y offsets
                        fixed_t yoffs, fixed_t xscale, fixed_t yscale, angle_t angle);

visplane_t *R_CheckPlane(visplane_t *pl, int32_t start, int32_t stop);

// [RH] Added for multires support
bool R_PlaneInitData(IRenderSurface *surface);
