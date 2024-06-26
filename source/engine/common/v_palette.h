// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 5f37939998a34e8c576f4e0d6868ea43a3271b32 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	V_PALETTE
//
//-----------------------------------------------------------------------------

#pragma once

#include "map_defs.h"

struct palette_t
{
    argb_t basecolors[256]; // non-gamma corrected colors
    argb_t colors[256];     // gamma corrected colors

    shademap_t maps;

    const palette_t &operator=(const palette_t &other)
    {
        for (size_t i = 0; i < 256; i++)
        {
            colors[i]     = other.colors[i];
            basecolors[i] = other.basecolors[i];
        }
        maps = other.maps;
        return *this;
    }
};

struct dyncolormap_s
{
    shaderef_t            maps;
    argb_t                color;
    argb_t                fade;
    struct dyncolormap_s *next;
};
typedef struct dyncolormap_s dyncolormap_t;

extern fargb_t baseblend;

extern uint8_t gammatable[256];
float          V_GetMinimumGammaLevel();
float          V_GetMaximumGammaLevel();
void           V_IncrementGammaLevel();

static inline argb_t V_GammaCorrect(const argb_t value)
{
    extern uint8_t gammatable[256];
    return argb_t(value.geta(), gammatable[value.getr()], gammatable[value.getg()], gammatable[value.getb()]);
}

palindex_t V_BestColor(const argb_t *palette_colors, int32_t r, int32_t g, int32_t b);
palindex_t V_BestColor(const argb_t *palette_colors, argb_t color);

// Alpha blend between two RGB colors with only dest alpha value
// 0 <=   toa <= 256
argb_t alphablend1a(const argb_t from, const argb_t to, const int32_t toa);
// Alpha blend between two RGB colors with two alpha values
// 0 <= froma <= 256
// 0 <=   toa <= 256
argb_t alphablend2a(const argb_t from, const int32_t froma, const argb_t to, const int32_t toa);

void V_InitPalette();

const palette_t *V_GetDefaultPalette();
const palette_t *V_GetGamePalette();

// V_RefreshColormaps()
//
// Generates all colormaps or shadings for the default palette
// with the current blending levels.
void V_RefreshColormaps();

// Sets up the default colormaps and shademaps based on the given palette:
void BuildDefaultColorAndShademap(const palette_t *pal, shademap_t &maps);
// Sets up the default shademaps (no colormaps) based on the given palette:
void BuildDefaultShademap(const palette_t *pal, shademap_t &maps);

// Colorspace conversion RGB <-> HSV
fahsv_t V_RGBtoHSV(const fargb_t &color);
fargb_t V_HSVtoRGB(const fahsv_t &color);

dyncolormap_t *GetSpecialLights(int32_t lr, int32_t lg, int32_t lb, int32_t fr, int32_t fg, int32_t fb);
