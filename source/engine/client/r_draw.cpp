// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: f878e66e7201c97021a528e052d53ef9a0811b5e $
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
//		The actual span/column drawing functions.
//		Here find the main potential for optimization,
//		 e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------

#include "r_draw.h"

#include <assert.h>
#include <math.h>

#include <algorithm>

#include "gi.h"
#include "i_sdl.h"
#include "i_video.h"
#include "mud_includes.h"
#include "mud_profiling.h"
#include "r_intrin.h"
#include "r_main.h"
#include "v_textcolors.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#undef RANGECHECK

// status bar height at bottom of screen
// [RH] status bar position at bottom of screen
extern int32_t ST_Y;

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//	not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//	and we need only the base address,
//	and the total size == width*height*depth/8.,
//

extern "C"
{
    drawcolumn_t dcol;
    drawspan_t   dspan;
}

uint8_t *viewimage;

int32_t viewwidth;
int32_t viewheight;

int32_t scaledviewwidth;
int32_t viewwindowx;
int32_t viewwindowy;

// [RH] Pointers to the different column drawers.
//		These get changed depending on the current
//		screen depth.
void (*R_DrawColumn)(void);
void (*R_DrawFuzzColumn)(void);
void (*R_DrawTranslucentColumn)(void);
void (*R_DrawSpan)(void);
void (*R_DrawSlopeSpan)(void);
void (*R_FillColumn)(void);
void (*R_FillSpan)(void);
void (*R_FillTranslucentSpan)(void);

// Possibly vectorized functions:
void (*R_DrawSpanD)(void);
void (*R_DrawSlopeSpanD)(void);
void (*r_dimpatchD)(IRenderSurface *surface, argb_t color, int32_t alpha, int32_t x1, int32_t y1, int32_t w, int32_t h);

// ============================================================================
//
// Fuzz Table
//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels from adjacent ones to left and right.
// Used with an all black colormap, this could create the SHADOW effect,
// i.e. spectres and invisible players.
//
// ============================================================================

class FuzzTable
{
  public:
    FuzzTable() : pos(0)
    {
    }

    forceinline void incrementRow()
    {
        pos = (pos + 1) % FuzzTable::size;
    }

    forceinline void incrementColumn()
    {
        pos = (pos + 3) % FuzzTable::size;
    }

    forceinline int32_t getValue() const
    {
        // [SL] quickly convert the table value (-1 or 1) into (-pitch or pitch).
        // [AM] Replaced with a multiply that returns accurate results.  Hopefully
        //      we can find a way to improve upon an imul someday.
        int32_t pitch = R_GetRenderingSurface()->getPitchInPixels();
        int32_t value = table[pos];
        return pitch * value;
    }

  private:
    static const size_t  size = 64;
    static const int32_t table[FuzzTable::size];
    int32_t              pos;
};

const int32_t FuzzTable::table[FuzzTable::size] = {
    1, -1, 1,  -1, 1, 1,  -1, 1,  1,  -1, 1, 1, 1, -1, 1, 1, 1,  -1, -1, -1, -1, 1, -1, -1, 1,  1, 1, 1,  -1, 1, -1, 1,
    1, -1, -1, 1,  1, -1, -1, -1, -1, 1,  1, 1, 1, -1, 1, 1, -1, 1,  1,  1,  -1, 1, 1,  1,  -1, 1, 1, -1, 1,  1, -1, 1};

static FuzzTable fuzztable;

// ============================================================================
//
// Translucency Table
//
// ============================================================================

/*
[RH] This translucency algorithm is based on DOSDoom 0.65's, but uses
a 32k RGB table instead of an 8k one. At least on my machine, it's
slightly faster (probably because it uses only one shift instead of
two), and it looks considerably less green at the ends of the
translucency range. The extra size doesn't appear to be an issue.

The following note is from DOSDoom 0.65:

New translucency algorithm, by Erik Sandberg:

Basically, we compute the red, green and blue values for each pixel, and
then use a RGB table to check which one of the palette colours that best
represents those RGB values. The RGB table is 8k big, with 4 R-bits,
5 G-bits and 4 B-bits. A 4k table gives a bit too bad precision, and a 32k
table takes up more memory and results in more cache misses, so an 8k
table seemed to be quite ultimate.

The computation of the RGB for each pixel is accelerated by using two
1k tables for each translucency level.
The xth element of one of these tables contains the r, g and b values for
the colour x, weighted for the current translucency level (for example,
the weighted rgb values for background colour at 75% translucency are 1/4
of the original rgb values). The rgb values are stored as three
low-precision fixed point values, packed into one long per colour:
Bit 0-4:   Frac part of blue  (5 bits)
Bit 5-8:   Int  part of blue  (4 bits)
Bit 9-13:  Frac part of red   (5 bits)
Bit 14-17: Int  part of red   (4 bits)
Bit 18-22: Frac part of green (5 bits)
Bit 23-27: Int  part of green (5 bits)
Bit 28-31: All zeros          (4 bits)

The point of this format is that the two colours now can be added, and
then be converted to a RGB table index very easily: First, we just set
all the frac bits and the four upper zero bits to 1. It's now possible
to get the RGB table index by anding the current value >> 5 with the
current value >> 19. When asm-optimised, this should be the fastest
algorithm that uses RGB tables.
*/

