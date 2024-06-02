// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 83c0464c9bd105924df5cd7117a413982ee82af9 $
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
//	Refresh module, drawing LineSegs from BSP.
//
//-----------------------------------------------------------------------------

#pragma once

void R_PrepWall(fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2, fixed_t dist1, fixed_t dist2, int32_t start,
                int32_t stop);
void R_RenderMaskedSegRange(drawseg_t *ds, int32_t x1, int32_t x2);
void R_StoreWallRange(int32_t start, int32_t stop);
void R_RenderSegLoop();
void R_ClearOpenings();

EXTERN_CVAR(r_columnmethod)
