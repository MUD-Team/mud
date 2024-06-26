// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: db32585620dcb1455275de16ba45b51e9b434d16 $
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

#include "m_alloc.h"
#include "mud_includes.h"
#include "r_common.h"
#include "v_video.h"
#include "z_zone.h"

// Declared in doomtype.h as part of argb_t
uint8_t argb_t::a_num, argb_t::r_num, argb_t::g_num, argb_t::b_num;

dyncolormap_t NormalLight;

/****************************/
/* Palette management stuff */
/****************************/

palindex_t V_BestColor(const argb_t *palette_colors, int32_t r, int32_t g, int32_t b)
{
    return 0;
}

palindex_t V_BestColor(const argb_t *palette_colors, argb_t color)
{
    return 0;
}

static palette_t default_palette;

const palette_t *V_GetDefaultPalette()
{
    return &default_palette;
}

const palette_t *V_GetGamePalette()
{
    return &default_palette;
}

void V_InitPalette()
{
    static bool initialized = false;

    if (!initialized)
    {
        // construct a valid palette_t so we don't get crashes
        memset(default_palette.basecolors, 0, 256 * sizeof(*default_palette.basecolors));
        memset(default_palette.colors, 0, 256 * sizeof(*default_palette.colors));

        default_palette.maps.colormap = NULL;
        default_palette.maps.shademap = NULL;

        initialized = true;
    }
}

shaderef_t::shaderef_t() : m_colors(NULL), m_mapnum(-1), m_colormap(NULL), m_shademap(NULL)
{
}

shaderef_t::shaderef_t(const shaderef_t &other)
    : m_colors(other.m_colors), m_mapnum(other.m_mapnum), m_colormap(other.m_colormap), m_shademap(other.m_shademap),
      m_dyncolormap(other.m_dyncolormap)
{
}

shaderef_t::shaderef_t(const shademap_t *const colors, const int32_t mapnum) : m_colors(colors), m_mapnum(mapnum)
{
#if ODAMEX_DEBUG
    // NOTE(jsd): Arbitrary value picked here because we don't record the max number of colormaps for dynamic ones... or
    // do we?
    if (m_mapnum >= 8192)
    {
        char tmp[100];
        sprintf(tmp, "32bpp: shaderef_t::shaderef_t() called with mapnum = %d, which looks too large", m_mapnum);
        throw CDoomError(tmp);
    }
#endif

    if (m_colors != NULL)
    {
        if (m_colors->colormap != NULL)
            m_colormap = m_colors->colormap + (256 * m_mapnum);
        else
            m_colormap = NULL;

        if (m_colors->shademap != NULL)
            m_shademap = m_colors->shademap + (256 * m_mapnum);
        else
            m_shademap = NULL;

        // Detect if the colormap is dynamic:
        m_dyncolormap = NULL;

        if (m_colors != &(V_GetDefaultPalette()->maps))
        {
            // Find the dynamic colormap by the `m_colors` pointer:
            extern dyncolormap_t NormalLight;
            dyncolormap_t       *colormap = &NormalLight;

            do
            {
                if (m_colors == colormap->maps.m_colors)
                {
                    m_dyncolormap = colormap;
                    break;
                }
                colormap = colormap->next;
            } while (colormap);
        }
    }
    else
    {
        m_colormap    = NULL;
        m_shademap    = NULL;
        m_dyncolormap = NULL;
    }
}

/****** Colored Lighting Stuffs (Sorry, 8-bit only) ******/

void BuildDefaultShademap(const palette_t *pal, shademap_t &maps)
{
}

dyncolormap_t *GetSpecialLights(int32_t lr, int32_t lg, int32_t lb, int32_t fr, int32_t fg, int32_t fb)
{
    argb_t         color(lr, lg, lb);
    argb_t         fade(fr, fg, fb);
    dyncolormap_t *colormap = &NormalLight;

    // Bah! Simple linear search because I want to get this done.
    while (colormap)
    {
        if (color == colormap->color && fade == colormap->fade)
            return colormap;
        else
            colormap = colormap->next;
    }

    // Not found. Create it.
    colormap         = (dyncolormap_t *)Z_Malloc(sizeof(*colormap), PU_LEVEL, 0);
    shademap_t *maps = new shademap_t();
    maps->colormap   = (uint8_t *)Z_Malloc(NUMCOLORMAPS * 256 * sizeof(uint8_t) + 3 + 255, PU_LEVEL, 0);
    maps->colormap   = (uint8_t *)(((ptrdiff_t)maps->colormap + 255) & ~0xff);
    maps->shademap   = (argb_t *)Z_Malloc(NUMCOLORMAPS * 256 * sizeof(argb_t) + 3 + 255, PU_LEVEL, 0);
    maps->shademap   = (argb_t *)(((ptrdiff_t)maps->shademap + 255) & ~0xff);

    colormap->maps   = shaderef_t(maps, 0);
    colormap->color  = color;
    colormap->fade   = fade;
    colormap->next   = NormalLight.next;
    NormalLight.next = colormap;

    // [AM] We don't keep the necessary palette info on the server to do this.
    // BuildColoredLights (maps, lr, lg, lb, fr, fg, fb);

    return colormap;
}

VERSION_CONTROL(v_palette_cpp, "$Id: db32585620dcb1455275de16ba45b51e9b434d16 $")