/**
 * @brief Apply a soft light filter using Pegtop's formula.
 *
 * @see https://en.wikipedia.org/wiki/Blend_modes#Soft_Light
 *
 * @param bot Bottom channel value.
 * @param top Top channel value.
 * @return Filtered value.
 */
static uint8_t SoftLight(const uint8_t bot, const uint8_t top)
{
    const float a   = bot / 255.f;
    const float b   = top / 255.f;
    const float res = (1.f - 2.f * b) * pow(a, 2.f) + (2.f * b * a);
    return res * 255;
}

// ============================================================================
//
// Spans
//
// With DOOM style restrictions on view orientation,
// the floors and ceilings consist of horizontal slices
// or spans with constant z depth.
// However, rotation around the world z axis is possible,
// thus this mapping, while simpler and faster than
// perspective correct texture mapping, has to traverse
// the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
// and the inner loop has to step in texture space u and v.
//
// ============================================================================

// ============================================================================
//
// Generic Drawers
//
// Templated versions of column and span drawing functions
//
// ============================================================================

//
// R_BlankColumn
//
// [SL] - Does nothing (obviously). Used when a column drawing function
// pointer should not draw anything.
//
void R_BlankColumn()
{
}

//
// R_BlankSpan
//
// [SL] - Does nothing (obviously). Used when a span drawing function
// pointer should not draw anything.
//
void R_BlankSpan()
{
}

//
// R_FillColumnGeneric
//
// Templated version of a function to fill a column with a solid color.
// The data type of the destination pixels and a color-remapping functor
// are passed as template parameters.
//
template <typename PIXEL_T, typename COLORFUNC>
static forceinline void R_FillColumnGeneric(PIXEL_T *dest, const drawcolumn_t &drawcolumn)
{
#ifdef RANGECHECK
    if (drawcolumn.x < 0 || drawcolumn.x >= viewwidth || drawcolumn.yl < 0 || drawcolumn.yh >= viewheight)
    {
        Printf(PRINT_HIGH, "R_FillColumn: %i to %i at %i\n", drawcolumn.yl, drawcolumn.yh, drawcolumn.x);
        return;
    }
#endif

    int32_t color = drawcolumn.color;
    int32_t pitch = drawcolumn.pitch_in_pixels;
    int32_t count = drawcolumn.yh - drawcolumn.yl + 1;
    if (count <= 0)
        return;

    COLORFUNC colorfunc(drawcolumn);

    do
    {
        colorfunc(color, dest);
        dest += pitch;
    } while (--count);
}

