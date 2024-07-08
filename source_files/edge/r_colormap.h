//----------------------------------------------------------------------------
//  EDGE Colour Code
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#pragma once

#include "ddf_colormap.h"
#include "r_defs.h"

class AbstractShader;

enum PaletteTypes
{
    kPaletteNormal = 0,
    kPalettePain   = 1,
    kPaletteBonus  = 2,
    kPaletteSuit   = 3
};

// -AJA- 1999/07/10: Some stuff for colmap.ddf.

void GetColormapRGB(const Colormap *colmap, float *r, float *g, float *b);

RGBAColor GetFontColor(const Colormap *colmap);
RGBAColor ParseFontColor(const char *name, bool strict = false);

AbstractShader *GetColormapShader(const struct RegionProperties *props, int light_add = 0, Sector *sec = nullptr);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
