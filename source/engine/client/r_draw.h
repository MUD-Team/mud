// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 63a480e6366b77a5959206d01d4fd2038989befd $
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdint.h>

#include "doomtype.h"
#include "r_defs.h"
#include "r_intrin.h"

typedef struct
{
    uint8_t *source;
    uint8_t *destination;

    int32_t pitch_in_pixels;

    tallpost_t *post;

    shaderef_t colormap;

    int32_t x;
    int32_t yl;
    int32_t yh;

    fixed_t iscale;
    fixed_t texturemid;
    fixed_t texturefrac;
    fixed_t textureheight;

    fixed_t translevel;

    palindex_t color; // for r_drawflat
} drawcolumn_t;

extern "C" drawcolumn_t dcol;

typedef struct
{
    uint8_t *source;
    uint8_t *destination;

    int32_t pitch_in_pixels;

    shaderef_t colormap;

    int32_t y;
    int32_t x1;
    int32_t x2;

    uint32_t  texture_width_bits;
    uint32_t  texture_height_bits;
    dsfixed_t xfrac;
    dsfixed_t yfrac;
    dsfixed_t xstep;
    dsfixed_t ystep;

    float iu;
    float iv;
    float id;
    float iustep;
    float ivstep;
    float idstep;

    fixed_t translevel;

    shaderef_t slopelighting[MAXWIDTH];

    palindex_t color;
} drawspan_t;

extern "C" drawspan_t dspan;

// [RH] Temporary buffer for column drawing

void R_RenderColumnRange(int32_t start, int32_t stop, int32_t *top, int32_t *bottom, tallpost_t **posts,
                         void (*colblast)(), bool calc_light, int32_t columnmethod);

// [RH] Pointers to the different column and span drawers...

// The span blitting interface.
// Hook in assembler or system specific BLT here.
extern void (*R_DrawColumn)(void);

// The Spectre/Invisibility effect.
extern void (*R_DrawFuzzColumn)(void);

// [RH] Draw translucent column;
extern void (*R_DrawTranslucentColumn)(void);

// Span blitting for rows, floor/ceiling.
// No Sepctre effect needed.
extern void (*R_DrawSpan)(void);

extern void (*R_DrawSlopeSpan)(void);

extern void (*R_FillColumn)(void);
extern void (*R_FillSpan)(void);
extern void (*R_FillTranslucentSpan)(void);

// [RH] Initialize the above function pointers
void R_InitColumnDrawers();

void R_InitVectorizedDrawers();

void R_DrawColumnD(void);
void R_DrawFuzzColumnD(void);
void R_DrawTranslucentColumnD(void);
void R_DrawTranslatedColumnD(void);

void R_BlankColumn(void);
void R_BlankSpan(void);
void R_FillSpanD(void);

void R_DrawSpanD_c(void);
void R_DrawSlopeSpanD_c(void);

#define SPANJUMP   16
#define INTERPSTEP (0.0625f)

class IRenderSurface;

void r_dimpatchD_c(IRenderSurface *surface, argb_t color, int32_t alpha, int32_t x1, int32_t y1, int32_t w, int32_t h);

#ifdef __SSE2__
void R_DrawSpanD_SSE2(void);
void R_DrawSlopeSpanD_SSE2(void);
void r_dimpatchD_SSE2(IRenderSurface *, argb_t color, int32_t alpha, int32_t x1, int32_t y1, int32_t w, int32_t h);
#endif

#ifdef __MMX__
void R_DrawSpanD_MMX(void);
void R_DrawSlopeSpanD_MMX(void);
void r_dimpatchD_MMX(IRenderSurface *, argb_t color, int32_t alpha, int32_t x1, int32_t y1, int32_t w, int32_t h);
#endif

#ifdef __ALTIVEC__
void R_DrawSpanD_ALTIVEC(void);
void R_DrawSlopeSpanD_ALTIVEC(void);
void r_dimpatchD_ALTIVEC(IRenderSurface *, argb_t color, int32_t alpha, int32_t x1, int32_t y1, int32_t w, int32_t h);
#endif

// Vectorizable function pointers:
extern void (*R_DrawSpanD)(void);
extern void (*R_DrawSlopeSpanD)(void);
extern void (*r_dimpatchD)(IRenderSurface *surface, argb_t color, int32_t alpha, int32_t x1, int32_t y1, int32_t w,
                           int32_t h);