//
// R_DrawColumnGeneric
//
// A column is a vertical slice/span from a wall texture that,
// given the DOOM style restrictions on the view orientation,
// will always have constant z depth.
// Thus a special case loop for very fast rendering can
// be used. It has also been used with Wolfenstein 3D.
//
// Templated version of a column mapping function.
// The data type of the destination pixels and a color-remapping functor
// are passed as template parameters.
//
template <typename PIXEL_T, typename COLORFUNC>
static forceinline void R_DrawColumnGeneric(PIXEL_T *dest, const drawcolumn_t &drawcolumn)
{
#ifdef RANGECHECK
    if (drawcolumn.x < 0 || drawcolumn.x >= viewwidth || drawcolumn.yl < 0 || drawcolumn.yh >= viewheight)
    {
        Printf(PRINT_HIGH, "R_DrawColumn: %i to %i at %i\n", drawcolumn.yl, drawcolumn.yh, drawcolumn.x);
        return;
    }
#endif

    palindex_t *source = drawcolumn.source;
    int32_t     pitch  = drawcolumn.pitch_in_pixels;
    int32_t     count  = drawcolumn.yh - drawcolumn.yl + 1;
    if (count <= 0)
        return;

    const fixed_t fracstep = drawcolumn.iscale;
    fixed_t       frac     = drawcolumn.texturefrac;

    const int32_t texheight = drawcolumn.textureheight;
    const int32_t mask      = (texheight >> FRACBITS) - 1;

    COLORFUNC colorfunc(drawcolumn);

    // [SL] Properly tile textures whose heights are not a power-of-2,
    // avoiding a tutti-frutti effect.  From Eternity Engine.
    if (texheight & (texheight - 1))
    {
        // texture height is NOT a power-of-2
        // just do a simple blit to the dest buffer (I'm lazy)

        if (frac < 0)
            while ((frac += texheight) < 0)
                ;
        else
            while (frac >= texheight)
                frac -= texheight;

        while (count--)
        {
            colorfunc(source[frac >> FRACBITS], dest);
            dest += pitch;
            if ((frac += fracstep) >= texheight)
                frac -= texheight;
        }
    }
    else
    {
        // texture height is a power-of-2
        // do some loop unrolling
        while (count >= 8)
        {
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            count -= 8;
        }

        if (count & 1)
        {
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
        }

        if (count & 2)
        {
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
        }

        if (count & 4)
        {
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
            colorfunc(source[(frac >> FRACBITS) & mask], dest);
            dest += pitch;
            frac += fracstep;
        }
    }
}

//
// R_FillSpanGeneric
//
// Templated version of a function to fill a span with a solid color.
// The data type of the destination pixels and a color-remapping functor
// are passed as template parameters.
//
template <typename PIXEL_T, typename COLORFUNC>
static forceinline void R_FillSpanGeneric(PIXEL_T *dest, const drawspan_t &drawspan)
{
#ifdef RANGECHECK
    if (drawspan.x2 < drawspan.x1 || drawspan.x1 < 0 || drawspan.x2 >= viewwidth || drawspan.y >= viewheight ||
        drawspan.y < 0)
    {
        Printf(PRINT_HIGH, "R_FillSpan: %i to %i at %i", drawspan.x1, drawspan.x2, drawspan.y);
        return;
    }
#endif

    int32_t color = drawspan.color;
    int32_t count = drawspan.x2 - drawspan.x1 + 1;
    if (count <= 0)
        return;

    COLORFUNC colorfunc(drawspan);

    do
    {
        colorfunc(color, dest);
        dest++;
    } while (--count);
}

//
// R_DrawLevelSpanGeneric
//
// Templated version of a function to fill a horizontal span with a texture map.
// The data type of the destination pixels and a color-remapping functor
// are passed as template parameters.
//
template <typename PIXEL_T, typename COLORFUNC>
static forceinline void R_DrawLevelSpanGeneric(PIXEL_T *dest, const drawspan_t &drawspan)
{
#ifdef RANGECHECK
    if (drawspan.x2 < drawspan.x1 || drawspan.x1 < 0 || drawspan.x2 >= viewwidth || drawspan.y >= viewheight ||
        drawspan.y < 0)
    {
        Printf(PRINT_HIGH, "R_DrawLevelSpan: %i to %i at %i", drawspan.x1, drawspan.x2, drawspan.y);
        return;
    }
#endif

    palindex_t *source = drawspan.source;
    int32_t     count  = drawspan.x2 - drawspan.x1 + 1;
    if (count <= 0)
        return;

    const uint32_t ubits = dspan.texture_height_bits;
    const uint32_t vbits = dspan.texture_width_bits;

    const uint32_t umask = ((1 << ubits) - 1) << vbits;
    const uint32_t vmask = (1 << vbits) - 1;
    // TODO: don't shift the values of ufrac and vfrac by 10 in R_MapLevelPlane
    const int32_t ushift = FRACBITS - vbits + 10;
    const int32_t vshift = FRACBITS + 10;

    dsfixed_t       vfrac = drawspan.xfrac;
    dsfixed_t       ufrac = drawspan.yfrac;
    const dsfixed_t vstep = drawspan.xstep;
    const dsfixed_t ustep = drawspan.ystep;

    COLORFUNC colorfunc(drawspan);

    do
    {
        // Current texture index in u,v.
        const uint32_t spot = ((ufrac >> ushift) & umask) | ((vfrac >> vshift) & vmask);

        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.

        colorfunc(source[spot], dest);
        dest++;

        // Next step in u,v.
        vfrac += vstep;
        ufrac += ustep;
    } while (--count);
}

