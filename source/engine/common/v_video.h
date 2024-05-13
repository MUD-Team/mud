// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ce6e07ffa86ede1bbef34693925224cca4dae328 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
// Copyright (C) 2024 by The MUD Team.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	Gamma correction LUT.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------

#pragma once

#include "m_vectors.h"
#include "r_data.h"
#include "v_palette.h"

class IRenderSurface;

extern int32_t CleanXfac, CleanYfac;

// Translucency tables
extern argb_t     Col2RGB8[65][256];
extern palindex_t RGB32k[32][32][32];

void            V_Init();
void STACK_ARGS V_Close();

void V_ForceVideoModeAdjustment();
void V_AdjustVideoMode();

// The color to fill with for #4 and #5 above
extern int32_t V_ColorFill;

// The color map for #1 and #2 above
extern translationref_t V_ColorMap;

void V_MarkRect(int32_t x, int32_t y, int32_t width, int32_t height);

// Returns the closest color to the one desired. String
// should be of the form "rr gg bb" or the name of a color
// as defined in the X11R6RGB lump.
argb_t V_GetColorFromString(const std::string &str);

template <> forceinline palindex_t rt_blend2(const palindex_t bg, const int32_t bga, const palindex_t fg, const int32_t fga)
{
    // Crazy 8bpp alpha-blending using lookup tables and bit twiddling magic
    argb_t bgARGB = Col2RGB8[bga >> 2][bg];
    argb_t fgARGB = Col2RGB8[fga >> 2][fg];

    argb_t mix = (fgARGB + bgARGB) | 0x1f07c1f;
    return RGB32k[0][0][mix & (mix >> 15)];
}

template <> forceinline argb_t rt_blend2(const argb_t bg, const int32_t bga, const argb_t fg, const int32_t fga)
{
    return alphablend2a(bg, bga, fg, fga);
}

bool V_UseWidescreen();

// Alpha blend between two RGB colors with only dest alpha value
// 0 <=   toa <= 255
forceinline argb_t alphablend1a(const argb_t from, const argb_t to, const int32_t toa)
{
    const int32_t fr = from.getr();
    const int32_t fg = from.getg();
    const int32_t fb = from.getb();

    const int32_t dr = to.getr() - fr;
    const int32_t dg = to.getg() - fg;
    const int32_t db = to.getb() - fb;

    return argb_t(fr + ((dr * toa) >> 8), fg + ((dg * toa) >> 8), fb + ((db * toa) >> 8));
}

// Alpha blend between two RGB colors with two alpha values
// 0 <= froma <= 255
// 0 <=   toa <= 255
forceinline argb_t alphablend2a(const argb_t from, const int32_t froma, const argb_t to, const int32_t toa)
{
    return argb_t((from.getr() * froma + to.getr() * toa) >> 8, (from.getg() * froma + to.getg() * toa) >> 8,
                  (from.getb() * froma + to.getb() * toa) >> 8);
}