//
// R_DrawSlopedSpanGeneric
//
// Texture maps a sloped surface using affine texturemapping for each row of
// the span.  Not as pretty as a perfect texturemapping but should be much
// faster.
//
// Based on R_DrawSlope_8_64 from Eternity Engine, written by SoM/Quasar
//
// The data type of the destination pixels and a color-remapping functor
// are passed as template parameters.
//
template <typename PIXEL_T, typename COLORFUNC>
static forceinline void R_DrawSlopedSpanGeneric(PIXEL_T *dest, const drawspan_t &drawspan)
{
#ifdef RANGECHECK
    if (drawspan.x2 < drawspan.x1 || drawspan.x1 < 0 || drawspan.x2 >= viewwidth || drawspan.y >= viewheight ||
        drawspan.y < 0)
    {
        Printf(PRINT_HIGH, "R_DrawSlopedSpan: %i to %i at %i", drawspan.x1, drawspan.x2, drawspan.y);
        return;
    }
#endif

    palindex_t *source = drawspan.source;
    int32_t     count  = drawspan.x2 - drawspan.x1 + 1;
    if (count <= 0)
        return;

    float       iu = drawspan.iu, iv = drawspan.iv;
    const float ius = drawspan.iustep, ivs = drawspan.ivstep;
    float       id = drawspan.id, ids = drawspan.idstep;

    int32_t ltindex = 0;

    const uint32_t ubits = dspan.texture_height_bits;
    const uint32_t vbits = dspan.texture_width_bits;

    const uint32_t vmask  = ((1 << ubits) - 1) << vbits;
    const uint32_t umask  = (1 << vbits) - 1;
    const int32_t  vshift = FRACBITS - vbits;
    const int32_t  ushift = FRACBITS;

    shaderef_t colormap;
    COLORFUNC  colorfunc(drawspan);

    while (count >= SPANJUMP)
    {
        const float mulstart = 65536.0f / id;
        id += ids * SPANJUMP;
        const float mulend = 65536.0f / id;

        const float ustart = iu * mulstart;
        const float vstart = iv * mulstart;

        fixed_t ufrac = (fixed_t)ustart;
        fixed_t vfrac = (fixed_t)vstart;

        iu += ius * SPANJUMP;
        iv += ivs * SPANJUMP;

        const float uend = iu * mulend;
        const float vend = iv * mulend;

        fixed_t ustep = (fixed_t)((uend - ustart) * INTERPSTEP);
        fixed_t vstep = (fixed_t)((vend - vstart) * INTERPSTEP);

        int32_t incount = SPANJUMP;
        while (incount--)
        {
            colormap = drawspan.slopelighting[ltindex++];

            const uint32_t spot = ((ufrac >> ushift) & umask) | ((vfrac >> vshift) & vmask);
            colorfunc(source[spot], dest);
            dest++;
            ufrac += ustep;
            vfrac += vstep;
        }

        count -= SPANJUMP;
    }

    if (count > 0)
    {
        const float mulstart = 65536.0f / id;
        id += ids * count;
        const float mulend = 65536.0f / id;

        const float ustart = iu * mulstart;
        const float vstart = iv * mulstart;

        fixed_t ufrac = (fixed_t)ustart;
        fixed_t vfrac = (fixed_t)vstart;

        iu += ius * count;
        iv += ivs * count;

        const float uend = iu * mulend;
        const float vend = iv * mulend;

        fixed_t ustep = (fixed_t)((uend - ustart) / count);
        fixed_t vstep = (fixed_t)((vend - vstart) / count);

        int32_t incount = count;
        while (incount--)
        {
            colormap = drawspan.slopelighting[ltindex++];

            const uint32_t spot = ((ufrac >> ushift) & umask) | ((vfrac >> vshift) & vmask);
            colorfunc(source[spot], dest);
            dest++;
            ufrac += ustep;
            vfrac += vstep;
        }
    }
}

/****************************************/
/*										*/
/* [RH] ARGB8888 drawers (C versions)	*/
/*										*/
/****************************************/

// ----------------------------------------------------------------------------
//
// 32bpp color remapping functors
//
// These functors provide a variety of ways to manipulate a source pixel
// color (given by 8bpp palette index) and write the result to the destination
// buffer.
//
// The functors are instantiated with a shaderef_t* parameter (typically
// dcol.colormap or dspan.colormap) that will be used to shade the pixel.
//
// ----------------------------------------------------------------------------

class DirectFunc
{
  public:
    DirectFunc(const drawcolumn_t &drawcolumn)
    {
    }
    DirectFunc(const drawspan_t &drawspan)
    {
    }

    forceinline void operator()(uint8_t c, argb_t *dest) const
    {
        *dest = basecolormap.shade(c);
    }
};

class DirectColormapFunc
{
  public:
    DirectColormapFunc(const drawcolumn_t &drawcolumn) : colormap(drawcolumn.colormap)
    {
    }
    DirectColormapFunc(const drawspan_t &drawspan) : colormap(drawspan.colormap)
    {
    }

    forceinline void operator()(uint8_t c, argb_t *dest) const
    {
        *dest = colormap.shade(c);
    }

  private:
    const shaderef_t &colormap;
};

class DirectFuzzyFunc
{
  public:
    DirectFuzzyFunc(const drawcolumn_t &drawcolumn)
    {
    }

    forceinline void operator()(uint8_t c, argb_t *dest) const
    {
        argb_t work = dest[fuzztable.getValue()];
        *dest       = work - ((work >> 2) & 0x3f3f3f);
        fuzztable.incrementRow();
    }
};

class DirectTranslucentColormapFunc
{
  public:
    DirectTranslucentColormapFunc(const drawcolumn_t &drawcolumn) : colormap(drawcolumn.colormap)
    {
        calculate_alpha(drawcolumn.translevel);
    }

    DirectTranslucentColormapFunc(const drawspan_t &drawspan) : colormap(drawspan.colormap)
    {
        calculate_alpha(drawspan.translevel);
    }

    forceinline void operator()(uint8_t c, argb_t *dest) const
    {
        argb_t fg = colormap.shade(c);
        argb_t bg = *dest;
        *dest     = alphablend2a(bg, bga, fg, fga);
    }

  private:
    void calculate_alpha(fixed_t translevel)
    {
        fga = (translevel & ~0x03FF) >> 8;
        fga = fga > 255 ? 255 : fga;
        bga = 255 - fga;
    }

    const shaderef_t &colormap;
    int32_t           fga, bga;
};

class DirectSlopeColormapFunc
{
  public:
    DirectSlopeColormapFunc(const drawspan_t &drawspan) : colormap(drawspan.slopelighting)
    {
    }

    forceinline void operator()(uint8_t c, argb_t *dest)
    {
        *dest = colormap->shade(c);
        colormap++;
    }

  private:
    const shaderef_t *colormap;
};

// ----------------------------------------------------------------------------
//
// 32bpp color drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_COLDEST_D ((argb_t *)dcol.destination + dcol.yl * dcol.pitch_in_pixels + dcol.x)

//
// R_FillColumnD
//
// Fills a column in the 32bpp ARGB8888 screen buffer with a solid color,
// determined by dcol.color. Performs no shading.
//
void R_FillColumnD()
{
    R_FillColumnGeneric<argb_t, DirectFunc>(FB_COLDEST_D, dcol);
}

//
// R_DrawColumnD
//
// Renders a column to the 32bpp ARGB8888 screen buffer from the source buffer
// dcol.source and scaled by dcol.iscale. Shading is performed using dcol.colormap.
//
void R_DrawColumnD()
{
    R_DrawColumnGeneric<argb_t, DirectColormapFunc>(FB_COLDEST_D, dcol);
}

//
// R_DrawFuzzColumnD
//
// Alters a column in the 32bpp ARGB8888 screen buffer using Doom's partial
// invisibility effect, which shades the column and rearranges the ordering
// the pixels to create distortion. Shading is performed using colormap 6.
//
void R_DrawFuzzColumnD()
{
    // adjust the borders (prevent buffer over/under-reads)
    if (dcol.yl <= 0)
        dcol.yl = 1;
    if (dcol.yh >= viewheight - 1)
        dcol.yh = viewheight - 2;

    R_FillColumnGeneric<argb_t, DirectFuzzyFunc>(FB_COLDEST_D, dcol);
    fuzztable.incrementColumn();
}

//
// R_DrawTranslucentColumnD
//
// Renders a translucent column to the 32bpp ARGB8888 screen buffer from the
// source buffer dcol.source and scaled by dcol.iscale. The amount of
// translucency is controlled by dcol.translevel. Shading is performed using
// dcol.colormap.
//
void R_DrawTranslucentColumnD()
{
    R_DrawColumnGeneric<argb_t, DirectTranslucentColormapFunc>(FB_COLDEST_D, dcol);
}

// ----------------------------------------------------------------------------
//
// 32bpp color span drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_SPANDEST_D ((argb_t *)dspan.destination + dspan.y * dspan.pitch_in_pixels + dspan.x1)

//
// R_FillSpanD
//
// Fills a span in the 32bpp ARGB8888 screen buffer with a solid color,
// determined by dspan.color. Performs no shading.
//
void R_FillSpanD()
{
    R_FillSpanGeneric<argb_t, DirectFunc>(FB_SPANDEST_D, dspan);
}

//
// R_FillTranslucentSpanD
//
// Fills a span in the 32bpp ARGB8888 screen buffer with a solid color,
// determined by dspan.color using translucency. Shading is performed
// using dspan.colormap.
//
void R_FillTranslucentSpanD()
{
    R_FillSpanGeneric<argb_t, DirectTranslucentColormapFunc>(FB_SPANDEST_D, dspan);
}

//
// R_DrawSpanD
//
// Renders a span for a level plane to the 32bpp ARGB8888 screen buffer from
// the source buffer dspan.source. Shading is performed using dspan.colormap.
//
void R_DrawSpanD_c()
{
    R_DrawLevelSpanGeneric<argb_t, DirectColormapFunc>(FB_SPANDEST_D, dspan);
}

//
// R_DrawSlopeSpanD
//
// Renders a span for a sloped plane to the 32bpp ARGB8888 screen buffer from
// the source buffer dspan.source. Shading is performed using dspan.colormap.
//
void R_DrawSlopeSpanD_c()
{
    R_DrawSlopedSpanGeneric<argb_t, DirectSlopeColormapFunc>(FB_SPANDEST_D, dspan);
}

/****************************************************/

//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void V_MarkRect(int32_t x, int32_t y, int32_t width, int32_t height);

enum r_optimize_kind
{
    OPTIMIZE_NONE,
    OPTIMIZE_SSE2,
    OPTIMIZE_MMX,
    OPTIMIZE_ALTIVEC
};

static r_optimize_kind              optimize_kind = OPTIMIZE_NONE;
static std::vector<r_optimize_kind> optimizations_available;

static const char *get_optimization_name(r_optimize_kind kind)
{
    switch (kind)
    {
    case OPTIMIZE_SSE2:
        return "sse2";
    case OPTIMIZE_MMX:
        return "mmx";
    case OPTIMIZE_ALTIVEC:
        return "altivec";
    case OPTIMIZE_NONE:
    default:
        return "none";
    }
}

static std::string get_optimization_name_list(const bool includeNone)
{
    std::string                                  str;
    std::vector<r_optimize_kind>::const_iterator it = optimizations_available.begin();
    if (!includeNone)
        ++it;

    for (; it != optimizations_available.end(); ++it)
    {
        str.append(get_optimization_name(*it));
        if (it + 1 != optimizations_available.end())
            str.append(", ");
    }
    return str;
}

static void print_optimizations()
{
    Printf(PRINT_HIGH, "r_optimize detected \"%s\"\n", get_optimization_name_list(false).c_str());
}

static bool detect_optimizations()
{
    if (!optimizations_available.empty())
        return false;

    optimizations_available.clear();

    // Start with default non-optimized:
    optimizations_available.push_back(OPTIMIZE_NONE);

// Detect CPU features in ascending order of preference:
#ifdef __MMX__
    if (SDL_HasMMX())
        optimizations_available.push_back(OPTIMIZE_MMX);
#endif
#ifdef __SSE2__
    if (SDL_HasSSE2())
        optimizations_available.push_back(OPTIMIZE_SSE2);
#endif
#ifdef __ALTIVEC__
    if (SDL_HasAltiVec())
        optimizations_available.push_back(OPTIMIZE_ALTIVEC);
#endif

    return true;
}

//
// R_IsOptimizationAvailable
//
// Returns true if Odamex was compiled with support for the optimization
// and the current CPU also supports it.
//
static bool R_IsOptimizationAvailable(r_optimize_kind kind)
{
    return std::find(optimizations_available.begin(), optimizations_available.end(), kind) !=
           optimizations_available.end();
}

CVAR_FUNC_IMPL(r_optimize)
{
    const char *val = var.cstring();

    // Only print the detected list the first time:
    if (detect_optimizations())
        print_optimizations();

    // Set the optimization based on availability:
    if (stricmp(val, "none") == 0)
        optimize_kind = OPTIMIZE_NONE;
    else if (stricmp(val, "sse2") == 0 && R_IsOptimizationAvailable(OPTIMIZE_SSE2))
        optimize_kind = OPTIMIZE_SSE2;
    else if (stricmp(val, "mmx") == 0 && R_IsOptimizationAvailable(OPTIMIZE_MMX))
        optimize_kind = OPTIMIZE_MMX;
    else if (stricmp(val, "altivec") == 0 && R_IsOptimizationAvailable(OPTIMIZE_ALTIVEC))
        optimize_kind = OPTIMIZE_ALTIVEC;
    else if (stricmp(val, "detect") == 0)
        // Default to the most preferred:
        optimize_kind = optimizations_available.back();
    else
    {
        Printf(PRINT_HIGH, "Invalid value for r_optimize. Availible options are \"%s, detect\"\n",
               get_optimization_name_list(true).c_str());

        // Restore the original setting:
        var.Set(get_optimization_name(optimize_kind));
        return;
    }

    const char *optimize_name = get_optimization_name(optimize_kind);
    if (stricmp(val, optimize_name) != 0)
    {
        // update the cvar string
        // this will trigger the callback to run a second time
        Printf(PRINT_HIGH, "r_optimize set to \"%s\" based on availability\n", optimize_name);
        var.Set(optimize_name);
    }
    else
    {
        // cvar string is current, now intialize the drawing function pointers
        R_InitVectorizedDrawers();
        R_InitColumnDrawers();
    }
}

//
// R_InitVectorizedDrawers
//
// Sets up the function pointers based on CPU optimization selected.
//
void R_InitVectorizedDrawers()
{
    if (optimize_kind == OPTIMIZE_NONE)
    {
        // [SL] set defaults to non-vectorized drawers
        R_DrawSpanD      = R_DrawSpanD_c;
        R_DrawSlopeSpanD = R_DrawSlopeSpanD_c;
        r_dimpatchD      = r_dimpatchD_c;
    }
#ifdef __SSE2__
    if (optimize_kind == OPTIMIZE_SSE2)
    {
        R_DrawSpanD      = R_DrawSpanD_SSE2;
        R_DrawSlopeSpanD = R_DrawSlopeSpanD_SSE2;
        r_dimpatchD      = r_dimpatchD_SSE2;
    }
#endif
#ifdef __MMX__
    else if (optimize_kind == OPTIMIZE_MMX)
    {
        R_DrawSpanD      = R_DrawSpanD_c;      // TODO
        R_DrawSlopeSpanD = R_DrawSlopeSpanD_c; // TODO
        r_dimpatchD      = r_dimpatchD_MMX;
    }
#endif
#ifdef __ALTIVEC__
    else if (optimize_kind == OPTIMIZE_ALTIVEC)
    {
        R_DrawSpanD      = R_DrawSpanD_c;      // TODO
        R_DrawSlopeSpanD = R_DrawSlopeSpanD_c; // TODO
        r_dimpatchD      = r_dimpatchD_ALTIVEC;
    }
#endif

    // Check that all pointers are definitely assigned!
    assert(R_DrawSpanD != NULL);
    assert(R_DrawSlopeSpanD != NULL);
    assert(r_dimpatchD != NULL);
}

// [RH] Initialize the column drawer pointers
void R_InitColumnDrawers()
{
    if (!I_VideoInitialized())
        return;

    R_DrawColumn            = R_DrawColumnD;
    R_DrawFuzzColumn        = R_DrawFuzzColumnD;
    R_DrawTranslucentColumn = R_DrawTranslucentColumnD;
    R_DrawSlopeSpan         = R_DrawSlopeSpanD;
    R_DrawSpan              = R_DrawSpanD;
    R_FillColumn            = R_FillColumnD;
    R_FillSpan              = R_FillSpanD;
    R_FillTranslucentSpan   = R_FillTranslucentSpanD;
}

VERSION_CONTROL(r_draw_cpp, "$Id: f878e66e7201c97021a528e052d53ef9a0811b5e $")
