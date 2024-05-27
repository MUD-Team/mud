// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: adae0296eb3e28c00375589b51c81413ecdda97c $
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
//	V_PALETTE
//
//-----------------------------------------------------------------------------

#include "v_palette.h"

#include <math.h>

#include <cassert>

#include "c_dispatch.h"
#include "cmdlib.h"
#include "i_system.h"
#include "i_video.h"
#include "m_alloc.h"
#include "m_fileio.h"
#include "mud_includes.h"
#include "r_main.h" // For lighting constants
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

static palette_t  default_palette;
static palette_t  game_palette;
static shaderef_t V_Palette;
//
// V_GetDefaultPalette
//
// Returns a pointer to the default palette for the video subsystem. The
// palette returned should be the default palette defined in the PLAYPAL lump with the
// user's gamma correction setting applied.
//
const palette_t *V_GetDefaultPalette()
{
    return &default_palette;
}

//
// V_GetGamePalette
//
// Returns a pointer to the game palette that is used in 8bpp video modes. The
// palette returned is chosen from the palettes in the PLAYPAL lump based on
// the displayplayer's current game status (eg, recently was damaged, wearing
// radiation suite, etc.).
//
const palette_t *V_GetGamePalette()
{
    return &game_palette;
}

// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS   1
#define STARTBONUSPALS 9
#define NUMREDPALS     8
#define NUMBONUSPALS   4
// Radiation suit, green shift.
#define RADIATIONPAL 13

EXTERN_CVAR(gammalevel)
EXTERN_CVAR(vid_gammatype)
EXTERN_CVAR(r_painintensity)
EXTERN_CVAR(sv_allowredscreen)

dyncolormap_t NormalLight;

static int32_t current_palette_num;

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
        throw CFatalError(tmp);
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
            dyncolormap_t *colormap = &NormalLight;

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

// ----------------------------------------------------------------------------
//
// Gamma Correction
//
// ----------------------------------------------------------------------------

uint8_t gammatable[256];

enum
{
    GAMMA_DOOM  = 0,
    GAMMA_ZDOOM = 1
};

EXTERN_CVAR(vid_gammatype)
EXTERN_CVAR(gammalevel)

//
// GammaStrategy
//
// Encapsulate the differences of the Doom and ZDoom gamma types with
// a strategy pattern. Provides a common interface for generation of gamma
// tables.
//
class GammaStrategy
{
  public:
    virtual float min() const                                        = 0;
    virtual float max() const                                        = 0;
    virtual float increment(float level) const                       = 0;
    virtual void  generateGammaTable(uint8_t *table, float level) const = 0;
};

class DoomGammaStrategy : public GammaStrategy
{
  public:
    float min() const
    {
        return 0.0f;
    }

    float max() const
    {
        return 7.0f;
    }

    float increment(float level) const
    {
        level += 1.0f;
        if (level > max())
            level = min();
        return level;
    }

    void generateGammaTable(uint8_t *table, float level) const
    {
        // [SL] Use vanilla Doom's gamma table
        //
        // This was derived from the original Doom gammatable after some
        // trial and error and several beers.  The +0.5 is used to round
        // while the 255/256 is to scale to ensure 255 isn't exceeded.
        // This generates a 1:1 match with the original gammatable but also
        // allows for intermediate values.

        const double basefac = pow(2.0, (double)level) * (255.0 / 256.0);
        const double exp     = 1.0 - 0.125 * level;

        for (int32_t i = 0; i < 256; i++)
            table[i] = (uint8_t)(0.5 + basefac * pow(double(i) + 1.0, exp));
    }
};

class ZDoomGammaStrategy : public GammaStrategy
{
  public:
    float min() const
    {
        return 0.5f;
    }

    float max() const
    {
        return 3.0f;
    }

    float increment(float level) const
    {
        level += 0.1f;
        if (level > max())
            level = min();
        return level;
    }

    void generateGammaTable(uint8_t *table, float level) const
    {
        // [SL] Use ZDoom 1.22 gamma correction

        // [RH] I found this formula on the web at
        // http://panda.mostang.com/sane/sane-gamma.html

        double invgamma = 1.0 / level;

        for (int32_t i = 0; i < 256; i++)
            table[i] = (uint8_t)(255.0 * pow(double(i) / 255.0, invgamma));
    }
};

static DoomGammaStrategy  doomgammastrat;
static ZDoomGammaStrategy zdoomgammastrat;
GammaStrategy            *gammastrat = &doomgammastrat;

float V_GetMinimumGammaLevel()
{
    return gammastrat->min();
}

float V_GetMaximumGammaLevel()
{
    return gammastrat->max();
}

void V_IncrementGammaLevel()
{
    float level(gammalevel.value());
    gammalevel.Set(gammastrat->increment(level));
}

//
// V_GammaAdjustPalette
//
static void V_GammaAdjustPalette(palette_t *palette)
{
    const argb_t *from = palette->basecolors;
    argb_t       *to   = palette->colors;

    for (int32_t i = 0; i < 256; i++)
        *to++ = V_GammaCorrect(*from++);
}

//
// V_UpdateGammaLevel
//
// Calls the concrete GammaStrategy generateGammaTable function to populate
// the gammatable array. The palette is also gamma-corrected.
//
static void V_UpdateGammaLevel(float level)
{
    static float lastgammalevel = 0.0f;
    static int32_t   lasttype       = -1; // ensure this gets set up the first time
    int32_t          type           = vid_gammatype;

    if (lastgammalevel != level || lasttype != type)
    {
        // Only recalculate the gamma table if the new gamma
        // value is different from the old one.

        lastgammalevel = level;
        lasttype       = type;

        gammastrat->generateGammaTable(gammatable, level);
        V_GammaAdjustPalette(&default_palette);

        V_RestoreScreenPalette();

        V_RefreshColormaps();
    }
}

//
// vid_gammatype
//
// Changes gammastrat to a new concrete GammaStrategy and forces the palette
// to be gamma-corrected.
//
CVAR_FUNC_IMPL(vid_gammatype)
{
    if (vid_gammatype == GAMMA_ZDOOM)
        gammastrat = &zdoomgammastrat;
    else
        gammastrat = &doomgammastrat;

    gammalevel.Set(gammalevel);
}

//
// gammalevel
//
// Specifies the gamma correction level. The level is clamped to the concrete
// GammaStrategy's minimum and maximum values prior to updating gammatable by
// calling V_UpdateGammaLevel.
//
CVAR_FUNC_IMPL(gammalevel)
{
    float sanitized_var = clamp(var.value(), gammastrat->min(), gammastrat->max());
    if (var == sanitized_var)
        V_UpdateGammaLevel(var);
    else
        var.Set(sanitized_var);
}

//
// bumpgamma
//
// Increments gammalevel by a value controlled by the concrete GammaStrategy.
//
BEGIN_COMMAND(bumpgamma)
{
    V_IncrementGammaLevel();

    if (gammalevel.value() == 0.0f)
        Printf(PRINT_HIGH, "Gamma correction off\n");
    else
        Printf(PRINT_HIGH, "Gamma correction level %g\n", gammalevel.value());
}
END_COMMAND(bumpgamma)

// [Russell] - Restore original screen palette from current gamma level
void V_RestoreScreenPalette()
{
}

//
// V_BestColor
//
// (borrowed from Quake2 source: utils3/qdata/images.c)
// [SL] Also nearly identical to BestColor in dcolors.c in Doom utilites
//
palindex_t V_BestColor(const argb_t *palette_colors, int32_t r, int32_t g, int32_t b)
{
    int32_t bestdistortion = INT32_MAX;
    int32_t bestcolor      = 0; /// let any color go to 0 as a last resort

    for (int32_t i = 0; i < 256; i++)
    {
        argb_t color(palette_colors[i]);

        int32_t dr         = r - color.getr();
        int32_t dg         = g - color.getg();
        int32_t db         = b - color.getb();
        int32_t distortion = dr * dr + dg * dg + db * db;
        if (distortion < bestdistortion)
        {
            if (distortion == 0)
                return i; // perfect match

            bestdistortion = distortion;
            bestcolor      = i;
        }
    }

    return bestcolor;
}

palindex_t V_BestColor(const argb_t *palette_colors, argb_t color)
{
    return V_BestColor(palette_colors, color.getr(), color.getg(), color.getb());
}

//
// V_ClosestColors
//
// Sets color1 and color2 to the palette indicies of the pair of colors that
// are the closest amongst the colors of the given palette. This is an N^2
// algorithm so use sparingly.
//
void V_ClosestColors(const argb_t *palette_colors, palindex_t &color1, palindex_t &color2)
{
    int32_t bestdistortion = INT32_MAX;

    color1 = color2 = 0; // go to color 0 as a last resort

    for (int32_t x = 0; x < 256; x++)
    {
        for (int32_t y = 0; y < 256 - x; y++)
        {
            // don't compare a color with itself
            if (x == y)
                continue;

            int32_t dr         = (int32_t)palette_colors[y].getr() - (int32_t)palette_colors[x].getr();
            int32_t dg         = (int32_t)palette_colors[y].getg() - (int32_t)palette_colors[x].getg();
            int32_t db         = (int32_t)palette_colors[y].getb() - (int32_t)palette_colors[x].getb();
            int32_t distortion = dr * dr + dg * dg + db * db;
            if (distortion < bestdistortion)
            {
                color1 = x, color2 = y;
                bestdistortion = distortion;
                if (bestdistortion == 0)
                    return; // perfect match
            }
        }
    }
}

static std::string V_GetColorStringByName(const std::string &name);

//
// V_GetColorFromString
//
// Parses a string of 6 hexadecimal digits representing an RGB triplet
// and converts it into an argb_t value. It will also accept the name of a
// color, as defined in the X11R6RGB lump, using V_GetColorStringByName
// to look up the RGB triplet value.
//
argb_t V_GetColorFromString(const std::string &input_string)
{
    // first check if input_string is the name of a color
    const std::string color_name_string = V_GetColorStringByName(input_string);

    // if not a valid color name, try to parse the color channel values
    const char *str = color_name_string.empty() == false ? color_name_string.c_str() : input_string.c_str();

    int32_t         c[3], i, p;
    char        val[5];
    const char *s, *g;

    val[4] = 0;
    for (s = str, i = 0; i < 3; i++)
    {
        c[i] = 0;

        while ((*s <= ' ') && (*s != 0))
            s++;

        if (*s)
        {
            p = 0;

            while (*s > ' ')
            {
                if (p < 4)
                    val[p++] = *s;

                s++;
            }

            g = val;

            while (p < 4)
                val[p++] = *g++;

            c[i] = ParseHex(val);
        }
    }

    return argb_t(c[0] >> 8, c[1] >> 8, c[2] >> 8);
}

/****************************/
/* Palette management stuff */
/****************************/

//
// V_InitPalette
//
// Initializes the default palette, loading the raw palette lump resource.
//
void V_InitPalette(const char *filename)
{
    std::string palfile = StrFormat("lumps/%s", filename);

    if (!M_FileExists(palfile))
        I_FatalError("Could not initialize %s palette file", palfile.c_str());

    current_palette_num = -1;

    if (default_palette.maps.colormap)
        delete[] default_palette.maps.colormap;
    if (default_palette.maps.shademap)
        delete[] default_palette.maps.shademap;

    PHYSFS_File *rawpal = PHYSFS_openRead(palfile.c_str());

    if (rawpal == NULL)
        I_FatalError("Could not open %s palette file", palfile.c_str());

    uint8_t *data = new uint8_t[PHYSFS_fileLength(rawpal)];

    if (PHYSFS_readBytes(rawpal, data, PHYSFS_fileLength(rawpal)) != PHYSFS_fileLength(rawpal))
    {
        PHYSFS_close(rawpal);
        delete[] data;
        I_FatalError("Could not read %s palette file", palfile.c_str());
    }

    PHYSFS_close(rawpal);

    default_palette.maps.colormap = new palindex_t[(NUMCOLORMAPS + 1) * 256];
    default_palette.maps.shademap = new argb_t[(NUMCOLORMAPS + 1) * 256];

    for (int32_t i = 0; i < 256; i++)
        default_palette.basecolors[i] = argb_t(255, data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2]);

    delete[] data;

    V_GammaAdjustPalette(&default_palette);

    V_ForceBlend(argb_t(0, 255, 255, 255));

    V_RefreshColormaps();

    V_ResetPalette();

    assert(default_palette.maps.colormap != NULL);
    assert(default_palette.maps.shademap != NULL);
    V_Palette = shaderef_t(&default_palette.maps, 0);

    game_palette = default_palette;
}

// This is based (loosely) on the ColorShiftPalette()
// function from the dcolors.c file in the Doom utilities.
static void V_DoBlending(argb_t *dest, const argb_t *source, argb_t color)
{
    if (color.geta() == 0)
    {
        if (source != dest)
            memcpy(dest, source, 256 * sizeof(argb_t));
    }
    else
    {
        for (int32_t i = 0; i < 256; i++, source++, dest++)
        {
            int32_t fromr = source->getr();
            int32_t fromg = source->getg();
            int32_t fromb = source->getb();

            int32_t toa = color.geta();
            int32_t tor = color.getr();
            int32_t tog = color.getg();
            int32_t tob = color.getb();

            int32_t dr = tor - fromr;
            int32_t dg = tog - fromg;
            int32_t db = tob - fromb;

            argb_t newcolor(source->geta(), fromr + ((dr * toa) >> 8), fromg + ((dg * toa) >> 8),
                            fromb + ((db * toa) >> 8));

            *dest = newcolor;
        }
    }
}

static const float lightScale(float a)
{
    // NOTE(jsd): Revised inverse logarithmic scale; near-perfect match to COLORMAP lump's scale
    // 1 - ((Exp[1] - Exp[a*2 - 1]) / (Exp[1] - Exp[-1]))
    static float e1      = exp(1.0f);
    static float e1sube0 = e1 - exp(-1.0f);

    float newa = clamp(1.0f - (e1 - (float)exp(a * 2.0f - 1.0f)) / e1sube0, 0.0f, 1.0f);
    return newa;
}

void BuildLightRamp(shademap_t &maps)
{
    int32_t l;
    // Build light ramp:
    for (l = 0; l < 256; ++l)
    {
        int32_t a        = (int32_t)(255 * lightScale(l / 255.0f));
        maps.ramp[l] = a;
    }
}

void BuildDefaultColorAndShademap(const palette_t *pal, shademap_t &maps)
{
    BuildLightRamp(maps);

    // [SL] Modified algorithm from RF_BuildLights in dcolors.c
    // from Doom Utilities. Now accomodates fading to non-black colors.

    const argb_t *palette = pal->basecolors;
    argb_t        fadecolor(level.fadeto_color[0], level.fadeto_color[1], level.fadeto_color[2], level.fadeto_color[3]);

    palindex_t *colormap = maps.colormap;
    argb_t     *shademap = maps.shademap;

    for (int32_t i = 0; i < NUMCOLORMAPS; i++, colormap += 256, shademap += 256)
    {
        for (int32_t c = 0; c < 256; c++)
        {
            uint32_t r =
                (palette[c].getr() * (NUMCOLORMAPS - i) + fadecolor.getr() * i + NUMCOLORMAPS / 2) / NUMCOLORMAPS;
            uint32_t g =
                (palette[c].getg() * (NUMCOLORMAPS - i) + fadecolor.getg() * i + NUMCOLORMAPS / 2) / NUMCOLORMAPS;
            uint32_t b =
                (palette[c].getb() * (NUMCOLORMAPS - i) + fadecolor.getb() * i + NUMCOLORMAPS / 2) / NUMCOLORMAPS;

            argb_t color(255, r, g, b);
            colormap[c] = V_BestColor(palette, color);
            shademap[c] = V_GammaCorrect(color);
        }
    }

    // build special maps (e.g. invulnerability)
    for (int32_t c = 0; c < 256; c++)
    {
        int32_t grayint = (int32_t)(255.0f * clamp(1.0f - (palette[c].getr() * 0.00116796875f +
                                                   palette[c].getg() * 0.00229296875f + palette[c].getb() * 0.0005625f),
                                           0.0f, 1.0f));

        argb_t color(255, grayint, grayint, grayint);
        colormap[c] = V_BestColor(palette, color);
        shademap[c] = V_GammaCorrect(color);
    }
}

void BuildDefaultShademap(const palette_t *pal, shademap_t &maps)
{
    BuildLightRamp(maps);

    // [SL] Modified algorithm from RF_BuildLights in dcolors.c
    // from Doom Utilities. Now accomodates fading to non-black colors.

    const argb_t *palette = pal->basecolors;
    argb_t        fadecolor(level.fadeto_color[0], level.fadeto_color[1], level.fadeto_color[2], level.fadeto_color[3]);

    argb_t *shademap = maps.shademap;

    for (int32_t i = 0; i < NUMCOLORMAPS; i++, shademap += 256)
    {
        for (int32_t c = 0; c < 256; c++)
        {
            uint32_t r =
                (palette[c].getr() * (NUMCOLORMAPS - i) + fadecolor.getr() * i + NUMCOLORMAPS / 2) / NUMCOLORMAPS;
            uint32_t g =
                (palette[c].getg() * (NUMCOLORMAPS - i) + fadecolor.getg() * i + NUMCOLORMAPS / 2) / NUMCOLORMAPS;
            uint32_t b =
                (palette[c].getb() * (NUMCOLORMAPS - i) + fadecolor.getb() * i + NUMCOLORMAPS / 2) / NUMCOLORMAPS;

            argb_t color(255, r, g, b);
            shademap[c] = V_GammaCorrect(color);
        }
    }

    // build special maps (e.g. invulnerability)
    for (int32_t c = 0; c < 256; c++)
    {
        int32_t grayint = (int32_t)(255.0f * clamp(1.0f - (palette[c].getr() * 0.00116796875f +
                                                   palette[c].getg() * 0.00229296875f + palette[c].getb() * 0.0005625f),
                                           0.0f, 1.0f));

        argb_t color(255, grayint, grayint, grayint);
        shademap[c] = V_GammaCorrect(color);
    }
}

//
// V_RefreshColormaps
//
void V_RefreshColormaps()
{
    BuildDefaultColorAndShademap(&default_palette, default_palette.maps);

    NormalLight.maps  = shaderef_t(&default_palette.maps, 0);
    NormalLight.color = argb_t(255, 255, 255, 255);
    NormalLight.fade =
        argb_t(level.fadeto_color[0], level.fadeto_color[1], level.fadeto_color[2], level.fadeto_color[3]);
}

//
// V_AddBlend
//
// Blends an ARGB color with an existing ARGB color blend.
//
// [RH] This is from Q2.
// [SL] Modified slightly to use fargb_t types.
//
static void V_AddBlend(fargb_t &blend, const fargb_t &newcolor)
{
    if (newcolor.geta() <= 0.0f)
        return;

    float a          = blend.geta() + newcolor.geta() * (1.0f - blend.geta());
    float old_amount = blend.geta() / a;

    blend.seta(a);
    blend.setr(blend.getr() * old_amount + newcolor.getr() * (1.0f - old_amount));
    blend.setg(blend.getg() * old_amount + newcolor.getg() * (1.0f - old_amount));
    blend.setb(blend.getb() * old_amount + newcolor.getb() * (1.0f - old_amount));
}

//
// V_ForceBlend
//
// Normally, V_SetBlend does nothing if the new blend is the
// same as the old. This function will perform the blending
// even if the blend hasn't changed.
//
void V_ForceBlend(const argb_t color)
{
}

//
// V_SetBlend
//
// Sets the global blending color and blends the color with the default
// palette and passes the palette to the video hardware (in 8bpp mode).
//
//
void V_SetBlend(const argb_t color)
{
    // Don't do anything if the new blend is the same as the old
    if (blend_color.geta() == 0 && color.geta() == 0)
        return;

    if (blend_color.geta() == color.geta() && blend_color.getr() == color.getr() &&
        blend_color.getg() == color.getg() && blend_color.getb() == color.getb())
        return;

    V_ForceBlend(color);
}

BEGIN_COMMAND(testblend)
{
    if (argc < 3)
    {
        Printf(PRINT_HIGH, "testblend <color> <amount>\n");
    }
    else
    {
        argb_t color(V_GetColorFromString(argv[1]));

        int32_t alpha = 255.0 * clamp((float)atof(argv[2]), 0.0f, 1.0f);
        R_SetSectorBlend(argb_t(alpha, color.getr(), color.getg(), color.getb()));
    }
}
END_COMMAND(testblend)

BEGIN_COMMAND(testfade)
{
    if (argc < 2)
    {
        Printf(PRINT_HIGH, "testfade <color>\n");
    }
    else
    {
        argb_t color(V_GetColorFromString(argv[1]));

        level.fadeto_color[0] = color.geta();
        level.fadeto_color[1] = color.getr();
        level.fadeto_color[2] = color.getg();
        level.fadeto_color[3] = color.getb();

        V_RefreshColormaps();
        NormalLight.maps = shaderef_t(&V_GetDefaultPalette()->maps, 0);
    }
}
END_COMMAND(testfade)

/****** Colorspace Conversion Functions ******/

//
// V_RGBtoHSV
//
// Converts from the RGB color space to the HSV color space.
// Code from http://www.cs.rit.edu/~yxv4997/t_convert.html
//
// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
// RGB - 0: {    .46  1 .429 } 7: {    .254 .571 .206 } 15: {    .0317 .0794 .0159 }
// HSV - 0: { 116.743 .571 1 } 7: { 112.110 .639 .571 } 15: { 105.071  .800 .0794 }
//
fahsv_t V_RGBtoHSV(const fargb_t &color)
{
    float a = color.geta(), r = color.getr(), g = color.getg(), b = color.getb();

    float smallest = std::min(std::min(r, g), b);
    float largest  = std::max(std::max(r, g), b);
    float delta    = largest - smallest;

    if (delta == 0.0f)
        return fahsv_t(a, 0, 0, largest);

    float hue;

    if (largest == r)
        hue = (g - b) / delta;        // between yellow & magenta
    else if (largest == g)
        hue = 2.0f + (b - r) / delta; // between cyan & yellow
    else
        hue = 4.0f + (r - g) / delta; // between magenta & cyan

    hue *= 60.f;
    if (hue < 0.0f)
        hue += 360.0f;

    return fahsv_t(a, hue, delta / largest, largest);
}

//
// V_HSVtoRGB
//
// Converts from the HSV color space to the RGB color space.
//
fargb_t V_HSVtoRGB(const fahsv_t &color)
{
    float a = color.geta(), h = color.geth(), s = color.gets(), v = color.getv();

    if (s == 0.0f) // achromatic (grey)
        return fargb_t(a, v, v, v);

    float f = (h / 60.0f) - floor(h / 60.0f);
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    int32_t sector = int32_t(h / 60.0f);
    switch (sector)
    {
    case 0:
        return fargb_t(a, v, t, p);
    case 1:
        return fargb_t(a, q, v, p);
    case 2:
        return fargb_t(a, p, v, t);
    case 3:
        return fargb_t(a, p, q, v);
    case 4:
        return fargb_t(a, t, p, v);
    case 5:
        return fargb_t(a, v, p, q);
    }

    return fargb_t(a, v, v, v);
}

/****** Colored Lighting Stuffs (Sorry, 8-bit only) ******/

// Builds NUMCOLORMAPS colormaps lit with the specified color
static void BuildColoredLights(shademap_t *maps, const int32_t lr, const int32_t lg, const int32_t lb, const int32_t fr, const int32_t fg,
                               const int32_t fb)
{
    // The default palette is assumed to contain the maps for white light.
    if (!maps)
        return;

    BuildLightRamp(*maps);

    const argb_t *palette_colors = V_GetDefaultPalette()->basecolors;

    // build normal (but colored) light mappings
    for (uint32_t l = 0; l < NUMCOLORMAPS; l++)
    {
        // Build the colormap and shademap:
        palindex_t *colormap = maps->colormap + 256 * l;
        argb_t     *shademap = maps->shademap + 256 * l;
        for (uint32_t c = 0; c < 256; c++)
        {
            uint32_t r = (palette_colors[c].getr() * (NUMCOLORMAPS - l) + fr * l + NUMCOLORMAPS / 2) / NUMCOLORMAPS;
            uint32_t g = (palette_colors[c].getg() * (NUMCOLORMAPS - l) + fg * l + NUMCOLORMAPS / 2) / NUMCOLORMAPS;
            uint32_t b = (palette_colors[c].getb() * (NUMCOLORMAPS - l) + fb * l + NUMCOLORMAPS / 2) / NUMCOLORMAPS;
            argb_t       color(255, r * lr / 255, g * lg / 255, b * lb / 255);

            shademap[c] = V_GammaCorrect(color);
            colormap[c] = V_BestColor(palette_colors, color);
        }
    }
}

dyncolormap_t *GetSpecialLights(int32_t lr, int32_t lg, int32_t lb, int32_t fr, int32_t fg, int32_t fb)
{
    argb_t         color(255, lr, lg, lb);
    argb_t         fade(255, fr, fg, fb);
    dyncolormap_t *colormap = &NormalLight;

    // Bah! Simple linear search because I want to get this done.
    while (colormap)
    {
        if (color.getr() == colormap->color.getr() && color.getg() == colormap->color.getg() &&
            color.getb() == colormap->color.getb() && fade.getr() == colormap->fade.getr() &&
            fade.getg() == colormap->fade.getg() && fade.getb() == colormap->fade.getb())
            return colormap;
        else
            colormap = colormap->next;
    }

    // Not found. Create it.
    colormap = (dyncolormap_t *)Z_Malloc(sizeof(*colormap), PU_LEVEL, 0);

    shademap_t *maps = new shademap_t();
    maps->colormap   = (palindex_t *)Z_Malloc(NUMCOLORMAPS * 256 * sizeof(palindex_t), PU_LEVEL, 0);
    maps->shademap   = (argb_t *)Z_Malloc(NUMCOLORMAPS * 256 * sizeof(argb_t), PU_LEVEL, 0);

    colormap->maps   = shaderef_t(maps, 0);
    colormap->color  = color;
    colormap->fade   = fade;
    colormap->next   = NormalLight.next;
    NormalLight.next = colormap;

    BuildColoredLights(maps, lr, lg, lb, fr, fg, fb);

    return colormap;
}

BEGIN_COMMAND(testcolor)
{
    if (argc < 2)
    {
        Printf(PRINT_HIGH, "testcolor <color>\n");
    }
    else
    {
        argb_t color(V_GetColorFromString(argv[1]));

        BuildColoredLights((shademap_t *)NormalLight.maps.map(), color.getr(), color.getg(), color.getb(),
                           level.fadeto_color[1], level.fadeto_color[2], level.fadeto_color[3]);
    }
}
END_COMMAND(testcolor)

//
// V_ResetPalette
//
// Resets the palette back to the default palette.
//
void V_ResetPalette()
{
    if (I_VideoInitialized())
    {
        fargb_t blend(0.0f, 0.0f, 0.0f, 0.0f);
        V_SetBlend(blend);
    }
}

static constexpr uint8_t x11r6rgb[] = {
    33,  32,  36,  88,  67,  111, 110, 115, 111, 114, 116, 105, 117, 109, 58,  32,  114, 103, 98,  46,  116, 120, 116,
    44,  118, 32,  49,  48,  46,  52,  49,  32,  57,  52,  47,  48,  50,  47,  50,  48,  32,  49,  56,  58,  51,  57,
    58,  51,  54,  32,  114, 119, 115, 32,  69,  120, 112, 32,  36,  13,  10,  13,  10,  50,  53,  53,  32,  50,  53,
    48,  32,  50,  53,  48,  9,   9,   115, 110, 111, 119, 13,  10,  50,  52,  56,  32,  50,  52,  56,  32,  50,  53,
    53,  9,   9,   103, 104, 111, 115, 116, 32,  119, 104, 105, 116, 101, 13,  10,  50,  52,  56,  32,  50,  52,  56,
    32,  50,  53,  53,  9,   9,   71,  104, 111, 115, 116, 87,  104, 105, 116, 101, 13,  10,  50,  52,  53,  32,  50,
    52,  53,  32,  50,  52,  53,  9,   9,   119, 104, 105, 116, 101, 32,  115, 109, 111, 107, 101, 13,  10,  50,  52,
    53,  32,  50,  52,  53,  32,  50,  52,  53,  9,   9,   87,  104, 105, 116, 101, 83,  109, 111, 107, 101, 13,  10,
    50,  50,  48,  32,  50,  50,  48,  32,  50,  50,  48,  9,   9,   103, 97,  105, 110, 115, 98,  111, 114, 111, 13,
    10,  50,  53,  53,  32,  50,  53,  48,  32,  50,  52,  48,  9,   9,   102, 108, 111, 114, 97,  108, 32,  119, 104,
    105, 116, 101, 13,  10,  50,  53,  53,  32,  50,  53,  48,  32,  50,  52,  48,  9,   9,   70,  108, 111, 114, 97,
    108, 87,  104, 105, 116, 101, 13,  10,  50,  53,  51,  32,  50,  52,  53,  32,  50,  51,  48,  9,   9,   111, 108,
    100, 32,  108, 97,  99,  101, 13,  10,  50,  53,  51,  32,  50,  52,  53,  32,  50,  51,  48,  9,   9,   79,  108,
    100, 76,  97,  99,  101, 13,  10,  50,  53,  48,  32,  50,  52,  48,  32,  50,  51,  48,  9,   9,   108, 105, 110,
    101, 110, 13,  10,  50,  53,  48,  32,  50,  51,  53,  32,  50,  49,  53,  9,   9,   97,  110, 116, 105, 113, 117,
    101, 32,  119, 104, 105, 116, 101, 13,  10,  50,  53,  48,  32,  50,  51,  53,  32,  50,  49,  53,  9,   9,   65,
    110, 116, 105, 113, 117, 101, 87,  104, 105, 116, 101, 13,  10,  50,  53,  53,  32,  50,  51,  57,  32,  50,  49,
    51,  9,   9,   112, 97,  112, 97,  121, 97,  32,  119, 104, 105, 112, 13,  10,  50,  53,  53,  32,  50,  51,  57,
    32,  50,  49,  51,  9,   9,   80,  97,  112, 97,  121, 97,  87,  104, 105, 112, 13,  10,  50,  53,  53,  32,  50,
    51,  53,  32,  50,  48,  53,  9,   9,   98,  108, 97,  110, 99,  104, 101, 100, 32,  97,  108, 109, 111, 110, 100,
    13,  10,  50,  53,  53,  32,  50,  51,  53,  32,  50,  48,  53,  9,   9,   66,  108, 97,  110, 99,  104, 101, 100,
    65,  108, 109, 111, 110, 100, 13,  10,  50,  53,  53,  32,  50,  50,  56,  32,  49,  57,  54,  9,   9,   98,  105,
    115, 113, 117, 101, 13,  10,  50,  53,  53,  32,  50,  49,  56,  32,  49,  56,  53,  9,   9,   112, 101, 97,  99,
    104, 32,  112, 117, 102, 102, 13,  10,  50,  53,  53,  32,  50,  49,  56,  32,  49,  56,  53,  9,   9,   80,  101,
    97,  99,  104, 80,  117, 102, 102, 13,  10,  50,  53,  53,  32,  50,  50,  50,  32,  49,  55,  51,  9,   9,   110,
    97,  118, 97,  106, 111, 32,  119, 104, 105, 116, 101, 13,  10,  50,  53,  53,  32,  50,  50,  50,  32,  49,  55,
    51,  9,   9,   78,  97,  118, 97,  106, 111, 87,  104, 105, 116, 101, 13,  10,  50,  53,  53,  32,  50,  50,  56,
    32,  49,  56,  49,  9,   9,   109, 111, 99,  99,  97,  115, 105, 110, 13,  10,  50,  53,  53,  32,  50,  52,  56,
    32,  50,  50,  48,  9,   9,   99,  111, 114, 110, 115, 105, 108, 107, 13,  10,  50,  53,  53,  32,  50,  53,  53,
    32,  50,  52,  48,  9,   9,   105, 118, 111, 114, 121, 13,  10,  50,  53,  53,  32,  50,  53,  48,  32,  50,  48,
    53,  9,   9,   108, 101, 109, 111, 110, 32,  99,  104, 105, 102, 102, 111, 110, 13,  10,  50,  53,  53,  32,  50,
    53,  48,  32,  50,  48,  53,  9,   9,   76,  101, 109, 111, 110, 67,  104, 105, 102, 102, 111, 110, 13,  10,  50,
    53,  53,  32,  50,  52,  53,  32,  50,  51,  56,  9,   9,   115, 101, 97,  115, 104, 101, 108, 108, 13,  10,  50,
    52,  48,  32,  50,  53,  53,  32,  50,  52,  48,  9,   9,   104, 111, 110, 101, 121, 100, 101, 119, 13,  10,  50,
    52,  53,  32,  50,  53,  53,  32,  50,  53,  48,  9,   9,   109, 105, 110, 116, 32,  99,  114, 101, 97,  109, 13,
    10,  50,  52,  53,  32,  50,  53,  53,  32,  50,  53,  48,  9,   9,   77,  105, 110, 116, 67,  114, 101, 97,  109,
    13,  10,  50,  52,  48,  32,  50,  53,  53,  32,  50,  53,  53,  9,   9,   97,  122, 117, 114, 101, 13,  10,  50,
    52,  48,  32,  50,  52,  56,  32,  50,  53,  53,  9,   9,   97,  108, 105, 99,  101, 32,  98,  108, 117, 101, 13,
    10,  50,  52,  48,  32,  50,  52,  56,  32,  50,  53,  53,  9,   9,   65,  108, 105, 99,  101, 66,  108, 117, 101,
    13,  10,  50,  51,  48,  32,  50,  51,  48,  32,  50,  53,  48,  9,   9,   108, 97,  118, 101, 110, 100, 101, 114,
    13,  10,  50,  53,  53,  32,  50,  52,  48,  32,  50,  52,  53,  9,   9,   108, 97,  118, 101, 110, 100, 101, 114,
    32,  98,  108, 117, 115, 104, 13,  10,  50,  53,  53,  32,  50,  52,  48,  32,  50,  52,  53,  9,   9,   76,  97,
    118, 101, 110, 100, 101, 114, 66,  108, 117, 115, 104, 13,  10,  50,  53,  53,  32,  50,  50,  56,  32,  50,  50,
    53,  9,   9,   109, 105, 115, 116, 121, 32,  114, 111, 115, 101, 13,  10,  50,  53,  53,  32,  50,  50,  56,  32,
    50,  50,  53,  9,   9,   77,  105, 115, 116, 121, 82,  111, 115, 101, 13,  10,  50,  53,  53,  32,  50,  53,  53,
    32,  50,  53,  53,  9,   9,   119, 104, 105, 116, 101, 13,  10,  32,  32,  48,  32,  32,  32,  48,  32,  32,  32,
    48,  9,   9,   98,  108, 97,  99,  107, 13,  10,  32,  52,  55,  32,  32,  55,  57,  32,  32,  55,  57,  9,   9,
    100, 97,  114, 107, 32,  115, 108, 97,  116, 101, 32,  103, 114, 97,  121, 13,  10,  32,  52,  55,  32,  32,  55,
    57,  32,  32,  55,  57,  9,   9,   68,  97,  114, 107, 83,  108, 97,  116, 101, 71,  114, 97,  121, 13,  10,  32,
    52,  55,  32,  32,  55,  57,  32,  32,  55,  57,  9,   9,   100, 97,  114, 107, 32,  115, 108, 97,  116, 101, 32,
    103, 114, 101, 121, 13,  10,  32,  52,  55,  32,  32,  55,  57,  32,  32,  55,  57,  9,   9,   68,  97,  114, 107,
    83,  108, 97,  116, 101, 71,  114, 101, 121, 13,  10,  49,  48,  53,  32,  49,  48,  53,  32,  49,  48,  53,  9,
    9,   100, 105, 109, 32,  103, 114, 97,  121, 13,  10,  49,  48,  53,  32,  49,  48,  53,  32,  49,  48,  53,  9,
    9,   68,  105, 109, 71,  114, 97,  121, 13,  10,  49,  48,  53,  32,  49,  48,  53,  32,  49,  48,  53,  9,   9,
    100, 105, 109, 32,  103, 114, 101, 121, 13,  10,  49,  48,  53,  32,  49,  48,  53,  32,  49,  48,  53,  9,   9,
    68,  105, 109, 71,  114, 101, 121, 13,  10,  49,  49,  50,  32,  49,  50,  56,  32,  49,  52,  52,  9,   9,   115,
    108, 97,  116, 101, 32,  103, 114, 97,  121, 13,  10,  49,  49,  50,  32,  49,  50,  56,  32,  49,  52,  52,  9,
    9,   83,  108, 97,  116, 101, 71,  114, 97,  121, 13,  10,  49,  49,  50,  32,  49,  50,  56,  32,  49,  52,  52,
    9,   9,   115, 108, 97,  116, 101, 32,  103, 114, 101, 121, 13,  10,  49,  49,  50,  32,  49,  50,  56,  32,  49,
    52,  52,  9,   9,   83,  108, 97,  116, 101, 71,  114, 101, 121, 13,  10,  49,  49,  57,  32,  49,  51,  54,  32,
    49,  53,  51,  9,   9,   108, 105, 103, 104, 116, 32,  115, 108, 97,  116, 101, 32,  103, 114, 97,  121, 13,  10,
    49,  49,  57,  32,  49,  51,  54,  32,  49,  53,  51,  9,   9,   76,  105, 103, 104, 116, 83,  108, 97,  116, 101,
    71,  114, 97,  121, 13,  10,  49,  49,  57,  32,  49,  51,  54,  32,  49,  53,  51,  9,   9,   108, 105, 103, 104,
    116, 32,  115, 108, 97,  116, 101, 32,  103, 114, 101, 121, 13,  10,  49,  49,  57,  32,  49,  51,  54,  32,  49,
    53,  51,  9,   9,   76,  105, 103, 104, 116, 83,  108, 97,  116, 101, 71,  114, 101, 121, 13,  10,  49,  57,  48,
    32,  49,  57,  48,  32,  49,  57,  48,  9,   9,   103, 114, 97,  121, 13,  10,  49,  57,  48,  32,  49,  57,  48,
    32,  49,  57,  48,  9,   9,   103, 114, 101, 121, 13,  10,  50,  49,  49,  32,  50,  49,  49,  32,  50,  49,  49,
    9,   9,   108, 105, 103, 104, 116, 32,  103, 114, 101, 121, 13,  10,  50,  49,  49,  32,  50,  49,  49,  32,  50,
    49,  49,  9,   9,   76,  105, 103, 104, 116, 71,  114, 101, 121, 13,  10,  50,  49,  49,  32,  50,  49,  49,  32,
    50,  49,  49,  9,   9,   108, 105, 103, 104, 116, 32,  103, 114, 97,  121, 13,  10,  50,  49,  49,  32,  50,  49,
    49,  32,  50,  49,  49,  9,   9,   76,  105, 103, 104, 116, 71,  114, 97,  121, 13,  10,  32,  50,  53,  32,  32,
    50,  53,  32,  49,  49,  50,  9,   9,   109, 105, 100, 110, 105, 103, 104, 116, 32,  98,  108, 117, 101, 13,  10,
    32,  50,  53,  32,  32,  50,  53,  32,  49,  49,  50,  9,   9,   77,  105, 100, 110, 105, 103, 104, 116, 66,  108,
    117, 101, 13,  10,  32,  32,  48,  32,  32,  32,  48,  32,  49,  50,  56,  9,   9,   110, 97,  118, 121, 13,  10,
    32,  32,  48,  32,  32,  32,  48,  32,  49,  50,  56,  9,   9,   110, 97,  118, 121, 32,  98,  108, 117, 101, 13,
    10,  32,  32,  48,  32,  32,  32,  48,  32,  49,  50,  56,  9,   9,   78,  97,  118, 121, 66,  108, 117, 101, 13,
    10,  49,  48,  48,  32,  49,  52,  57,  32,  50,  51,  55,  9,   9,   99,  111, 114, 110, 102, 108, 111, 119, 101,
    114, 32,  98,  108, 117, 101, 13,  10,  49,  48,  48,  32,  49,  52,  57,  32,  50,  51,  55,  9,   9,   67,  111,
    114, 110, 102, 108, 111, 119, 101, 114, 66,  108, 117, 101, 13,  10,  32,  55,  50,  32,  32,  54,  49,  32,  49,
    51,  57,  9,   9,   100, 97,  114, 107, 32,  115, 108, 97,  116, 101, 32,  98,  108, 117, 101, 13,  10,  32,  55,
    50,  32,  32,  54,  49,  32,  49,  51,  57,  9,   9,   68,  97,  114, 107, 83,  108, 97,  116, 101, 66,  108, 117,
    101, 13,  10,  49,  48,  54,  32,  32,  57,  48,  32,  50,  48,  53,  9,   9,   115, 108, 97,  116, 101, 32,  98,
    108, 117, 101, 13,  10,  49,  48,  54,  32,  32,  57,  48,  32,  50,  48,  53,  9,   9,   83,  108, 97,  116, 101,
    66,  108, 117, 101, 13,  10,  49,  50,  51,  32,  49,  48,  52,  32,  50,  51,  56,  9,   9,   109, 101, 100, 105,
    117, 109, 32,  115, 108, 97,  116, 101, 32,  98,  108, 117, 101, 13,  10,  49,  50,  51,  32,  49,  48,  52,  32,
    50,  51,  56,  9,   9,   77,  101, 100, 105, 117, 109, 83,  108, 97,  116, 101, 66,  108, 117, 101, 13,  10,  49,
    51,  50,  32,  49,  49,  50,  32,  50,  53,  53,  9,   9,   108, 105, 103, 104, 116, 32,  115, 108, 97,  116, 101,
    32,  98,  108, 117, 101, 13,  10,  49,  51,  50,  32,  49,  49,  50,  32,  50,  53,  53,  9,   9,   76,  105, 103,
    104, 116, 83,  108, 97,  116, 101, 66,  108, 117, 101, 13,  10,  32,  32,  48,  32,  32,  32,  48,  32,  50,  48,
    53,  9,   9,   109, 101, 100, 105, 117, 109, 32,  98,  108, 117, 101, 13,  10,  32,  32,  48,  32,  32,  32,  48,
    32,  50,  48,  53,  9,   9,   77,  101, 100, 105, 117, 109, 66,  108, 117, 101, 13,  10,  32,  54,  53,  32,  49,
    48,  53,  32,  50,  50,  53,  9,   9,   114, 111, 121, 97,  108, 32,  98,  108, 117, 101, 13,  10,  32,  54,  53,
    32,  49,  48,  53,  32,  50,  50,  53,  9,   9,   82,  111, 121, 97,  108, 66,  108, 117, 101, 13,  10,  32,  32,
    48,  32,  32,  32,  48,  32,  50,  53,  53,  9,   9,   98,  108, 117, 101, 13,  10,  32,  51,  48,  32,  49,  52,
    52,  32,  50,  53,  53,  9,   9,   100, 111, 100, 103, 101, 114, 32,  98,  108, 117, 101, 13,  10,  32,  51,  48,
    32,  49,  52,  52,  32,  50,  53,  53,  9,   9,   68,  111, 100, 103, 101, 114, 66,  108, 117, 101, 13,  10,  32,
    32,  48,  32,  49,  57,  49,  32,  50,  53,  53,  9,   9,   100, 101, 101, 112, 32,  115, 107, 121, 32,  98,  108,
    117, 101, 13,  10,  32,  32,  48,  32,  49,  57,  49,  32,  50,  53,  53,  9,   9,   68,  101, 101, 112, 83,  107,
    121, 66,  108, 117, 101, 13,  10,  49,  51,  53,  32,  50,  48,  54,  32,  50,  51,  53,  9,   9,   115, 107, 121,
    32,  98,  108, 117, 101, 13,  10,  49,  51,  53,  32,  50,  48,  54,  32,  50,  51,  53,  9,   9,   83,  107, 121,
    66,  108, 117, 101, 13,  10,  49,  51,  53,  32,  50,  48,  54,  32,  50,  53,  48,  9,   9,   108, 105, 103, 104,
    116, 32,  115, 107, 121, 32,  98,  108, 117, 101, 13,  10,  49,  51,  53,  32,  50,  48,  54,  32,  50,  53,  48,
    9,   9,   76,  105, 103, 104, 116, 83,  107, 121, 66,  108, 117, 101, 13,  10,  32,  55,  48,  32,  49,  51,  48,
    32,  49,  56,  48,  9,   9,   115, 116, 101, 101, 108, 32,  98,  108, 117, 101, 13,  10,  32,  55,  48,  32,  49,
    51,  48,  32,  49,  56,  48,  9,   9,   83,  116, 101, 101, 108, 66,  108, 117, 101, 13,  10,  49,  55,  54,  32,
    49,  57,  54,  32,  50,  50,  50,  9,   9,   108, 105, 103, 104, 116, 32,  115, 116, 101, 101, 108, 32,  98,  108,
    117, 101, 13,  10,  49,  55,  54,  32,  49,  57,  54,  32,  50,  50,  50,  9,   9,   76,  105, 103, 104, 116, 83,
    116, 101, 101, 108, 66,  108, 117, 101, 13,  10,  49,  55,  51,  32,  50,  49,  54,  32,  50,  51,  48,  9,   9,
    108, 105, 103, 104, 116, 32,  98,  108, 117, 101, 13,  10,  49,  55,  51,  32,  50,  49,  54,  32,  50,  51,  48,
    9,   9,   76,  105, 103, 104, 116, 66,  108, 117, 101, 13,  10,  49,  55,  54,  32,  50,  50,  52,  32,  50,  51,
    48,  9,   9,   112, 111, 119, 100, 101, 114, 32,  98,  108, 117, 101, 13,  10,  49,  55,  54,  32,  50,  50,  52,
    32,  50,  51,  48,  9,   9,   80,  111, 119, 100, 101, 114, 66,  108, 117, 101, 13,  10,  49,  55,  53,  32,  50,
    51,  56,  32,  50,  51,  56,  9,   9,   112, 97,  108, 101, 32,  116, 117, 114, 113, 117, 111, 105, 115, 101, 13,
    10,  49,  55,  53,  32,  50,  51,  56,  32,  50,  51,  56,  9,   9,   80,  97,  108, 101, 84,  117, 114, 113, 117,
    111, 105, 115, 101, 13,  10,  32,  32,  48,  32,  50,  48,  54,  32,  50,  48,  57,  9,   9,   100, 97,  114, 107,
    32,  116, 117, 114, 113, 117, 111, 105, 115, 101, 13,  10,  32,  32,  48,  32,  50,  48,  54,  32,  50,  48,  57,
    9,   9,   68,  97,  114, 107, 84,  117, 114, 113, 117, 111, 105, 115, 101, 13,  10,  32,  55,  50,  32,  50,  48,
    57,  32,  50,  48,  52,  9,   9,   109, 101, 100, 105, 117, 109, 32,  116, 117, 114, 113, 117, 111, 105, 115, 101,
    13,  10,  32,  55,  50,  32,  50,  48,  57,  32,  50,  48,  52,  9,   9,   77,  101, 100, 105, 117, 109, 84,  117,
    114, 113, 117, 111, 105, 115, 101, 13,  10,  32,  54,  52,  32,  50,  50,  52,  32,  50,  48,  56,  9,   9,   116,
    117, 114, 113, 117, 111, 105, 115, 101, 13,  10,  32,  32,  48,  32,  50,  53,  53,  32,  50,  53,  53,  9,   9,
    99,  121, 97,  110, 13,  10,  50,  50,  52,  32,  50,  53,  53,  32,  50,  53,  53,  9,   9,   108, 105, 103, 104,
    116, 32,  99,  121, 97,  110, 13,  10,  50,  50,  52,  32,  50,  53,  53,  32,  50,  53,  53,  9,   9,   76,  105,
    103, 104, 116, 67,  121, 97,  110, 13,  10,  32,  57,  53,  32,  49,  53,  56,  32,  49,  54,  48,  9,   9,   99,
    97,  100, 101, 116, 32,  98,  108, 117, 101, 13,  10,  32,  57,  53,  32,  49,  53,  56,  32,  49,  54,  48,  9,
    9,   67,  97,  100, 101, 116, 66,  108, 117, 101, 13,  10,  49,  48,  50,  32,  50,  48,  53,  32,  49,  55,  48,
    9,   9,   109, 101, 100, 105, 117, 109, 32,  97,  113, 117, 97,  109, 97,  114, 105, 110, 101, 13,  10,  49,  48,
    50,  32,  50,  48,  53,  32,  49,  55,  48,  9,   9,   77,  101, 100, 105, 117, 109, 65,  113, 117, 97,  109, 97,
    114, 105, 110, 101, 13,  10,  49,  50,  55,  32,  50,  53,  53,  32,  50,  49,  50,  9,   9,   97,  113, 117, 97,
    109, 97,  114, 105, 110, 101, 13,  10,  32,  32,  48,  32,  49,  48,  48,  32,  32,  32,  48,  9,   9,   100, 97,
    114, 107, 32,  103, 114, 101, 101, 110, 13,  10,  32,  32,  48,  32,  49,  48,  48,  32,  32,  32,  48,  9,   9,
    68,  97,  114, 107, 71,  114, 101, 101, 110, 13,  10,  32,  56,  53,  32,  49,  48,  55,  32,  32,  52,  55,  9,
    9,   100, 97,  114, 107, 32,  111, 108, 105, 118, 101, 32,  103, 114, 101, 101, 110, 13,  10,  32,  56,  53,  32,
    49,  48,  55,  32,  32,  52,  55,  9,   9,   68,  97,  114, 107, 79,  108, 105, 118, 101, 71,  114, 101, 101, 110,
    13,  10,  49,  52,  51,  32,  49,  56,  56,  32,  49,  52,  51,  9,   9,   100, 97,  114, 107, 32,  115, 101, 97,
    32,  103, 114, 101, 101, 110, 13,  10,  49,  52,  51,  32,  49,  56,  56,  32,  49,  52,  51,  9,   9,   68,  97,
    114, 107, 83,  101, 97,  71,  114, 101, 101, 110, 13,  10,  32,  52,  54,  32,  49,  51,  57,  32,  32,  56,  55,
    9,   9,   115, 101, 97,  32,  103, 114, 101, 101, 110, 13,  10,  32,  52,  54,  32,  49,  51,  57,  32,  32,  56,
    55,  9,   9,   83,  101, 97,  71,  114, 101, 101, 110, 13,  10,  32,  54,  48,  32,  49,  55,  57,  32,  49,  49,
    51,  9,   9,   109, 101, 100, 105, 117, 109, 32,  115, 101, 97,  32,  103, 114, 101, 101, 110, 13,  10,  32,  54,
    48,  32,  49,  55,  57,  32,  49,  49,  51,  9,   9,   77,  101, 100, 105, 117, 109, 83,  101, 97,  71,  114, 101,
    101, 110, 13,  10,  32,  51,  50,  32,  49,  55,  56,  32,  49,  55,  48,  9,   9,   108, 105, 103, 104, 116, 32,
    115, 101, 97,  32,  103, 114, 101, 101, 110, 13,  10,  32,  51,  50,  32,  49,  55,  56,  32,  49,  55,  48,  9,
    9,   76,  105, 103, 104, 116, 83,  101, 97,  71,  114, 101, 101, 110, 13,  10,  49,  53,  50,  32,  50,  53,  49,
    32,  49,  53,  50,  9,   9,   112, 97,  108, 101, 32,  103, 114, 101, 101, 110, 13,  10,  49,  53,  50,  32,  50,
    53,  49,  32,  49,  53,  50,  9,   9,   80,  97,  108, 101, 71,  114, 101, 101, 110, 13,  10,  32,  32,  48,  32,
    50,  53,  53,  32,  49,  50,  55,  9,   9,   115, 112, 114, 105, 110, 103, 32,  103, 114, 101, 101, 110, 13,  10,
    32,  32,  48,  32,  50,  53,  53,  32,  49,  50,  55,  9,   9,   83,  112, 114, 105, 110, 103, 71,  114, 101, 101,
    110, 13,  10,  49,  50,  52,  32,  50,  53,  50,  32,  32,  32,  48,  9,   9,   108, 97,  119, 110, 32,  103, 114,
    101, 101, 110, 13,  10,  49,  50,  52,  32,  50,  53,  50,  32,  32,  32,  48,  9,   9,   76,  97,  119, 110, 71,
    114, 101, 101, 110, 13,  10,  32,  32,  48,  32,  50,  53,  53,  32,  32,  32,  48,  9,   9,   103, 114, 101, 101,
    110, 13,  10,  49,  50,  55,  32,  50,  53,  53,  32,  32,  32,  48,  9,   9,   99,  104, 97,  114, 116, 114, 101,
    117, 115, 101, 13,  10,  32,  32,  48,  32,  50,  53,  48,  32,  49,  53,  52,  9,   9,   109, 101, 100, 105, 117,
    109, 32,  115, 112, 114, 105, 110, 103, 32,  103, 114, 101, 101, 110, 13,  10,  32,  32,  48,  32,  50,  53,  48,
    32,  49,  53,  52,  9,   9,   77,  101, 100, 105, 117, 109, 83,  112, 114, 105, 110, 103, 71,  114, 101, 101, 110,
    13,  10,  49,  55,  51,  32,  50,  53,  53,  32,  32,  52,  55,  9,   9,   103, 114, 101, 101, 110, 32,  121, 101,
    108, 108, 111, 119, 13,  10,  49,  55,  51,  32,  50,  53,  53,  32,  32,  52,  55,  9,   9,   71,  114, 101, 101,
    110, 89,  101, 108, 108, 111, 119, 13,  10,  32,  53,  48,  32,  50,  48,  53,  32,  32,  53,  48,  9,   9,   108,
    105, 109, 101, 32,  103, 114, 101, 101, 110, 13,  10,  32,  53,  48,  32,  50,  48,  53,  32,  32,  53,  48,  9,
    9,   76,  105, 109, 101, 71,  114, 101, 101, 110, 13,  10,  49,  53,  52,  32,  50,  48,  53,  32,  32,  53,  48,
    9,   9,   121, 101, 108, 108, 111, 119, 32,  103, 114, 101, 101, 110, 13,  10,  49,  53,  52,  32,  50,  48,  53,
    32,  32,  53,  48,  9,   9,   89,  101, 108, 108, 111, 119, 71,  114, 101, 101, 110, 13,  10,  32,  51,  52,  32,
    49,  51,  57,  32,  32,  51,  52,  9,   9,   102, 111, 114, 101, 115, 116, 32,  103, 114, 101, 101, 110, 13,  10,
    32,  51,  52,  32,  49,  51,  57,  32,  32,  51,  52,  9,   9,   70,  111, 114, 101, 115, 116, 71,  114, 101, 101,
    110, 13,  10,  49,  48,  55,  32,  49,  52,  50,  32,  32,  51,  53,  9,   9,   111, 108, 105, 118, 101, 32,  100,
    114, 97,  98,  13,  10,  49,  48,  55,  32,  49,  52,  50,  32,  32,  51,  53,  9,   9,   79,  108, 105, 118, 101,
    68,  114, 97,  98,  13,  10,  49,  56,  57,  32,  49,  56,  51,  32,  49,  48,  55,  9,   9,   100, 97,  114, 107,
    32,  107, 104, 97,  107, 105, 13,  10,  49,  56,  57,  32,  49,  56,  51,  32,  49,  48,  55,  9,   9,   68,  97,
    114, 107, 75,  104, 97,  107, 105, 13,  10,  50,  52,  48,  32,  50,  51,  48,  32,  49,  52,  48,  9,   9,   107,
    104, 97,  107, 105, 13,  10,  50,  51,  56,  32,  50,  51,  50,  32,  49,  55,  48,  9,   9,   112, 97,  108, 101,
    32,  103, 111, 108, 100, 101, 110, 114, 111, 100, 13,  10,  50,  51,  56,  32,  50,  51,  50,  32,  49,  55,  48,
    9,   9,   80,  97,  108, 101, 71,  111, 108, 100, 101, 110, 114, 111, 100, 13,  10,  50,  53,  48,  32,  50,  53,
    48,  32,  50,  49,  48,  9,   9,   108, 105, 103, 104, 116, 32,  103, 111, 108, 100, 101, 110, 114, 111, 100, 32,
    121, 101, 108, 108, 111, 119, 13,  10,  50,  53,  48,  32,  50,  53,  48,  32,  50,  49,  48,  9,   9,   76,  105,
    103, 104, 116, 71,  111, 108, 100, 101, 110, 114, 111, 100, 89,  101, 108, 108, 111, 119, 13,  10,  50,  53,  53,
    32,  50,  53,  53,  32,  50,  50,  52,  9,   9,   108, 105, 103, 104, 116, 32,  121, 101, 108, 108, 111, 119, 13,
    10,  50,  53,  53,  32,  50,  53,  53,  32,  50,  50,  52,  9,   9,   76,  105, 103, 104, 116, 89,  101, 108, 108,
    111, 119, 13,  10,  50,  53,  53,  32,  50,  53,  53,  32,  32,  32,  48,  9,   9,   121, 101, 108, 108, 111, 119,
    13,  10,  50,  53,  53,  32,  50,  49,  53,  32,  32,  32,  48,  32,  9,   9,   103, 111, 108, 100, 13,  10,  50,
    51,  56,  32,  50,  50,  49,  32,  49,  51,  48,  9,   9,   108, 105, 103, 104, 116, 32,  103, 111, 108, 100, 101,
    110, 114, 111, 100, 13,  10,  50,  51,  56,  32,  50,  50,  49,  32,  49,  51,  48,  9,   9,   76,  105, 103, 104,
    116, 71,  111, 108, 100, 101, 110, 114, 111, 100, 13,  10,  50,  49,  56,  32,  49,  54,  53,  32,  32,  51,  50,
    9,   9,   103, 111, 108, 100, 101, 110, 114, 111, 100, 13,  10,  49,  56,  52,  32,  49,  51,  52,  32,  32,  49,
    49,  9,   9,   100, 97,  114, 107, 32,  103, 111, 108, 100, 101, 110, 114, 111, 100, 13,  10,  49,  56,  52,  32,
    49,  51,  52,  32,  32,  49,  49,  9,   9,   68,  97,  114, 107, 71,  111, 108, 100, 101, 110, 114, 111, 100, 13,
    10,  49,  56,  56,  32,  49,  52,  51,  32,  49,  52,  51,  9,   9,   114, 111, 115, 121, 32,  98,  114, 111, 119,
    110, 13,  10,  49,  56,  56,  32,  49,  52,  51,  32,  49,  52,  51,  9,   9,   82,  111, 115, 121, 66,  114, 111,
    119, 110, 13,  10,  50,  48,  53,  32,  32,  57,  50,  32,  32,  57,  50,  9,   9,   105, 110, 100, 105, 97,  110,
    32,  114, 101, 100, 13,  10,  50,  48,  53,  32,  32,  57,  50,  32,  32,  57,  50,  9,   9,   73,  110, 100, 105,
    97,  110, 82,  101, 100, 13,  10,  49,  51,  57,  32,  32,  54,  57,  32,  32,  49,  57,  9,   9,   115, 97,  100,
    100, 108, 101, 32,  98,  114, 111, 119, 110, 13,  10,  49,  51,  57,  32,  32,  54,  57,  32,  32,  49,  57,  9,
    9,   83,  97,  100, 100, 108, 101, 66,  114, 111, 119, 110, 13,  10,  49,  54,  48,  32,  32,  56,  50,  32,  32,
    52,  53,  9,   9,   115, 105, 101, 110, 110, 97,  13,  10,  50,  48,  53,  32,  49,  51,  51,  32,  32,  54,  51,
    9,   9,   112, 101, 114, 117, 13,  10,  50,  50,  50,  32,  49,  56,  52,  32,  49,  51,  53,  9,   9,   98,  117,
    114, 108, 121, 119, 111, 111, 100, 13,  10,  50,  52,  53,  32,  50,  52,  53,  32,  50,  50,  48,  9,   9,   98,
    101, 105, 103, 101, 13,  10,  50,  52,  53,  32,  50,  50,  50,  32,  49,  55,  57,  9,   9,   119, 104, 101, 97,
    116, 13,  10,  50,  52,  52,  32,  49,  54,  52,  32,  32,  57,  54,  9,   9,   115, 97,  110, 100, 121, 32,  98,
    114, 111, 119, 110, 13,  10,  50,  52,  52,  32,  49,  54,  52,  32,  32,  57,  54,  9,   9,   83,  97,  110, 100,
    121, 66,  114, 111, 119, 110, 13,  10,  50,  49,  48,  32,  49,  56,  48,  32,  49,  52,  48,  9,   9,   116, 97,
    110, 13,  10,  50,  49,  48,  32,  49,  48,  53,  32,  32,  51,  48,  9,   9,   99,  104, 111, 99,  111, 108, 97,
    116, 101, 13,  10,  49,  55,  56,  32,  32,  51,  52,  32,  32,  51,  52,  9,   9,   102, 105, 114, 101, 98,  114,
    105, 99,  107, 13,  10,  49,  54,  53,  32,  32,  52,  50,  32,  32,  52,  50,  9,   9,   98,  114, 111, 119, 110,
    13,  10,  50,  51,  51,  32,  49,  53,  48,  32,  49,  50,  50,  9,   9,   100, 97,  114, 107, 32,  115, 97,  108,
    109, 111, 110, 13,  10,  50,  51,  51,  32,  49,  53,  48,  32,  49,  50,  50,  9,   9,   68,  97,  114, 107, 83,
    97,  108, 109, 111, 110, 13,  10,  50,  53,  48,  32,  49,  50,  56,  32,  49,  49,  52,  9,   9,   115, 97,  108,
    109, 111, 110, 13,  10,  50,  53,  53,  32,  49,  54,  48,  32,  49,  50,  50,  9,   9,   108, 105, 103, 104, 116,
    32,  115, 97,  108, 109, 111, 110, 13,  10,  50,  53,  53,  32,  49,  54,  48,  32,  49,  50,  50,  9,   9,   76,
    105, 103, 104, 116, 83,  97,  108, 109, 111, 110, 13,  10,  50,  53,  53,  32,  49,  54,  53,  32,  32,  32,  48,
    9,   9,   111, 114, 97,  110, 103, 101, 13,  10,  50,  53,  53,  32,  49,  52,  48,  32,  32,  32,  48,  9,   9,
    100, 97,  114, 107, 32,  111, 114, 97,  110, 103, 101, 13,  10,  50,  53,  53,  32,  49,  52,  48,  32,  32,  32,
    48,  9,   9,   68,  97,  114, 107, 79,  114, 97,  110, 103, 101, 13,  10,  50,  53,  53,  32,  49,  50,  55,  32,
    32,  56,  48,  9,   9,   99,  111, 114, 97,  108, 13,  10,  50,  52,  48,  32,  49,  50,  56,  32,  49,  50,  56,
    9,   9,   108, 105, 103, 104, 116, 32,  99,  111, 114, 97,  108, 13,  10,  50,  52,  48,  32,  49,  50,  56,  32,
    49,  50,  56,  9,   9,   76,  105, 103, 104, 116, 67,  111, 114, 97,  108, 13,  10,  50,  53,  53,  32,  32,  57,
    57,  32,  32,  55,  49,  9,   9,   116, 111, 109, 97,  116, 111, 13,  10,  50,  53,  53,  32,  32,  54,  57,  32,
    32,  32,  48,  9,   9,   111, 114, 97,  110, 103, 101, 32,  114, 101, 100, 13,  10,  50,  53,  53,  32,  32,  54,
    57,  32,  32,  32,  48,  9,   9,   79,  114, 97,  110, 103, 101, 82,  101, 100, 13,  10,  50,  53,  53,  32,  32,
    32,  48,  32,  32,  32,  48,  9,   9,   114, 101, 100, 13,  10,  50,  53,  53,  32,  49,  48,  53,  32,  49,  56,
    48,  9,   9,   104, 111, 116, 32,  112, 105, 110, 107, 13,  10,  50,  53,  53,  32,  49,  48,  53,  32,  49,  56,
    48,  9,   9,   72,  111, 116, 80,  105, 110, 107, 13,  10,  50,  53,  53,  32,  32,  50,  48,  32,  49,  52,  55,
    9,   9,   100, 101, 101, 112, 32,  112, 105, 110, 107, 13,  10,  50,  53,  53,  32,  32,  50,  48,  32,  49,  52,
    55,  9,   9,   68,  101, 101, 112, 80,  105, 110, 107, 13,  10,  50,  53,  53,  32,  49,  57,  50,  32,  50,  48,
    51,  9,   9,   112, 105, 110, 107, 13,  10,  50,  53,  53,  32,  49,  56,  50,  32,  49,  57,  51,  9,   9,   108,
    105, 103, 104, 116, 32,  112, 105, 110, 107, 13,  10,  50,  53,  53,  32,  49,  56,  50,  32,  49,  57,  51,  9,
    9,   76,  105, 103, 104, 116, 80,  105, 110, 107, 13,  10,  50,  49,  57,  32,  49,  49,  50,  32,  49,  52,  55,
    9,   9,   112, 97,  108, 101, 32,  118, 105, 111, 108, 101, 116, 32,  114, 101, 100, 13,  10,  50,  49,  57,  32,
    49,  49,  50,  32,  49,  52,  55,  9,   9,   80,  97,  108, 101, 86,  105, 111, 108, 101, 116, 82,  101, 100, 13,
    10,  49,  55,  54,  32,  32,  52,  56,  32,  32,  57,  54,  9,   9,   109, 97,  114, 111, 111, 110, 13,  10,  49,
    57,  57,  32,  32,  50,  49,  32,  49,  51,  51,  9,   9,   109, 101, 100, 105, 117, 109, 32,  118, 105, 111, 108,
    101, 116, 32,  114, 101, 100, 13,  10,  49,  57,  57,  32,  32,  50,  49,  32,  49,  51,  51,  9,   9,   77,  101,
    100, 105, 117, 109, 86,  105, 111, 108, 101, 116, 82,  101, 100, 13,  10,  50,  48,  56,  32,  32,  51,  50,  32,
    49,  52,  52,  9,   9,   118, 105, 111, 108, 101, 116, 32,  114, 101, 100, 13,  10,  50,  48,  56,  32,  32,  51,
    50,  32,  49,  52,  52,  9,   9,   86,  105, 111, 108, 101, 116, 82,  101, 100, 13,  10,  50,  53,  53,  32,  32,
    32,  48,  32,  50,  53,  53,  9,   9,   109, 97,  103, 101, 110, 116, 97,  13,  10,  50,  51,  56,  32,  49,  51,
    48,  32,  50,  51,  56,  9,   9,   118, 105, 111, 108, 101, 116, 13,  10,  50,  50,  49,  32,  49,  54,  48,  32,
    50,  50,  49,  9,   9,   112, 108, 117, 109, 13,  10,  50,  49,  56,  32,  49,  49,  50,  32,  50,  49,  52,  9,
    9,   111, 114, 99,  104, 105, 100, 13,  10,  49,  56,  54,  32,  32,  56,  53,  32,  50,  49,  49,  9,   9,   109,
    101, 100, 105, 117, 109, 32,  111, 114, 99,  104, 105, 100, 13,  10,  49,  56,  54,  32,  32,  56,  53,  32,  50,
    49,  49,  9,   9,   77,  101, 100, 105, 117, 109, 79,  114, 99,  104, 105, 100, 13,  10,  49,  53,  51,  32,  32,
    53,  48,  32,  50,  48,  52,  9,   9,   100, 97,  114, 107, 32,  111, 114, 99,  104, 105, 100, 13,  10,  49,  53,
    51,  32,  32,  53,  48,  32,  50,  48,  52,  9,   9,   68,  97,  114, 107, 79,  114, 99,  104, 105, 100, 13,  10,
    49,  52,  56,  32,  32,  32,  48,  32,  50,  49,  49,  9,   9,   100, 97,  114, 107, 32,  118, 105, 111, 108, 101,
    116, 13,  10,  49,  52,  56,  32,  32,  32,  48,  32,  50,  49,  49,  9,   9,   68,  97,  114, 107, 86,  105, 111,
    108, 101, 116, 13,  10,  49,  51,  56,  32,  32,  52,  51,  32,  50,  50,  54,  9,   9,   98,  108, 117, 101, 32,
    118, 105, 111, 108, 101, 116, 13,  10,  49,  51,  56,  32,  32,  52,  51,  32,  50,  50,  54,  9,   9,   66,  108,
    117, 101, 86,  105, 111, 108, 101, 116, 13,  10,  49,  54,  48,  32,  32,  51,  50,  32,  50,  52,  48,  9,   9,
    112, 117, 114, 112, 108, 101, 13,  10,  49,  52,  55,  32,  49,  49,  50,  32,  50,  49,  57,  9,   9,   109, 101,
    100, 105, 117, 109, 32,  112, 117, 114, 112, 108, 101, 13,  10,  49,  52,  55,  32,  49,  49,  50,  32,  50,  49,
    57,  9,   9,   77,  101, 100, 105, 117, 109, 80,  117, 114, 112, 108, 101, 13,  10,  50,  49,  54,  32,  49,  57,
    49,  32,  50,  49,  54,  9,   9,   116, 104, 105, 115, 116, 108, 101, 13,  10,  50,  53,  53,  32,  50,  53,  48,
    32,  50,  53,  48,  9,   9,   115, 110, 111, 119, 49,  13,  10,  50,  51,  56,  32,  50,  51,  51,  32,  50,  51,
    51,  9,   9,   115, 110, 111, 119, 50,  13,  10,  50,  48,  53,  32,  50,  48,  49,  32,  50,  48,  49,  9,   9,
    115, 110, 111, 119, 51,  13,  10,  49,  51,  57,  32,  49,  51,  55,  32,  49,  51,  55,  9,   9,   115, 110, 111,
    119, 52,  13,  10,  50,  53,  53,  32,  50,  52,  53,  32,  50,  51,  56,  9,   9,   115, 101, 97,  115, 104, 101,
    108, 108, 49,  13,  10,  50,  51,  56,  32,  50,  50,  57,  32,  50,  50,  50,  9,   9,   115, 101, 97,  115, 104,
    101, 108, 108, 50,  13,  10,  50,  48,  53,  32,  49,  57,  55,  32,  49,  57,  49,  9,   9,   115, 101, 97,  115,
    104, 101, 108, 108, 51,  13,  10,  49,  51,  57,  32,  49,  51,  52,  32,  49,  51,  48,  9,   9,   115, 101, 97,
    115, 104, 101, 108, 108, 52,  13,  10,  50,  53,  53,  32,  50,  51,  57,  32,  50,  49,  57,  9,   9,   65,  110,
    116, 105, 113, 117, 101, 87,  104, 105, 116, 101, 49,  13,  10,  50,  51,  56,  32,  50,  50,  51,  32,  50,  48,
    52,  9,   9,   65,  110, 116, 105, 113, 117, 101, 87,  104, 105, 116, 101, 50,  13,  10,  50,  48,  53,  32,  49,
    57,  50,  32,  49,  55,  54,  9,   9,   65,  110, 116, 105, 113, 117, 101, 87,  104, 105, 116, 101, 51,  13,  10,
    49,  51,  57,  32,  49,  51,  49,  32,  49,  50,  48,  9,   9,   65,  110, 116, 105, 113, 117, 101, 87,  104, 105,
    116, 101, 52,  13,  10,  50,  53,  53,  32,  50,  50,  56,  32,  49,  57,  54,  9,   9,   98,  105, 115, 113, 117,
    101, 49,  13,  10,  50,  51,  56,  32,  50,  49,  51,  32,  49,  56,  51,  9,   9,   98,  105, 115, 113, 117, 101,
    50,  13,  10,  50,  48,  53,  32,  49,  56,  51,  32,  49,  53,  56,  9,   9,   98,  105, 115, 113, 117, 101, 51,
    13,  10,  49,  51,  57,  32,  49,  50,  53,  32,  49,  48,  55,  9,   9,   98,  105, 115, 113, 117, 101, 52,  13,
    10,  50,  53,  53,  32,  50,  49,  56,  32,  49,  56,  53,  9,   9,   80,  101, 97,  99,  104, 80,  117, 102, 102,
    49,  13,  10,  50,  51,  56,  32,  50,  48,  51,  32,  49,  55,  51,  9,   9,   80,  101, 97,  99,  104, 80,  117,
    102, 102, 50,  13,  10,  50,  48,  53,  32,  49,  55,  53,  32,  49,  52,  57,  9,   9,   80,  101, 97,  99,  104,
    80,  117, 102, 102, 51,  13,  10,  49,  51,  57,  32,  49,  49,  57,  32,  49,  48,  49,  9,   9,   80,  101, 97,
    99,  104, 80,  117, 102, 102, 52,  13,  10,  50,  53,  53,  32,  50,  50,  50,  32,  49,  55,  51,  9,   9,   78,
    97,  118, 97,  106, 111, 87,  104, 105, 116, 101, 49,  13,  10,  50,  51,  56,  32,  50,  48,  55,  32,  49,  54,
    49,  9,   9,   78,  97,  118, 97,  106, 111, 87,  104, 105, 116, 101, 50,  13,  10,  50,  48,  53,  32,  49,  55,
    57,  32,  49,  51,  57,  9,   9,   78,  97,  118, 97,  106, 111, 87,  104, 105, 116, 101, 51,  13,  10,  49,  51,
    57,  32,  49,  50,  49,  9,   32,  57,  52,  9,   9,   78,  97,  118, 97,  106, 111, 87,  104, 105, 116, 101, 52,
    13,  10,  50,  53,  53,  32,  50,  53,  48,  32,  50,  48,  53,  9,   9,   76,  101, 109, 111, 110, 67,  104, 105,
    102, 102, 111, 110, 49,  13,  10,  50,  51,  56,  32,  50,  51,  51,  32,  49,  57,  49,  9,   9,   76,  101, 109,
    111, 110, 67,  104, 105, 102, 102, 111, 110, 50,  13,  10,  50,  48,  53,  32,  50,  48,  49,  32,  49,  54,  53,
    9,   9,   76,  101, 109, 111, 110, 67,  104, 105, 102, 102, 111, 110, 51,  13,  10,  49,  51,  57,  32,  49,  51,
    55,  32,  49,  49,  50,  9,   9,   76,  101, 109, 111, 110, 67,  104, 105, 102, 102, 111, 110, 52,  13,  10,  50,
    53,  53,  32,  50,  52,  56,  32,  50,  50,  48,  9,   9,   99,  111, 114, 110, 115, 105, 108, 107, 49,  13,  10,
    50,  51,  56,  32,  50,  51,  50,  32,  50,  48,  53,  9,   9,   99,  111, 114, 110, 115, 105, 108, 107, 50,  13,
    10,  50,  48,  53,  32,  50,  48,  48,  32,  49,  55,  55,  9,   9,   99,  111, 114, 110, 115, 105, 108, 107, 51,
    13,  10,  49,  51,  57,  32,  49,  51,  54,  32,  49,  50,  48,  9,   9,   99,  111, 114, 110, 115, 105, 108, 107,
    52,  13,  10,  50,  53,  53,  32,  50,  53,  53,  32,  50,  52,  48,  9,   9,   105, 118, 111, 114, 121, 49,  13,
    10,  50,  51,  56,  32,  50,  51,  56,  32,  50,  50,  52,  9,   9,   105, 118, 111, 114, 121, 50,  13,  10,  50,
    48,  53,  32,  50,  48,  53,  32,  49,  57,  51,  9,   9,   105, 118, 111, 114, 121, 51,  13,  10,  49,  51,  57,
    32,  49,  51,  57,  32,  49,  51,  49,  9,   9,   105, 118, 111, 114, 121, 52,  13,  10,  50,  52,  48,  32,  50,
    53,  53,  32,  50,  52,  48,  9,   9,   104, 111, 110, 101, 121, 100, 101, 119, 49,  13,  10,  50,  50,  52,  32,
    50,  51,  56,  32,  50,  50,  52,  9,   9,   104, 111, 110, 101, 121, 100, 101, 119, 50,  13,  10,  49,  57,  51,
    32,  50,  48,  53,  32,  49,  57,  51,  9,   9,   104, 111, 110, 101, 121, 100, 101, 119, 51,  13,  10,  49,  51,
    49,  32,  49,  51,  57,  32,  49,  51,  49,  9,   9,   104, 111, 110, 101, 121, 100, 101, 119, 52,  13,  10,  50,
    53,  53,  32,  50,  52,  48,  32,  50,  52,  53,  9,   9,   76,  97,  118, 101, 110, 100, 101, 114, 66,  108, 117,
    115, 104, 49,  13,  10,  50,  51,  56,  32,  50,  50,  52,  32,  50,  50,  57,  9,   9,   76,  97,  118, 101, 110,
    100, 101, 114, 66,  108, 117, 115, 104, 50,  13,  10,  50,  48,  53,  32,  49,  57,  51,  32,  49,  57,  55,  9,
    9,   76,  97,  118, 101, 110, 100, 101, 114, 66,  108, 117, 115, 104, 51,  13,  10,  49,  51,  57,  32,  49,  51,
    49,  32,  49,  51,  52,  9,   9,   76,  97,  118, 101, 110, 100, 101, 114, 66,  108, 117, 115, 104, 52,  13,  10,
    50,  53,  53,  32,  50,  50,  56,  32,  50,  50,  53,  9,   9,   77,  105, 115, 116, 121, 82,  111, 115, 101, 49,
    13,  10,  50,  51,  56,  32,  50,  49,  51,  32,  50,  49,  48,  9,   9,   77,  105, 115, 116, 121, 82,  111, 115,
    101, 50,  13,  10,  50,  48,  53,  32,  49,  56,  51,  32,  49,  56,  49,  9,   9,   77,  105, 115, 116, 121, 82,
    111, 115, 101, 51,  13,  10,  49,  51,  57,  32,  49,  50,  53,  32,  49,  50,  51,  9,   9,   77,  105, 115, 116,
    121, 82,  111, 115, 101, 52,  13,  10,  50,  52,  48,  32,  50,  53,  53,  32,  50,  53,  53,  9,   9,   97,  122,
    117, 114, 101, 49,  13,  10,  50,  50,  52,  32,  50,  51,  56,  32,  50,  51,  56,  9,   9,   97,  122, 117, 114,
    101, 50,  13,  10,  49,  57,  51,  32,  50,  48,  53,  32,  50,  48,  53,  9,   9,   97,  122, 117, 114, 101, 51,
    13,  10,  49,  51,  49,  32,  49,  51,  57,  32,  49,  51,  57,  9,   9,   97,  122, 117, 114, 101, 52,  13,  10,
    49,  51,  49,  32,  49,  49,  49,  32,  50,  53,  53,  9,   9,   83,  108, 97,  116, 101, 66,  108, 117, 101, 49,
    13,  10,  49,  50,  50,  32,  49,  48,  51,  32,  50,  51,  56,  9,   9,   83,  108, 97,  116, 101, 66,  108, 117,
    101, 50,  13,  10,  49,  48,  53,  32,  32,  56,  57,  32,  50,  48,  53,  9,   9,   83,  108, 97,  116, 101, 66,
    108, 117, 101, 51,  13,  10,  32,  55,  49,  32,  32,  54,  48,  32,  49,  51,  57,  9,   9,   83,  108, 97,  116,
    101, 66,  108, 117, 101, 52,  13,  10,  32,  55,  50,  32,  49,  49,  56,  32,  50,  53,  53,  9,   9,   82,  111,
    121, 97,  108, 66,  108, 117, 101, 49,  13,  10,  32,  54,  55,  32,  49,  49,  48,  32,  50,  51,  56,  9,   9,
    82,  111, 121, 97,  108, 66,  108, 117, 101, 50,  13,  10,  32,  53,  56,  32,  32,  57,  53,  32,  50,  48,  53,
    9,   9,   82,  111, 121, 97,  108, 66,  108, 117, 101, 51,  13,  10,  32,  51,  57,  32,  32,  54,  52,  32,  49,
    51,  57,  9,   9,   82,  111, 121, 97,  108, 66,  108, 117, 101, 52,  13,  10,  32,  32,  48,  32,  32,  32,  48,
    32,  50,  53,  53,  9,   9,   98,  108, 117, 101, 49,  13,  10,  32,  32,  48,  32,  32,  32,  48,  32,  50,  51,
    56,  9,   9,   98,  108, 117, 101, 50,  13,  10,  32,  32,  48,  32,  32,  32,  48,  32,  50,  48,  53,  9,   9,
    98,  108, 117, 101, 51,  13,  10,  32,  32,  48,  32,  32,  32,  48,  32,  49,  51,  57,  9,   9,   98,  108, 117,
    101, 52,  13,  10,  32,  51,  48,  32,  49,  52,  52,  32,  50,  53,  53,  9,   9,   68,  111, 100, 103, 101, 114,
    66,  108, 117, 101, 49,  13,  10,  32,  50,  56,  32,  49,  51,  52,  32,  50,  51,  56,  9,   9,   68,  111, 100,
    103, 101, 114, 66,  108, 117, 101, 50,  13,  10,  32,  50,  52,  32,  49,  49,  54,  32,  50,  48,  53,  9,   9,
    68,  111, 100, 103, 101, 114, 66,  108, 117, 101, 51,  13,  10,  32,  49,  54,  32,  32,  55,  56,  32,  49,  51,
    57,  9,   9,   68,  111, 100, 103, 101, 114, 66,  108, 117, 101, 52,  13,  10,  32,  57,  57,  32,  49,  56,  52,
    32,  50,  53,  53,  9,   9,   83,  116, 101, 101, 108, 66,  108, 117, 101, 49,  13,  10,  32,  57,  50,  32,  49,
    55,  50,  32,  50,  51,  56,  9,   9,   83,  116, 101, 101, 108, 66,  108, 117, 101, 50,  13,  10,  32,  55,  57,
    32,  49,  52,  56,  32,  50,  48,  53,  9,   9,   83,  116, 101, 101, 108, 66,  108, 117, 101, 51,  13,  10,  32,
    53,  52,  32,  49,  48,  48,  32,  49,  51,  57,  9,   9,   83,  116, 101, 101, 108, 66,  108, 117, 101, 52,  13,
    10,  32,  32,  48,  32,  49,  57,  49,  32,  50,  53,  53,  9,   9,   68,  101, 101, 112, 83,  107, 121, 66,  108,
    117, 101, 49,  13,  10,  32,  32,  48,  32,  49,  55,  56,  32,  50,  51,  56,  9,   9,   68,  101, 101, 112, 83,
    107, 121, 66,  108, 117, 101, 50,  13,  10,  32,  32,  48,  32,  49,  53,  52,  32,  50,  48,  53,  9,   9,   68,
    101, 101, 112, 83,  107, 121, 66,  108, 117, 101, 51,  13,  10,  32,  32,  48,  32,  49,  48,  52,  32,  49,  51,
    57,  9,   9,   68,  101, 101, 112, 83,  107, 121, 66,  108, 117, 101, 52,  13,  10,  49,  51,  53,  32,  50,  48,
    54,  32,  50,  53,  53,  9,   9,   83,  107, 121, 66,  108, 117, 101, 49,  13,  10,  49,  50,  54,  32,  49,  57,
    50,  32,  50,  51,  56,  9,   9,   83,  107, 121, 66,  108, 117, 101, 50,  13,  10,  49,  48,  56,  32,  49,  54,
    54,  32,  50,  48,  53,  9,   9,   83,  107, 121, 66,  108, 117, 101, 51,  13,  10,  32,  55,  52,  32,  49,  49,
    50,  32,  49,  51,  57,  9,   9,   83,  107, 121, 66,  108, 117, 101, 52,  13,  10,  49,  55,  54,  32,  50,  50,
    54,  32,  50,  53,  53,  9,   9,   76,  105, 103, 104, 116, 83,  107, 121, 66,  108, 117, 101, 49,  13,  10,  49,
    54,  52,  32,  50,  49,  49,  32,  50,  51,  56,  9,   9,   76,  105, 103, 104, 116, 83,  107, 121, 66,  108, 117,
    101, 50,  13,  10,  49,  52,  49,  32,  49,  56,  50,  32,  50,  48,  53,  9,   9,   76,  105, 103, 104, 116, 83,
    107, 121, 66,  108, 117, 101, 51,  13,  10,  32,  57,  54,  32,  49,  50,  51,  32,  49,  51,  57,  9,   9,   76,
    105, 103, 104, 116, 83,  107, 121, 66,  108, 117, 101, 52,  13,  10,  49,  57,  56,  32,  50,  50,  54,  32,  50,
    53,  53,  9,   9,   83,  108, 97,  116, 101, 71,  114, 97,  121, 49,  13,  10,  49,  56,  53,  32,  50,  49,  49,
    32,  50,  51,  56,  9,   9,   83,  108, 97,  116, 101, 71,  114, 97,  121, 50,  13,  10,  49,  53,  57,  32,  49,
    56,  50,  32,  50,  48,  53,  9,   9,   83,  108, 97,  116, 101, 71,  114, 97,  121, 51,  13,  10,  49,  48,  56,
    32,  49,  50,  51,  32,  49,  51,  57,  9,   9,   83,  108, 97,  116, 101, 71,  114, 97,  121, 52,  13,  10,  50,
    48,  50,  32,  50,  50,  53,  32,  50,  53,  53,  9,   9,   76,  105, 103, 104, 116, 83,  116, 101, 101, 108, 66,
    108, 117, 101, 49,  13,  10,  49,  56,  56,  32,  50,  49,  48,  32,  50,  51,  56,  9,   9,   76,  105, 103, 104,
    116, 83,  116, 101, 101, 108, 66,  108, 117, 101, 50,  13,  10,  49,  54,  50,  32,  49,  56,  49,  32,  50,  48,
    53,  9,   9,   76,  105, 103, 104, 116, 83,  116, 101, 101, 108, 66,  108, 117, 101, 51,  13,  10,  49,  49,  48,
    32,  49,  50,  51,  32,  49,  51,  57,  9,   9,   76,  105, 103, 104, 116, 83,  116, 101, 101, 108, 66,  108, 117,
    101, 52,  13,  10,  49,  57,  49,  32,  50,  51,  57,  32,  50,  53,  53,  9,   9,   76,  105, 103, 104, 116, 66,
    108, 117, 101, 49,  13,  10,  49,  55,  56,  32,  50,  50,  51,  32,  50,  51,  56,  9,   9,   76,  105, 103, 104,
    116, 66,  108, 117, 101, 50,  13,  10,  49,  53,  52,  32,  49,  57,  50,  32,  50,  48,  53,  9,   9,   76,  105,
    103, 104, 116, 66,  108, 117, 101, 51,  13,  10,  49,  48,  52,  32,  49,  51,  49,  32,  49,  51,  57,  9,   9,
    76,  105, 103, 104, 116, 66,  108, 117, 101, 52,  13,  10,  50,  50,  52,  32,  50,  53,  53,  32,  50,  53,  53,
    9,   9,   76,  105, 103, 104, 116, 67,  121, 97,  110, 49,  13,  10,  50,  48,  57,  32,  50,  51,  56,  32,  50,
    51,  56,  9,   9,   76,  105, 103, 104, 116, 67,  121, 97,  110, 50,  13,  10,  49,  56,  48,  32,  50,  48,  53,
    32,  50,  48,  53,  9,   9,   76,  105, 103, 104, 116, 67,  121, 97,  110, 51,  13,  10,  49,  50,  50,  32,  49,
    51,  57,  32,  49,  51,  57,  9,   9,   76,  105, 103, 104, 116, 67,  121, 97,  110, 52,  13,  10,  49,  56,  55,
    32,  50,  53,  53,  32,  50,  53,  53,  9,   9,   80,  97,  108, 101, 84,  117, 114, 113, 117, 111, 105, 115, 101,
    49,  13,  10,  49,  55,  52,  32,  50,  51,  56,  32,  50,  51,  56,  9,   9,   80,  97,  108, 101, 84,  117, 114,
    113, 117, 111, 105, 115, 101, 50,  13,  10,  49,  53,  48,  32,  50,  48,  53,  32,  50,  48,  53,  9,   9,   80,
    97,  108, 101, 84,  117, 114, 113, 117, 111, 105, 115, 101, 51,  13,  10,  49,  48,  50,  32,  49,  51,  57,  32,
    49,  51,  57,  9,   9,   80,  97,  108, 101, 84,  117, 114, 113, 117, 111, 105, 115, 101, 52,  13,  10,  49,  53,
    50,  32,  50,  52,  53,  32,  50,  53,  53,  9,   9,   67,  97,  100, 101, 116, 66,  108, 117, 101, 49,  13,  10,
    49,  52,  50,  32,  50,  50,  57,  32,  50,  51,  56,  9,   9,   67,  97,  100, 101, 116, 66,  108, 117, 101, 50,
    13,  10,  49,  50,  50,  32,  49,  57,  55,  32,  50,  48,  53,  9,   9,   67,  97,  100, 101, 116, 66,  108, 117,
    101, 51,  13,  10,  32,  56,  51,  32,  49,  51,  52,  32,  49,  51,  57,  9,   9,   67,  97,  100, 101, 116, 66,
    108, 117, 101, 52,  13,  10,  32,  32,  48,  32,  50,  52,  53,  32,  50,  53,  53,  9,   9,   116, 117, 114, 113,
    117, 111, 105, 115, 101, 49,  13,  10,  32,  32,  48,  32,  50,  50,  57,  32,  50,  51,  56,  9,   9,   116, 117,
    114, 113, 117, 111, 105, 115, 101, 50,  13,  10,  32,  32,  48,  32,  49,  57,  55,  32,  50,  48,  53,  9,   9,
    116, 117, 114, 113, 117, 111, 105, 115, 101, 51,  13,  10,  32,  32,  48,  32,  49,  51,  52,  32,  49,  51,  57,
    9,   9,   116, 117, 114, 113, 117, 111, 105, 115, 101, 52,  13,  10,  32,  32,  48,  32,  50,  53,  53,  32,  50,
    53,  53,  9,   9,   99,  121, 97,  110, 49,  13,  10,  32,  32,  48,  32,  50,  51,  56,  32,  50,  51,  56,  9,
    9,   99,  121, 97,  110, 50,  13,  10,  32,  32,  48,  32,  50,  48,  53,  32,  50,  48,  53,  9,   9,   99,  121,
    97,  110, 51,  13,  10,  32,  32,  48,  32,  49,  51,  57,  32,  49,  51,  57,  9,   9,   99,  121, 97,  110, 52,
    13,  10,  49,  53,  49,  32,  50,  53,  53,  32,  50,  53,  53,  9,   9,   68,  97,  114, 107, 83,  108, 97,  116,
    101, 71,  114, 97,  121, 49,  13,  10,  49,  52,  49,  32,  50,  51,  56,  32,  50,  51,  56,  9,   9,   68,  97,
    114, 107, 83,  108, 97,  116, 101, 71,  114, 97,  121, 50,  13,  10,  49,  50,  49,  32,  50,  48,  53,  32,  50,
    48,  53,  9,   9,   68,  97,  114, 107, 83,  108, 97,  116, 101, 71,  114, 97,  121, 51,  13,  10,  32,  56,  50,
    32,  49,  51,  57,  32,  49,  51,  57,  9,   9,   68,  97,  114, 107, 83,  108, 97,  116, 101, 71,  114, 97,  121,
    52,  13,  10,  49,  50,  55,  32,  50,  53,  53,  32,  50,  49,  50,  9,   9,   97,  113, 117, 97,  109, 97,  114,
    105, 110, 101, 49,  13,  10,  49,  49,  56,  32,  50,  51,  56,  32,  49,  57,  56,  9,   9,   97,  113, 117, 97,
    109, 97,  114, 105, 110, 101, 50,  13,  10,  49,  48,  50,  32,  50,  48,  53,  32,  49,  55,  48,  9,   9,   97,
    113, 117, 97,  109, 97,  114, 105, 110, 101, 51,  13,  10,  32,  54,  57,  32,  49,  51,  57,  32,  49,  49,  54,
    9,   9,   97,  113, 117, 97,  109, 97,  114, 105, 110, 101, 52,  13,  10,  49,  57,  51,  32,  50,  53,  53,  32,
    49,  57,  51,  9,   9,   68,  97,  114, 107, 83,  101, 97,  71,  114, 101, 101, 110, 49,  13,  10,  49,  56,  48,
    32,  50,  51,  56,  32,  49,  56,  48,  9,   9,   68,  97,  114, 107, 83,  101, 97,  71,  114, 101, 101, 110, 50,
    13,  10,  49,  53,  53,  32,  50,  48,  53,  32,  49,  53,  53,  9,   9,   68,  97,  114, 107, 83,  101, 97,  71,
    114, 101, 101, 110, 51,  13,  10,  49,  48,  53,  32,  49,  51,  57,  32,  49,  48,  53,  9,   9,   68,  97,  114,
    107, 83,  101, 97,  71,  114, 101, 101, 110, 52,  13,  10,  32,  56,  52,  32,  50,  53,  53,  32,  49,  53,  57,
    9,   9,   83,  101, 97,  71,  114, 101, 101, 110, 49,  13,  10,  32,  55,  56,  32,  50,  51,  56,  32,  49,  52,
    56,  9,   9,   83,  101, 97,  71,  114, 101, 101, 110, 50,  13,  10,  32,  54,  55,  32,  50,  48,  53,  32,  49,
    50,  56,  9,   9,   83,  101, 97,  71,  114, 101, 101, 110, 51,  13,  10,  32,  52,  54,  32,  49,  51,  57,  9,
    32,  56,  55,  9,   9,   83,  101, 97,  71,  114, 101, 101, 110, 52,  13,  10,  49,  53,  52,  32,  50,  53,  53,
    32,  49,  53,  52,  9,   9,   80,  97,  108, 101, 71,  114, 101, 101, 110, 49,  13,  10,  49,  52,  52,  32,  50,
    51,  56,  32,  49,  52,  52,  9,   9,   80,  97,  108, 101, 71,  114, 101, 101, 110, 50,  13,  10,  49,  50,  52,
    32,  50,  48,  53,  32,  49,  50,  52,  9,   9,   80,  97,  108, 101, 71,  114, 101, 101, 110, 51,  13,  10,  32,
    56,  52,  32,  49,  51,  57,  9,   32,  56,  52,  9,   9,   80,  97,  108, 101, 71,  114, 101, 101, 110, 52,  13,
    10,  32,  32,  48,  32,  50,  53,  53,  32,  49,  50,  55,  9,   9,   83,  112, 114, 105, 110, 103, 71,  114, 101,
    101, 110, 49,  13,  10,  32,  32,  48,  32,  50,  51,  56,  32,  49,  49,  56,  9,   9,   83,  112, 114, 105, 110,
    103, 71,  114, 101, 101, 110, 50,  13,  10,  32,  32,  48,  32,  50,  48,  53,  32,  49,  48,  50,  9,   9,   83,
    112, 114, 105, 110, 103, 71,  114, 101, 101, 110, 51,  13,  10,  32,  32,  48,  32,  49,  51,  57,  9,   32,  54,
    57,  9,   9,   83,  112, 114, 105, 110, 103, 71,  114, 101, 101, 110, 52,  13,  10,  32,  32,  48,  32,  50,  53,
    53,  9,   32,  32,  48,  9,   9,   103, 114, 101, 101, 110, 49,  13,  10,  32,  32,  48,  32,  50,  51,  56,  9,
    32,  32,  48,  9,   9,   103, 114, 101, 101, 110, 50,  13,  10,  32,  32,  48,  32,  50,  48,  53,  9,   32,  32,
    48,  9,   9,   103, 114, 101, 101, 110, 51,  13,  10,  32,  32,  48,  32,  49,  51,  57,  9,   32,  32,  48,  9,
    9,   103, 114, 101, 101, 110, 52,  13,  10,  49,  50,  55,  32,  50,  53,  53,  9,   32,  32,  48,  9,   9,   99,
    104, 97,  114, 116, 114, 101, 117, 115, 101, 49,  13,  10,  49,  49,  56,  32,  50,  51,  56,  9,   32,  32,  48,
    9,   9,   99,  104, 97,  114, 116, 114, 101, 117, 115, 101, 50,  13,  10,  49,  48,  50,  32,  50,  48,  53,  9,
    32,  32,  48,  9,   9,   99,  104, 97,  114, 116, 114, 101, 117, 115, 101, 51,  13,  10,  32,  54,  57,  32,  49,
    51,  57,  9,   32,  32,  48,  9,   9,   99,  104, 97,  114, 116, 114, 101, 117, 115, 101, 52,  13,  10,  49,  57,
    50,  32,  50,  53,  53,  9,   32,  54,  50,  9,   9,   79,  108, 105, 118, 101, 68,  114, 97,  98,  49,  13,  10,
    49,  55,  57,  32,  50,  51,  56,  9,   32,  53,  56,  9,   9,   79,  108, 105, 118, 101, 68,  114, 97,  98,  50,
    13,  10,  49,  53,  52,  32,  50,  48,  53,  9,   32,  53,  48,  9,   9,   79,  108, 105, 118, 101, 68,  114, 97,
    98,  51,  13,  10,  49,  48,  53,  32,  49,  51,  57,  9,   32,  51,  52,  9,   9,   79,  108, 105, 118, 101, 68,
    114, 97,  98,  52,  13,  10,  50,  48,  50,  32,  50,  53,  53,  32,  49,  49,  50,  9,   9,   68,  97,  114, 107,
    79,  108, 105, 118, 101, 71,  114, 101, 101, 110, 49,  13,  10,  49,  56,  56,  32,  50,  51,  56,  32,  49,  48,
    52,  9,   9,   68,  97,  114, 107, 79,  108, 105, 118, 101, 71,  114, 101, 101, 110, 50,  13,  10,  49,  54,  50,
    32,  50,  48,  53,  9,   32,  57,  48,  9,   9,   68,  97,  114, 107, 79,  108, 105, 118, 101, 71,  114, 101, 101,
    110, 51,  13,  10,  49,  49,  48,  32,  49,  51,  57,  9,   32,  54,  49,  9,   9,   68,  97,  114, 107, 79,  108,
    105, 118, 101, 71,  114, 101, 101, 110, 52,  13,  10,  50,  53,  53,  32,  50,  52,  54,  32,  49,  52,  51,  9,
    9,   107, 104, 97,  107, 105, 49,  13,  10,  50,  51,  56,  32,  50,  51,  48,  32,  49,  51,  51,  9,   9,   107,
    104, 97,  107, 105, 50,  13,  10,  50,  48,  53,  32,  49,  57,  56,  32,  49,  49,  53,  9,   9,   107, 104, 97,
    107, 105, 51,  13,  10,  49,  51,  57,  32,  49,  51,  52,  9,   32,  55,  56,  9,   9,   107, 104, 97,  107, 105,
    52,  13,  10,  50,  53,  53,  32,  50,  51,  54,  32,  49,  51,  57,  9,   9,   76,  105, 103, 104, 116, 71,  111,
    108, 100, 101, 110, 114, 111, 100, 49,  13,  10,  50,  51,  56,  32,  50,  50,  48,  32,  49,  51,  48,  9,   9,
    76,  105, 103, 104, 116, 71,  111, 108, 100, 101, 110, 114, 111, 100, 50,  13,  10,  50,  48,  53,  32,  49,  57,
    48,  32,  49,  49,  50,  9,   9,   76,  105, 103, 104, 116, 71,  111, 108, 100, 101, 110, 114, 111, 100, 51,  13,
    10,  49,  51,  57,  32,  49,  50,  57,  9,   32,  55,  54,  9,   9,   76,  105, 103, 104, 116, 71,  111, 108, 100,
    101, 110, 114, 111, 100, 52,  13,  10,  50,  53,  53,  32,  50,  53,  53,  32,  50,  50,  52,  9,   9,   76,  105,
    103, 104, 116, 89,  101, 108, 108, 111, 119, 49,  13,  10,  50,  51,  56,  32,  50,  51,  56,  32,  50,  48,  57,
    9,   9,   76,  105, 103, 104, 116, 89,  101, 108, 108, 111, 119, 50,  13,  10,  50,  48,  53,  32,  50,  48,  53,
    32,  49,  56,  48,  9,   9,   76,  105, 103, 104, 116, 89,  101, 108, 108, 111, 119, 51,  13,  10,  49,  51,  57,
    32,  49,  51,  57,  32,  49,  50,  50,  9,   9,   76,  105, 103, 104, 116, 89,  101, 108, 108, 111, 119, 52,  13,
    10,  50,  53,  53,  32,  50,  53,  53,  9,   32,  32,  48,  9,   9,   121, 101, 108, 108, 111, 119, 49,  13,  10,
    50,  51,  56,  32,  50,  51,  56,  9,   32,  32,  48,  9,   9,   121, 101, 108, 108, 111, 119, 50,  13,  10,  50,
    48,  53,  32,  50,  48,  53,  9,   32,  32,  48,  9,   9,   121, 101, 108, 108, 111, 119, 51,  13,  10,  49,  51,
    57,  32,  49,  51,  57,  9,   32,  32,  48,  9,   9,   121, 101, 108, 108, 111, 119, 52,  13,  10,  50,  53,  53,
    32,  50,  49,  53,  9,   32,  32,  48,  9,   9,   103, 111, 108, 100, 49,  13,  10,  50,  51,  56,  32,  50,  48,
    49,  9,   32,  32,  48,  9,   9,   103, 111, 108, 100, 50,  13,  10,  50,  48,  53,  32,  49,  55,  51,  9,   32,
    32,  48,  9,   9,   103, 111, 108, 100, 51,  13,  10,  49,  51,  57,  32,  49,  49,  55,  9,   32,  32,  48,  9,
    9,   103, 111, 108, 100, 52,  13,  10,  50,  53,  53,  32,  49,  57,  51,  9,   32,  51,  55,  9,   9,   103, 111,
    108, 100, 101, 110, 114, 111, 100, 49,  13,  10,  50,  51,  56,  32,  49,  56,  48,  9,   32,  51,  52,  9,   9,
    103, 111, 108, 100, 101, 110, 114, 111, 100, 50,  13,  10,  50,  48,  53,  32,  49,  53,  53,  9,   32,  50,  57,
    9,   9,   103, 111, 108, 100, 101, 110, 114, 111, 100, 51,  13,  10,  49,  51,  57,  32,  49,  48,  53,  9,   32,
    50,  48,  9,   9,   103, 111, 108, 100, 101, 110, 114, 111, 100, 52,  13,  10,  50,  53,  53,  32,  49,  56,  53,
    9,   32,  49,  53,  9,   9,   68,  97,  114, 107, 71,  111, 108, 100, 101, 110, 114, 111, 100, 49,  13,  10,  50,
    51,  56,  32,  49,  55,  51,  9,   32,  49,  52,  9,   9,   68,  97,  114, 107, 71,  111, 108, 100, 101, 110, 114,
    111, 100, 50,  13,  10,  50,  48,  53,  32,  49,  52,  57,  9,   32,  49,  50,  9,   9,   68,  97,  114, 107, 71,
    111, 108, 100, 101, 110, 114, 111, 100, 51,  13,  10,  49,  51,  57,  32,  49,  48,  49,  9,   32,  32,  56,  9,
    9,   68,  97,  114, 107, 71,  111, 108, 100, 101, 110, 114, 111, 100, 52,  13,  10,  50,  53,  53,  32,  49,  57,
    51,  32,  49,  57,  51,  9,   9,   82,  111, 115, 121, 66,  114, 111, 119, 110, 49,  13,  10,  50,  51,  56,  32,
    49,  56,  48,  32,  49,  56,  48,  9,   9,   82,  111, 115, 121, 66,  114, 111, 119, 110, 50,  13,  10,  50,  48,
    53,  32,  49,  53,  53,  32,  49,  53,  53,  9,   9,   82,  111, 115, 121, 66,  114, 111, 119, 110, 51,  13,  10,
    49,  51,  57,  32,  49,  48,  53,  32,  49,  48,  53,  9,   9,   82,  111, 115, 121, 66,  114, 111, 119, 110, 52,
    13,  10,  50,  53,  53,  32,  49,  48,  54,  32,  49,  48,  54,  9,   9,   73,  110, 100, 105, 97,  110, 82,  101,
    100, 49,  13,  10,  50,  51,  56,  32,  32,  57,  57,  9,   32,  57,  57,  9,   9,   73,  110, 100, 105, 97,  110,
    82,  101, 100, 50,  13,  10,  50,  48,  53,  32,  32,  56,  53,  9,   32,  56,  53,  9,   9,   73,  110, 100, 105,
    97,  110, 82,  101, 100, 51,  13,  10,  49,  51,  57,  32,  32,  53,  56,  9,   32,  53,  56,  9,   9,   73,  110,
    100, 105, 97,  110, 82,  101, 100, 52,  13,  10,  50,  53,  53,  32,  49,  51,  48,  9,   32,  55,  49,  9,   9,
    115, 105, 101, 110, 110, 97,  49,  13,  10,  50,  51,  56,  32,  49,  50,  49,  9,   32,  54,  54,  9,   9,   115,
    105, 101, 110, 110, 97,  50,  13,  10,  50,  48,  53,  32,  49,  48,  52,  9,   32,  53,  55,  9,   9,   115, 105,
    101, 110, 110, 97,  51,  13,  10,  49,  51,  57,  32,  32,  55,  49,  9,   32,  51,  56,  9,   9,   115, 105, 101,
    110, 110, 97,  52,  13,  10,  50,  53,  53,  32,  50,  49,  49,  32,  49,  53,  53,  9,   9,   98,  117, 114, 108,
    121, 119, 111, 111, 100, 49,  13,  10,  50,  51,  56,  32,  49,  57,  55,  32,  49,  52,  53,  9,   9,   98,  117,
    114, 108, 121, 119, 111, 111, 100, 50,  13,  10,  50,  48,  53,  32,  49,  55,  48,  32,  49,  50,  53,  9,   9,
    98,  117, 114, 108, 121, 119, 111, 111, 100, 51,  13,  10,  49,  51,  57,  32,  49,  49,  53,  9,   32,  56,  53,
    9,   9,   98,  117, 114, 108, 121, 119, 111, 111, 100, 52,  13,  10,  50,  53,  53,  32,  50,  51,  49,  32,  49,
    56,  54,  9,   9,   119, 104, 101, 97,  116, 49,  13,  10,  50,  51,  56,  32,  50,  49,  54,  32,  49,  55,  52,
    9,   9,   119, 104, 101, 97,  116, 50,  13,  10,  50,  48,  53,  32,  49,  56,  54,  32,  49,  53,  48,  9,   9,
    119, 104, 101, 97,  116, 51,  13,  10,  49,  51,  57,  32,  49,  50,  54,  32,  49,  48,  50,  9,   9,   119, 104,
    101, 97,  116, 52,  13,  10,  50,  53,  53,  32,  49,  54,  53,  9,   32,  55,  57,  9,   9,   116, 97,  110, 49,
    13,  10,  50,  51,  56,  32,  49,  53,  52,  9,   32,  55,  51,  9,   9,   116, 97,  110, 50,  13,  10,  50,  48,
    53,  32,  49,  51,  51,  9,   32,  54,  51,  9,   9,   116, 97,  110, 51,  13,  10,  49,  51,  57,  32,  32,  57,
    48,  9,   32,  52,  51,  9,   9,   116, 97,  110, 52,  13,  10,  50,  53,  53,  32,  49,  50,  55,  9,   32,  51,
    54,  9,   9,   99,  104, 111, 99,  111, 108, 97,  116, 101, 49,  13,  10,  50,  51,  56,  32,  49,  49,  56,  9,
    32,  51,  51,  9,   9,   99,  104, 111, 99,  111, 108, 97,  116, 101, 50,  13,  10,  50,  48,  53,  32,  49,  48,
    50,  9,   32,  50,  57,  9,   9,   99,  104, 111, 99,  111, 108, 97,  116, 101, 51,  13,  10,  49,  51,  57,  32,
    32,  54,  57,  9,   32,  49,  57,  9,   9,   99,  104, 111, 99,  111, 108, 97,  116, 101, 52,  13,  10,  50,  53,
    53,  32,  32,  52,  56,  9,   32,  52,  56,  9,   9,   102, 105, 114, 101, 98,  114, 105, 99,  107, 49,  13,  10,
    50,  51,  56,  32,  32,  52,  52,  9,   32,  52,  52,  9,   9,   102, 105, 114, 101, 98,  114, 105, 99,  107, 50,
    13,  10,  50,  48,  53,  32,  32,  51,  56,  9,   32,  51,  56,  9,   9,   102, 105, 114, 101, 98,  114, 105, 99,
    107, 51,  13,  10,  49,  51,  57,  32,  32,  50,  54,  9,   32,  50,  54,  9,   9,   102, 105, 114, 101, 98,  114,
    105, 99,  107, 52,  13,  10,  50,  53,  53,  32,  32,  54,  52,  9,   32,  54,  52,  9,   9,   98,  114, 111, 119,
    110, 49,  13,  10,  50,  51,  56,  32,  32,  53,  57,  9,   32,  53,  57,  9,   9,   98,  114, 111, 119, 110, 50,
    13,  10,  50,  48,  53,  32,  32,  53,  49,  9,   32,  53,  49,  9,   9,   98,  114, 111, 119, 110, 51,  13,  10,
    49,  51,  57,  32,  32,  51,  53,  9,   32,  51,  53,  9,   9,   98,  114, 111, 119, 110, 52,  13,  10,  50,  53,
    53,  32,  49,  52,  48,  32,  49,  48,  53,  9,   9,   115, 97,  108, 109, 111, 110, 49,  13,  10,  50,  51,  56,
    32,  49,  51,  48,  9,   32,  57,  56,  9,   9,   115, 97,  108, 109, 111, 110, 50,  13,  10,  50,  48,  53,  32,
    49,  49,  50,  9,   32,  56,  52,  9,   9,   115, 97,  108, 109, 111, 110, 51,  13,  10,  49,  51,  57,  32,  32,
    55,  54,  9,   32,  53,  55,  9,   9,   115, 97,  108, 109, 111, 110, 52,  13,  10,  50,  53,  53,  32,  49,  54,
    48,  32,  49,  50,  50,  9,   9,   76,  105, 103, 104, 116, 83,  97,  108, 109, 111, 110, 49,  13,  10,  50,  51,
    56,  32,  49,  52,  57,  32,  49,  49,  52,  9,   9,   76,  105, 103, 104, 116, 83,  97,  108, 109, 111, 110, 50,
    13,  10,  50,  48,  53,  32,  49,  50,  57,  9,   32,  57,  56,  9,   9,   76,  105, 103, 104, 116, 83,  97,  108,
    109, 111, 110, 51,  13,  10,  49,  51,  57,  32,  32,  56,  55,  9,   32,  54,  54,  9,   9,   76,  105, 103, 104,
    116, 83,  97,  108, 109, 111, 110, 52,  13,  10,  50,  53,  53,  32,  49,  54,  53,  9,   32,  32,  48,  9,   9,
    111, 114, 97,  110, 103, 101, 49,  13,  10,  50,  51,  56,  32,  49,  53,  52,  9,   32,  32,  48,  9,   9,   111,
    114, 97,  110, 103, 101, 50,  13,  10,  50,  48,  53,  32,  49,  51,  51,  9,   32,  32,  48,  9,   9,   111, 114,
    97,  110, 103, 101, 51,  13,  10,  49,  51,  57,  32,  32,  57,  48,  9,   32,  32,  48,  9,   9,   111, 114, 97,
    110, 103, 101, 52,  13,  10,  50,  53,  53,  32,  49,  50,  55,  9,   32,  32,  48,  9,   9,   68,  97,  114, 107,
    79,  114, 97,  110, 103, 101, 49,  13,  10,  50,  51,  56,  32,  49,  49,  56,  9,   32,  32,  48,  9,   9,   68,
    97,  114, 107, 79,  114, 97,  110, 103, 101, 50,  13,  10,  50,  48,  53,  32,  49,  48,  50,  9,   32,  32,  48,
    9,   9,   68,  97,  114, 107, 79,  114, 97,  110, 103, 101, 51,  13,  10,  49,  51,  57,  32,  32,  54,  57,  9,
    32,  32,  48,  9,   9,   68,  97,  114, 107, 79,  114, 97,  110, 103, 101, 52,  13,  10,  50,  53,  53,  32,  49,
    49,  52,  9,   32,  56,  54,  9,   9,   99,  111, 114, 97,  108, 49,  13,  10,  50,  51,  56,  32,  49,  48,  54,
    9,   32,  56,  48,  9,   9,   99,  111, 114, 97,  108, 50,  13,  10,  50,  48,  53,  32,  32,  57,  49,  9,   32,
    54,  57,  9,   9,   99,  111, 114, 97,  108, 51,  13,  10,  49,  51,  57,  32,  32,  54,  50,  9,   32,  52,  55,
    9,   9,   99,  111, 114, 97,  108, 52,  13,  10,  50,  53,  53,  32,  32,  57,  57,  9,   32,  55,  49,  9,   9,
    116, 111, 109, 97,  116, 111, 49,  13,  10,  50,  51,  56,  32,  32,  57,  50,  9,   32,  54,  54,  9,   9,   116,
    111, 109, 97,  116, 111, 50,  13,  10,  50,  48,  53,  32,  32,  55,  57,  9,   32,  53,  55,  9,   9,   116, 111,
    109, 97,  116, 111, 51,  13,  10,  49,  51,  57,  32,  32,  53,  52,  9,   32,  51,  56,  9,   9,   116, 111, 109,
    97,  116, 111, 52,  13,  10,  50,  53,  53,  32,  32,  54,  57,  9,   32,  32,  48,  9,   9,   79,  114, 97,  110,
    103, 101, 82,  101, 100, 49,  13,  10,  50,  51,  56,  32,  32,  54,  52,  9,   32,  32,  48,  9,   9,   79,  114,
    97,  110, 103, 101, 82,  101, 100, 50,  13,  10,  50,  48,  53,  32,  32,  53,  53,  9,   32,  32,  48,  9,   9,
    79,  114, 97,  110, 103, 101, 82,  101, 100, 51,  13,  10,  49,  51,  57,  32,  32,  51,  55,  9,   32,  32,  48,
    9,   9,   79,  114, 97,  110, 103, 101, 82,  101, 100, 52,  13,  10,  50,  53,  53,  32,  32,  32,  48,  9,   32,
    32,  48,  9,   9,   114, 101, 100, 49,  13,  10,  50,  51,  56,  32,  32,  32,  48,  9,   32,  32,  48,  9,   9,
    114, 101, 100, 50,  13,  10,  50,  48,  53,  32,  32,  32,  48,  9,   32,  32,  48,  9,   9,   114, 101, 100, 51,
    13,  10,  49,  51,  57,  32,  32,  32,  48,  9,   32,  32,  48,  9,   9,   114, 101, 100, 52,  13,  10,  50,  53,
    53,  32,  32,  50,  48,  32,  49,  52,  55,  9,   9,   68,  101, 101, 112, 80,  105, 110, 107, 49,  13,  10,  50,
    51,  56,  32,  32,  49,  56,  32,  49,  51,  55,  9,   9,   68,  101, 101, 112, 80,  105, 110, 107, 50,  13,  10,
    50,  48,  53,  32,  32,  49,  54,  32,  49,  49,  56,  9,   9,   68,  101, 101, 112, 80,  105, 110, 107, 51,  13,
    10,  49,  51,  57,  32,  32,  49,  48,  9,   32,  56,  48,  9,   9,   68,  101, 101, 112, 80,  105, 110, 107, 52,
    13,  10,  50,  53,  53,  32,  49,  49,  48,  32,  49,  56,  48,  9,   9,   72,  111, 116, 80,  105, 110, 107, 49,
    13,  10,  50,  51,  56,  32,  49,  48,  54,  32,  49,  54,  55,  9,   9,   72,  111, 116, 80,  105, 110, 107, 50,
    13,  10,  50,  48,  53,  32,  32,  57,  54,  32,  49,  52,  52,  9,   9,   72,  111, 116, 80,  105, 110, 107, 51,
    13,  10,  49,  51,  57,  32,  32,  53,  56,  32,  32,  57,  56,  9,   9,   72,  111, 116, 80,  105, 110, 107, 52,
    13,  10,  50,  53,  53,  32,  49,  56,  49,  32,  49,  57,  55,  9,   9,   112, 105, 110, 107, 49,  13,  10,  50,
    51,  56,  32,  49,  54,  57,  32,  49,  56,  52,  9,   9,   112, 105, 110, 107, 50,  13,  10,  50,  48,  53,  32,
    49,  52,  53,  32,  49,  53,  56,  9,   9,   112, 105, 110, 107, 51,  13,  10,  49,  51,  57,  32,  32,  57,  57,
    32,  49,  48,  56,  9,   9,   112, 105, 110, 107, 52,  13,  10,  50,  53,  53,  32,  49,  55,  52,  32,  49,  56,
    53,  9,   9,   76,  105, 103, 104, 116, 80,  105, 110, 107, 49,  13,  10,  50,  51,  56,  32,  49,  54,  50,  32,
    49,  55,  51,  9,   9,   76,  105, 103, 104, 116, 80,  105, 110, 107, 50,  13,  10,  50,  48,  53,  32,  49,  52,
    48,  32,  49,  52,  57,  9,   9,   76,  105, 103, 104, 116, 80,  105, 110, 107, 51,  13,  10,  49,  51,  57,  32,
    32,  57,  53,  32,  49,  48,  49,  9,   9,   76,  105, 103, 104, 116, 80,  105, 110, 107, 52,  13,  10,  50,  53,
    53,  32,  49,  51,  48,  32,  49,  55,  49,  9,   9,   80,  97,  108, 101, 86,  105, 111, 108, 101, 116, 82,  101,
    100, 49,  13,  10,  50,  51,  56,  32,  49,  50,  49,  32,  49,  53,  57,  9,   9,   80,  97,  108, 101, 86,  105,
    111, 108, 101, 116, 82,  101, 100, 50,  13,  10,  50,  48,  53,  32,  49,  48,  52,  32,  49,  51,  55,  9,   9,
    80,  97,  108, 101, 86,  105, 111, 108, 101, 116, 82,  101, 100, 51,  13,  10,  49,  51,  57,  32,  32,  55,  49,
    9,   32,  57,  51,  9,   9,   80,  97,  108, 101, 86,  105, 111, 108, 101, 116, 82,  101, 100, 52,  13,  10,  50,
    53,  53,  32,  32,  53,  50,  32,  49,  55,  57,  9,   9,   109, 97,  114, 111, 111, 110, 49,  13,  10,  50,  51,
    56,  32,  32,  52,  56,  32,  49,  54,  55,  9,   9,   109, 97,  114, 111, 111, 110, 50,  13,  10,  50,  48,  53,
    32,  32,  52,  49,  32,  49,  52,  52,  9,   9,   109, 97,  114, 111, 111, 110, 51,  13,  10,  49,  51,  57,  32,
    32,  50,  56,  9,   32,  57,  56,  9,   9,   109, 97,  114, 111, 111, 110, 52,  13,  10,  50,  53,  53,  32,  32,
    54,  50,  32,  49,  53,  48,  9,   9,   86,  105, 111, 108, 101, 116, 82,  101, 100, 49,  13,  10,  50,  51,  56,
    32,  32,  53,  56,  32,  49,  52,  48,  9,   9,   86,  105, 111, 108, 101, 116, 82,  101, 100, 50,  13,  10,  50,
    48,  53,  32,  32,  53,  48,  32,  49,  50,  48,  9,   9,   86,  105, 111, 108, 101, 116, 82,  101, 100, 51,  13,
    10,  49,  51,  57,  32,  32,  51,  52,  9,   32,  56,  50,  9,   9,   86,  105, 111, 108, 101, 116, 82,  101, 100,
    52,  13,  10,  50,  53,  53,  32,  32,  32,  48,  32,  50,  53,  53,  9,   9,   109, 97,  103, 101, 110, 116, 97,
    49,  13,  10,  50,  51,  56,  32,  32,  32,  48,  32,  50,  51,  56,  9,   9,   109, 97,  103, 101, 110, 116, 97,
    50,  13,  10,  50,  48,  53,  32,  32,  32,  48,  32,  50,  48,  53,  9,   9,   109, 97,  103, 101, 110, 116, 97,
    51,  13,  10,  49,  51,  57,  32,  32,  32,  48,  32,  49,  51,  57,  9,   9,   109, 97,  103, 101, 110, 116, 97,
    52,  13,  10,  50,  53,  53,  32,  49,  51,  49,  32,  50,  53,  48,  9,   9,   111, 114, 99,  104, 105, 100, 49,
    13,  10,  50,  51,  56,  32,  49,  50,  50,  32,  50,  51,  51,  9,   9,   111, 114, 99,  104, 105, 100, 50,  13,
    10,  50,  48,  53,  32,  49,  48,  53,  32,  50,  48,  49,  9,   9,   111, 114, 99,  104, 105, 100, 51,  13,  10,
    49,  51,  57,  32,  32,  55,  49,  32,  49,  51,  55,  9,   9,   111, 114, 99,  104, 105, 100, 52,  13,  10,  50,
    53,  53,  32,  49,  56,  55,  32,  50,  53,  53,  9,   9,   112, 108, 117, 109, 49,  13,  10,  50,  51,  56,  32,
    49,  55,  52,  32,  50,  51,  56,  9,   9,   112, 108, 117, 109, 50,  13,  10,  50,  48,  53,  32,  49,  53,  48,
    32,  50,  48,  53,  9,   9,   112, 108, 117, 109, 51,  13,  10,  49,  51,  57,  32,  49,  48,  50,  32,  49,  51,
    57,  9,   9,   112, 108, 117, 109, 52,  13,  10,  50,  50,  52,  32,  49,  48,  50,  32,  50,  53,  53,  9,   9,
    77,  101, 100, 105, 117, 109, 79,  114, 99,  104, 105, 100, 49,  13,  10,  50,  48,  57,  32,  32,  57,  53,  32,
    50,  51,  56,  9,   9,   77,  101, 100, 105, 117, 109, 79,  114, 99,  104, 105, 100, 50,  13,  10,  49,  56,  48,
    32,  32,  56,  50,  32,  50,  48,  53,  9,   9,   77,  101, 100, 105, 117, 109, 79,  114, 99,  104, 105, 100, 51,
    13,  10,  49,  50,  50,  32,  32,  53,  53,  32,  49,  51,  57,  9,   9,   77,  101, 100, 105, 117, 109, 79,  114,
    99,  104, 105, 100, 52,  13,  10,  49,  57,  49,  32,  32,  54,  50,  32,  50,  53,  53,  9,   9,   68,  97,  114,
    107, 79,  114, 99,  104, 105, 100, 49,  13,  10,  49,  55,  56,  32,  32,  53,  56,  32,  50,  51,  56,  9,   9,
    68,  97,  114, 107, 79,  114, 99,  104, 105, 100, 50,  13,  10,  49,  53,  52,  32,  32,  53,  48,  32,  50,  48,
    53,  9,   9,   68,  97,  114, 107, 79,  114, 99,  104, 105, 100, 51,  13,  10,  49,  48,  52,  32,  32,  51,  52,
    32,  49,  51,  57,  9,   9,   68,  97,  114, 107, 79,  114, 99,  104, 105, 100, 52,  13,  10,  49,  53,  53,  32,
    32,  52,  56,  32,  50,  53,  53,  9,   9,   112, 117, 114, 112, 108, 101, 49,  13,  10,  49,  52,  53,  32,  32,
    52,  52,  32,  50,  51,  56,  9,   9,   112, 117, 114, 112, 108, 101, 50,  13,  10,  49,  50,  53,  32,  32,  51,
    56,  32,  50,  48,  53,  9,   9,   112, 117, 114, 112, 108, 101, 51,  13,  10,  32,  56,  53,  32,  32,  50,  54,
    32,  49,  51,  57,  9,   9,   112, 117, 114, 112, 108, 101, 52,  13,  10,  49,  55,  49,  32,  49,  51,  48,  32,
    50,  53,  53,  9,   9,   77,  101, 100, 105, 117, 109, 80,  117, 114, 112, 108, 101, 49,  13,  10,  49,  53,  57,
    32,  49,  50,  49,  32,  50,  51,  56,  9,   9,   77,  101, 100, 105, 117, 109, 80,  117, 114, 112, 108, 101, 50,
    13,  10,  49,  51,  55,  32,  49,  48,  52,  32,  50,  48,  53,  9,   9,   77,  101, 100, 105, 117, 109, 80,  117,
    114, 112, 108, 101, 51,  13,  10,  32,  57,  51,  32,  32,  55,  49,  32,  49,  51,  57,  9,   9,   77,  101, 100,
    105, 117, 109, 80,  117, 114, 112, 108, 101, 52,  13,  10,  50,  53,  53,  32,  50,  50,  53,  32,  50,  53,  53,
    9,   9,   116, 104, 105, 115, 116, 108, 101, 49,  13,  10,  50,  51,  56,  32,  50,  49,  48,  32,  50,  51,  56,
    9,   9,   116, 104, 105, 115, 116, 108, 101, 50,  13,  10,  50,  48,  53,  32,  49,  56,  49,  32,  50,  48,  53,
    9,   9,   116, 104, 105, 115, 116, 108, 101, 51,  13,  10,  49,  51,  57,  32,  49,  50,  51,  32,  49,  51,  57,
    9,   9,   116, 104, 105, 115, 116, 108, 101, 52,  13,  10,  32,  32,  48,  32,  32,  32,  48,  32,  32,  32,  48,
    9,   9,   103, 114, 97,  121, 48,  13,  10,  32,  32,  48,  32,  32,  32,  48,  32,  32,  32,  48,  9,   9,   103,
    114, 101, 121, 48,  13,  10,  32,  32,  51,  32,  32,  32,  51,  32,  32,  32,  51,  9,   9,   103, 114, 97,  121,
    49,  13,  10,  32,  32,  51,  32,  32,  32,  51,  32,  32,  32,  51,  9,   9,   103, 114, 101, 121, 49,  13,  10,
    32,  32,  53,  32,  32,  32,  53,  32,  32,  32,  53,  9,   9,   103, 114, 97,  121, 50,  13,  10,  32,  32,  53,
    32,  32,  32,  53,  32,  32,  32,  53,  9,   9,   103, 114, 101, 121, 50,  13,  10,  32,  32,  56,  32,  32,  32,
    56,  32,  32,  32,  56,  9,   9,   103, 114, 97,  121, 51,  13,  10,  32,  32,  56,  32,  32,  32,  56,  32,  32,
    32,  56,  9,   9,   103, 114, 101, 121, 51,  13,  10,  32,  49,  48,  32,  32,  49,  48,  32,  32,  49,  48,  32,
    9,   9,   103, 114, 97,  121, 52,  13,  10,  32,  49,  48,  32,  32,  49,  48,  32,  32,  49,  48,  32,  9,   9,
    103, 114, 101, 121, 52,  13,  10,  32,  49,  51,  32,  32,  49,  51,  32,  32,  49,  51,  32,  9,   9,   103, 114,
    97,  121, 53,  13,  10,  32,  49,  51,  32,  32,  49,  51,  32,  32,  49,  51,  32,  9,   9,   103, 114, 101, 121,
    53,  13,  10,  32,  49,  53,  32,  32,  49,  53,  32,  32,  49,  53,  32,  9,   9,   103, 114, 97,  121, 54,  13,
    10,  32,  49,  53,  32,  32,  49,  53,  32,  32,  49,  53,  32,  9,   9,   103, 114, 101, 121, 54,  13,  10,  32,
    49,  56,  32,  32,  49,  56,  32,  32,  49,  56,  32,  9,   9,   103, 114, 97,  121, 55,  13,  10,  32,  49,  56,
    32,  32,  49,  56,  32,  32,  49,  56,  32,  9,   9,   103, 114, 101, 121, 55,  13,  10,  32,  50,  48,  32,  32,
    50,  48,  32,  32,  50,  48,  32,  9,   9,   103, 114, 97,  121, 56,  13,  10,  32,  50,  48,  32,  32,  50,  48,
    32,  32,  50,  48,  32,  9,   9,   103, 114, 101, 121, 56,  13,  10,  32,  50,  51,  32,  32,  50,  51,  32,  32,
    50,  51,  32,  9,   9,   103, 114, 97,  121, 57,  13,  10,  32,  50,  51,  32,  32,  50,  51,  32,  32,  50,  51,
    32,  9,   9,   103, 114, 101, 121, 57,  13,  10,  32,  50,  54,  32,  32,  50,  54,  32,  32,  50,  54,  32,  9,
    9,   103, 114, 97,  121, 49,  48,  13,  10,  32,  50,  54,  32,  32,  50,  54,  32,  32,  50,  54,  32,  9,   9,
    103, 114, 101, 121, 49,  48,  13,  10,  32,  50,  56,  32,  32,  50,  56,  32,  32,  50,  56,  32,  9,   9,   103,
    114, 97,  121, 49,  49,  13,  10,  32,  50,  56,  32,  32,  50,  56,  32,  32,  50,  56,  32,  9,   9,   103, 114,
    101, 121, 49,  49,  13,  10,  32,  51,  49,  32,  32,  51,  49,  32,  32,  51,  49,  32,  9,   9,   103, 114, 97,
    121, 49,  50,  13,  10,  32,  51,  49,  32,  32,  51,  49,  32,  32,  51,  49,  32,  9,   9,   103, 114, 101, 121,
    49,  50,  13,  10,  32,  51,  51,  32,  32,  51,  51,  32,  32,  51,  51,  32,  9,   9,   103, 114, 97,  121, 49,
    51,  13,  10,  32,  51,  51,  32,  32,  51,  51,  32,  32,  51,  51,  32,  9,   9,   103, 114, 101, 121, 49,  51,
    13,  10,  32,  51,  54,  32,  32,  51,  54,  32,  32,  51,  54,  32,  9,   9,   103, 114, 97,  121, 49,  52,  13,
    10,  32,  51,  54,  32,  32,  51,  54,  32,  32,  51,  54,  32,  9,   9,   103, 114, 101, 121, 49,  52,  13,  10,
    32,  51,  56,  32,  32,  51,  56,  32,  32,  51,  56,  32,  9,   9,   103, 114, 97,  121, 49,  53,  13,  10,  32,
    51,  56,  32,  32,  51,  56,  32,  32,  51,  56,  32,  9,   9,   103, 114, 101, 121, 49,  53,  13,  10,  32,  52,
    49,  32,  32,  52,  49,  32,  32,  52,  49,  32,  9,   9,   103, 114, 97,  121, 49,  54,  13,  10,  32,  52,  49,
    32,  32,  52,  49,  32,  32,  52,  49,  32,  9,   9,   103, 114, 101, 121, 49,  54,  13,  10,  32,  52,  51,  32,
    32,  52,  51,  32,  32,  52,  51,  32,  9,   9,   103, 114, 97,  121, 49,  55,  13,  10,  32,  52,  51,  32,  32,
    52,  51,  32,  32,  52,  51,  32,  9,   9,   103, 114, 101, 121, 49,  55,  13,  10,  32,  52,  54,  32,  32,  52,
    54,  32,  32,  52,  54,  32,  9,   9,   103, 114, 97,  121, 49,  56,  13,  10,  32,  52,  54,  32,  32,  52,  54,
    32,  32,  52,  54,  32,  9,   9,   103, 114, 101, 121, 49,  56,  13,  10,  32,  52,  56,  32,  32,  52,  56,  32,
    32,  52,  56,  32,  9,   9,   103, 114, 97,  121, 49,  57,  13,  10,  32,  52,  56,  32,  32,  52,  56,  32,  32,
    52,  56,  32,  9,   9,   103, 114, 101, 121, 49,  57,  13,  10,  32,  53,  49,  32,  32,  53,  49,  32,  32,  53,
    49,  32,  9,   9,   103, 114, 97,  121, 50,  48,  13,  10,  32,  53,  49,  32,  32,  53,  49,  32,  32,  53,  49,
    32,  9,   9,   103, 114, 101, 121, 50,  48,  13,  10,  32,  53,  52,  32,  32,  53,  52,  32,  32,  53,  52,  32,
    9,   9,   103, 114, 97,  121, 50,  49,  13,  10,  32,  53,  52,  32,  32,  53,  52,  32,  32,  53,  52,  32,  9,
    9,   103, 114, 101, 121, 50,  49,  13,  10,  32,  53,  54,  32,  32,  53,  54,  32,  32,  53,  54,  32,  9,   9,
    103, 114, 97,  121, 50,  50,  13,  10,  32,  53,  54,  32,  32,  53,  54,  32,  32,  53,  54,  32,  9,   9,   103,
    114, 101, 121, 50,  50,  13,  10,  32,  53,  57,  32,  32,  53,  57,  32,  32,  53,  57,  32,  9,   9,   103, 114,
    97,  121, 50,  51,  13,  10,  32,  53,  57,  32,  32,  53,  57,  32,  32,  53,  57,  32,  9,   9,   103, 114, 101,
    121, 50,  51,  13,  10,  32,  54,  49,  32,  32,  54,  49,  32,  32,  54,  49,  32,  9,   9,   103, 114, 97,  121,
    50,  52,  13,  10,  32,  54,  49,  32,  32,  54,  49,  32,  32,  54,  49,  32,  9,   9,   103, 114, 101, 121, 50,
    52,  13,  10,  32,  54,  52,  32,  32,  54,  52,  32,  32,  54,  52,  32,  9,   9,   103, 114, 97,  121, 50,  53,
    13,  10,  32,  54,  52,  32,  32,  54,  52,  32,  32,  54,  52,  32,  9,   9,   103, 114, 101, 121, 50,  53,  13,
    10,  32,  54,  54,  32,  32,  54,  54,  32,  32,  54,  54,  32,  9,   9,   103, 114, 97,  121, 50,  54,  13,  10,
    32,  54,  54,  32,  32,  54,  54,  32,  32,  54,  54,  32,  9,   9,   103, 114, 101, 121, 50,  54,  13,  10,  32,
    54,  57,  32,  32,  54,  57,  32,  32,  54,  57,  32,  9,   9,   103, 114, 97,  121, 50,  55,  13,  10,  32,  54,
    57,  32,  32,  54,  57,  32,  32,  54,  57,  32,  9,   9,   103, 114, 101, 121, 50,  55,  13,  10,  32,  55,  49,
    32,  32,  55,  49,  32,  32,  55,  49,  32,  9,   9,   103, 114, 97,  121, 50,  56,  13,  10,  32,  55,  49,  32,
    32,  55,  49,  32,  32,  55,  49,  32,  9,   9,   103, 114, 101, 121, 50,  56,  13,  10,  32,  55,  52,  32,  32,
    55,  52,  32,  32,  55,  52,  32,  9,   9,   103, 114, 97,  121, 50,  57,  13,  10,  32,  55,  52,  32,  32,  55,
    52,  32,  32,  55,  52,  32,  9,   9,   103, 114, 101, 121, 50,  57,  13,  10,  32,  55,  55,  32,  32,  55,  55,
    32,  32,  55,  55,  32,  9,   9,   103, 114, 97,  121, 51,  48,  13,  10,  32,  55,  55,  32,  32,  55,  55,  32,
    32,  55,  55,  32,  9,   9,   103, 114, 101, 121, 51,  48,  13,  10,  32,  55,  57,  32,  32,  55,  57,  32,  32,
    55,  57,  32,  9,   9,   103, 114, 97,  121, 51,  49,  13,  10,  32,  55,  57,  32,  32,  55,  57,  32,  32,  55,
    57,  32,  9,   9,   103, 114, 101, 121, 51,  49,  13,  10,  32,  56,  50,  32,  32,  56,  50,  32,  32,  56,  50,
    32,  9,   9,   103, 114, 97,  121, 51,  50,  13,  10,  32,  56,  50,  32,  32,  56,  50,  32,  32,  56,  50,  32,
    9,   9,   103, 114, 101, 121, 51,  50,  13,  10,  32,  56,  52,  32,  32,  56,  52,  32,  32,  56,  52,  32,  9,
    9,   103, 114, 97,  121, 51,  51,  13,  10,  32,  56,  52,  32,  32,  56,  52,  32,  32,  56,  52,  32,  9,   9,
    103, 114, 101, 121, 51,  51,  13,  10,  32,  56,  55,  32,  32,  56,  55,  32,  32,  56,  55,  32,  9,   9,   103,
    114, 97,  121, 51,  52,  13,  10,  32,  56,  55,  32,  32,  56,  55,  32,  32,  56,  55,  32,  9,   9,   103, 114,
    101, 121, 51,  52,  13,  10,  32,  56,  57,  32,  32,  56,  57,  32,  32,  56,  57,  32,  9,   9,   103, 114, 97,
    121, 51,  53,  13,  10,  32,  56,  57,  32,  32,  56,  57,  32,  32,  56,  57,  32,  9,   9,   103, 114, 101, 121,
    51,  53,  13,  10,  32,  57,  50,  32,  32,  57,  50,  32,  32,  57,  50,  32,  9,   9,   103, 114, 97,  121, 51,
    54,  13,  10,  32,  57,  50,  32,  32,  57,  50,  32,  32,  57,  50,  32,  9,   9,   103, 114, 101, 121, 51,  54,
    13,  10,  32,  57,  52,  32,  32,  57,  52,  32,  32,  57,  52,  32,  9,   9,   103, 114, 97,  121, 51,  55,  13,
    10,  32,  57,  52,  32,  32,  57,  52,  32,  32,  57,  52,  32,  9,   9,   103, 114, 101, 121, 51,  55,  13,  10,
    32,  57,  55,  32,  32,  57,  55,  32,  32,  57,  55,  32,  9,   9,   103, 114, 97,  121, 51,  56,  13,  10,  32,
    57,  55,  32,  32,  57,  55,  32,  32,  57,  55,  32,  9,   9,   103, 114, 101, 121, 51,  56,  13,  10,  32,  57,
    57,  32,  32,  57,  57,  32,  32,  57,  57,  32,  9,   9,   103, 114, 97,  121, 51,  57,  13,  10,  32,  57,  57,
    32,  32,  57,  57,  32,  32,  57,  57,  32,  9,   9,   103, 114, 101, 121, 51,  57,  13,  10,  49,  48,  50,  32,
    49,  48,  50,  32,  49,  48,  50,  32,  9,   9,   103, 114, 97,  121, 52,  48,  13,  10,  49,  48,  50,  32,  49,
    48,  50,  32,  49,  48,  50,  32,  9,   9,   103, 114, 101, 121, 52,  48,  13,  10,  49,  48,  53,  32,  49,  48,
    53,  32,  49,  48,  53,  32,  9,   9,   103, 114, 97,  121, 52,  49,  13,  10,  49,  48,  53,  32,  49,  48,  53,
    32,  49,  48,  53,  32,  9,   9,   103, 114, 101, 121, 52,  49,  13,  10,  49,  48,  55,  32,  49,  48,  55,  32,
    49,  48,  55,  32,  9,   9,   103, 114, 97,  121, 52,  50,  13,  10,  49,  48,  55,  32,  49,  48,  55,  32,  49,
    48,  55,  32,  9,   9,   103, 114, 101, 121, 52,  50,  13,  10,  49,  49,  48,  32,  49,  49,  48,  32,  49,  49,
    48,  32,  9,   9,   103, 114, 97,  121, 52,  51,  13,  10,  49,  49,  48,  32,  49,  49,  48,  32,  49,  49,  48,
    32,  9,   9,   103, 114, 101, 121, 52,  51,  13,  10,  49,  49,  50,  32,  49,  49,  50,  32,  49,  49,  50,  32,
    9,   9,   103, 114, 97,  121, 52,  52,  13,  10,  49,  49,  50,  32,  49,  49,  50,  32,  49,  49,  50,  32,  9,
    9,   103, 114, 101, 121, 52,  52,  13,  10,  49,  49,  53,  32,  49,  49,  53,  32,  49,  49,  53,  32,  9,   9,
    103, 114, 97,  121, 52,  53,  13,  10,  49,  49,  53,  32,  49,  49,  53,  32,  49,  49,  53,  32,  9,   9,   103,
    114, 101, 121, 52,  53,  13,  10,  49,  49,  55,  32,  49,  49,  55,  32,  49,  49,  55,  32,  9,   9,   103, 114,
    97,  121, 52,  54,  13,  10,  49,  49,  55,  32,  49,  49,  55,  32,  49,  49,  55,  32,  9,   9,   103, 114, 101,
    121, 52,  54,  13,  10,  49,  50,  48,  32,  49,  50,  48,  32,  49,  50,  48,  32,  9,   9,   103, 114, 97,  121,
    52,  55,  13,  10,  49,  50,  48,  32,  49,  50,  48,  32,  49,  50,  48,  32,  9,   9,   103, 114, 101, 121, 52,
    55,  13,  10,  49,  50,  50,  32,  49,  50,  50,  32,  49,  50,  50,  32,  9,   9,   103, 114, 97,  121, 52,  56,
    13,  10,  49,  50,  50,  32,  49,  50,  50,  32,  49,  50,  50,  32,  9,   9,   103, 114, 101, 121, 52,  56,  13,
    10,  49,  50,  53,  32,  49,  50,  53,  32,  49,  50,  53,  32,  9,   9,   103, 114, 97,  121, 52,  57,  13,  10,
    49,  50,  53,  32,  49,  50,  53,  32,  49,  50,  53,  32,  9,   9,   103, 114, 101, 121, 52,  57,  13,  10,  49,
    50,  55,  32,  49,  50,  55,  32,  49,  50,  55,  32,  9,   9,   103, 114, 97,  121, 53,  48,  13,  10,  49,  50,
    55,  32,  49,  50,  55,  32,  49,  50,  55,  32,  9,   9,   103, 114, 101, 121, 53,  48,  13,  10,  49,  51,  48,
    32,  49,  51,  48,  32,  49,  51,  48,  32,  9,   9,   103, 114, 97,  121, 53,  49,  13,  10,  49,  51,  48,  32,
    49,  51,  48,  32,  49,  51,  48,  32,  9,   9,   103, 114, 101, 121, 53,  49,  13,  10,  49,  51,  51,  32,  49,
    51,  51,  32,  49,  51,  51,  32,  9,   9,   103, 114, 97,  121, 53,  50,  13,  10,  49,  51,  51,  32,  49,  51,
    51,  32,  49,  51,  51,  32,  9,   9,   103, 114, 101, 121, 53,  50,  13,  10,  49,  51,  53,  32,  49,  51,  53,
    32,  49,  51,  53,  32,  9,   9,   103, 114, 97,  121, 53,  51,  13,  10,  49,  51,  53,  32,  49,  51,  53,  32,
    49,  51,  53,  32,  9,   9,   103, 114, 101, 121, 53,  51,  13,  10,  49,  51,  56,  32,  49,  51,  56,  32,  49,
    51,  56,  32,  9,   9,   103, 114, 97,  121, 53,  52,  13,  10,  49,  51,  56,  32,  49,  51,  56,  32,  49,  51,
    56,  32,  9,   9,   103, 114, 101, 121, 53,  52,  13,  10,  49,  52,  48,  32,  49,  52,  48,  32,  49,  52,  48,
    32,  9,   9,   103, 114, 97,  121, 53,  53,  13,  10,  49,  52,  48,  32,  49,  52,  48,  32,  49,  52,  48,  32,
    9,   9,   103, 114, 101, 121, 53,  53,  13,  10,  49,  52,  51,  32,  49,  52,  51,  32,  49,  52,  51,  32,  9,
    9,   103, 114, 97,  121, 53,  54,  13,  10,  49,  52,  51,  32,  49,  52,  51,  32,  49,  52,  51,  32,  9,   9,
    103, 114, 101, 121, 53,  54,  13,  10,  49,  52,  53,  32,  49,  52,  53,  32,  49,  52,  53,  32,  9,   9,   103,
    114, 97,  121, 53,  55,  13,  10,  49,  52,  53,  32,  49,  52,  53,  32,  49,  52,  53,  32,  9,   9,   103, 114,
    101, 121, 53,  55,  13,  10,  49,  52,  56,  32,  49,  52,  56,  32,  49,  52,  56,  32,  9,   9,   103, 114, 97,
    121, 53,  56,  13,  10,  49,  52,  56,  32,  49,  52,  56,  32,  49,  52,  56,  32,  9,   9,   103, 114, 101, 121,
    53,  56,  13,  10,  49,  53,  48,  32,  49,  53,  48,  32,  49,  53,  48,  32,  9,   9,   103, 114, 97,  121, 53,
    57,  13,  10,  49,  53,  48,  32,  49,  53,  48,  32,  49,  53,  48,  32,  9,   9,   103, 114, 101, 121, 53,  57,
    13,  10,  49,  53,  51,  32,  49,  53,  51,  32,  49,  53,  51,  32,  9,   9,   103, 114, 97,  121, 54,  48,  13,
    10,  49,  53,  51,  32,  49,  53,  51,  32,  49,  53,  51,  32,  9,   9,   103, 114, 101, 121, 54,  48,  13,  10,
    49,  53,  54,  32,  49,  53,  54,  32,  49,  53,  54,  32,  9,   9,   103, 114, 97,  121, 54,  49,  13,  10,  49,
    53,  54,  32,  49,  53,  54,  32,  49,  53,  54,  32,  9,   9,   103, 114, 101, 121, 54,  49,  13,  10,  49,  53,
    56,  32,  49,  53,  56,  32,  49,  53,  56,  32,  9,   9,   103, 114, 97,  121, 54,  50,  13,  10,  49,  53,  56,
    32,  49,  53,  56,  32,  49,  53,  56,  32,  9,   9,   103, 114, 101, 121, 54,  50,  13,  10,  49,  54,  49,  32,
    49,  54,  49,  32,  49,  54,  49,  32,  9,   9,   103, 114, 97,  121, 54,  51,  13,  10,  49,  54,  49,  32,  49,
    54,  49,  32,  49,  54,  49,  32,  9,   9,   103, 114, 101, 121, 54,  51,  13,  10,  49,  54,  51,  32,  49,  54,
    51,  32,  49,  54,  51,  32,  9,   9,   103, 114, 97,  121, 54,  52,  13,  10,  49,  54,  51,  32,  49,  54,  51,
    32,  49,  54,  51,  32,  9,   9,   103, 114, 101, 121, 54,  52,  13,  10,  49,  54,  54,  32,  49,  54,  54,  32,
    49,  54,  54,  32,  9,   9,   103, 114, 97,  121, 54,  53,  13,  10,  49,  54,  54,  32,  49,  54,  54,  32,  49,
    54,  54,  32,  9,   9,   103, 114, 101, 121, 54,  53,  13,  10,  49,  54,  56,  32,  49,  54,  56,  32,  49,  54,
    56,  32,  9,   9,   103, 114, 97,  121, 54,  54,  13,  10,  49,  54,  56,  32,  49,  54,  56,  32,  49,  54,  56,
    32,  9,   9,   103, 114, 101, 121, 54,  54,  13,  10,  49,  55,  49,  32,  49,  55,  49,  32,  49,  55,  49,  32,
    9,   9,   103, 114, 97,  121, 54,  55,  13,  10,  49,  55,  49,  32,  49,  55,  49,  32,  49,  55,  49,  32,  9,
    9,   103, 114, 101, 121, 54,  55,  13,  10,  49,  55,  51,  32,  49,  55,  51,  32,  49,  55,  51,  32,  9,   9,
    103, 114, 97,  121, 54,  56,  13,  10,  49,  55,  51,  32,  49,  55,  51,  32,  49,  55,  51,  32,  9,   9,   103,
    114, 101, 121, 54,  56,  13,  10,  49,  55,  54,  32,  49,  55,  54,  32,  49,  55,  54,  32,  9,   9,   103, 114,
    97,  121, 54,  57,  13,  10,  49,  55,  54,  32,  49,  55,  54,  32,  49,  55,  54,  32,  9,   9,   103, 114, 101,
    121, 54,  57,  13,  10,  49,  55,  57,  32,  49,  55,  57,  32,  49,  55,  57,  32,  9,   9,   103, 114, 97,  121,
    55,  48,  13,  10,  49,  55,  57,  32,  49,  55,  57,  32,  49,  55,  57,  32,  9,   9,   103, 114, 101, 121, 55,
    48,  13,  10,  49,  56,  49,  32,  49,  56,  49,  32,  49,  56,  49,  32,  9,   9,   103, 114, 97,  121, 55,  49,
    13,  10,  49,  56,  49,  32,  49,  56,  49,  32,  49,  56,  49,  32,  9,   9,   103, 114, 101, 121, 55,  49,  13,
    10,  49,  56,  52,  32,  49,  56,  52,  32,  49,  56,  52,  32,  9,   9,   103, 114, 97,  121, 55,  50,  13,  10,
    49,  56,  52,  32,  49,  56,  52,  32,  49,  56,  52,  32,  9,   9,   103, 114, 101, 121, 55,  50,  13,  10,  49,
    56,  54,  32,  49,  56,  54,  32,  49,  56,  54,  32,  9,   9,   103, 114, 97,  121, 55,  51,  13,  10,  49,  56,
    54,  32,  49,  56,  54,  32,  49,  56,  54,  32,  9,   9,   103, 114, 101, 121, 55,  51,  13,  10,  49,  56,  57,
    32,  49,  56,  57,  32,  49,  56,  57,  32,  9,   9,   103, 114, 97,  121, 55,  52,  13,  10,  49,  56,  57,  32,
    49,  56,  57,  32,  49,  56,  57,  32,  9,   9,   103, 114, 101, 121, 55,  52,  13,  10,  49,  57,  49,  32,  49,
    57,  49,  32,  49,  57,  49,  32,  9,   9,   103, 114, 97,  121, 55,  53,  13,  10,  49,  57,  49,  32,  49,  57,
    49,  32,  49,  57,  49,  32,  9,   9,   103, 114, 101, 121, 55,  53,  13,  10,  49,  57,  52,  32,  49,  57,  52,
    32,  49,  57,  52,  32,  9,   9,   103, 114, 97,  121, 55,  54,  13,  10,  49,  57,  52,  32,  49,  57,  52,  32,
    49,  57,  52,  32,  9,   9,   103, 114, 101, 121, 55,  54,  13,  10,  49,  57,  54,  32,  49,  57,  54,  32,  49,
    57,  54,  32,  9,   9,   103, 114, 97,  121, 55,  55,  13,  10,  49,  57,  54,  32,  49,  57,  54,  32,  49,  57,
    54,  32,  9,   9,   103, 114, 101, 121, 55,  55,  13,  10,  49,  57,  57,  32,  49,  57,  57,  32,  49,  57,  57,
    32,  9,   9,   103, 114, 97,  121, 55,  56,  13,  10,  49,  57,  57,  32,  49,  57,  57,  32,  49,  57,  57,  32,
    9,   9,   103, 114, 101, 121, 55,  56,  13,  10,  50,  48,  49,  32,  50,  48,  49,  32,  50,  48,  49,  32,  9,
    9,   103, 114, 97,  121, 55,  57,  13,  10,  50,  48,  49,  32,  50,  48,  49,  32,  50,  48,  49,  32,  9,   9,
    103, 114, 101, 121, 55,  57,  13,  10,  50,  48,  52,  32,  50,  48,  52,  32,  50,  48,  52,  32,  9,   9,   103,
    114, 97,  121, 56,  48,  13,  10,  50,  48,  52,  32,  50,  48,  52,  32,  50,  48,  52,  32,  9,   9,   103, 114,
    101, 121, 56,  48,  13,  10,  50,  48,  55,  32,  50,  48,  55,  32,  50,  48,  55,  32,  9,   9,   103, 114, 97,
    121, 56,  49,  13,  10,  50,  48,  55,  32,  50,  48,  55,  32,  50,  48,  55,  32,  9,   9,   103, 114, 101, 121,
    56,  49,  13,  10,  50,  48,  57,  32,  50,  48,  57,  32,  50,  48,  57,  32,  9,   9,   103, 114, 97,  121, 56,
    50,  13,  10,  50,  48,  57,  32,  50,  48,  57,  32,  50,  48,  57,  32,  9,   9,   103, 114, 101, 121, 56,  50,
    13,  10,  50,  49,  50,  32,  50,  49,  50,  32,  50,  49,  50,  32,  9,   9,   103, 114, 97,  121, 56,  51,  13,
    10,  50,  49,  50,  32,  50,  49,  50,  32,  50,  49,  50,  32,  9,   9,   103, 114, 101, 121, 56,  51,  13,  10,
    50,  49,  52,  32,  50,  49,  52,  32,  50,  49,  52,  32,  9,   9,   103, 114, 97,  121, 56,  52,  13,  10,  50,
    49,  52,  32,  50,  49,  52,  32,  50,  49,  52,  32,  9,   9,   103, 114, 101, 121, 56,  52,  13,  10,  50,  49,
    55,  32,  50,  49,  55,  32,  50,  49,  55,  32,  9,   9,   103, 114, 97,  121, 56,  53,  13,  10,  50,  49,  55,
    32,  50,  49,  55,  32,  50,  49,  55,  32,  9,   9,   103, 114, 101, 121, 56,  53,  13,  10,  50,  49,  57,  32,
    50,  49,  57,  32,  50,  49,  57,  32,  9,   9,   103, 114, 97,  121, 56,  54,  13,  10,  50,  49,  57,  32,  50,
    49,  57,  32,  50,  49,  57,  32,  9,   9,   103, 114, 101, 121, 56,  54,  13,  10,  50,  50,  50,  32,  50,  50,
    50,  32,  50,  50,  50,  32,  9,   9,   103, 114, 97,  121, 56,  55,  13,  10,  50,  50,  50,  32,  50,  50,  50,
    32,  50,  50,  50,  32,  9,   9,   103, 114, 101, 121, 56,  55,  13,  10,  50,  50,  52,  32,  50,  50,  52,  32,
    50,  50,  52,  32,  9,   9,   103, 114, 97,  121, 56,  56,  13,  10,  50,  50,  52,  32,  50,  50,  52,  32,  50,
    50,  52,  32,  9,   9,   103, 114, 101, 121, 56,  56,  13,  10,  50,  50,  55,  32,  50,  50,  55,  32,  50,  50,
    55,  32,  9,   9,   103, 114, 97,  121, 56,  57,  13,  10,  50,  50,  55,  32,  50,  50,  55,  32,  50,  50,  55,
    32,  9,   9,   103, 114, 101, 121, 56,  57,  13,  10,  50,  50,  57,  32,  50,  50,  57,  32,  50,  50,  57,  32,
    9,   9,   103, 114, 97,  121, 57,  48,  13,  10,  50,  50,  57,  32,  50,  50,  57,  32,  50,  50,  57,  32,  9,
    9,   103, 114, 101, 121, 57,  48,  13,  10,  50,  51,  50,  32,  50,  51,  50,  32,  50,  51,  50,  32,  9,   9,
    103, 114, 97,  121, 57,  49,  13,  10,  50,  51,  50,  32,  50,  51,  50,  32,  50,  51,  50,  32,  9,   9,   103,
    114, 101, 121, 57,  49,  13,  10,  50,  51,  53,  32,  50,  51,  53,  32,  50,  51,  53,  32,  9,   9,   103, 114,
    97,  121, 57,  50,  13,  10,  50,  51,  53,  32,  50,  51,  53,  32,  50,  51,  53,  32,  9,   9,   103, 114, 101,
    121, 57,  50,  13,  10,  50,  51,  55,  32,  50,  51,  55,  32,  50,  51,  55,  32,  9,   9,   103, 114, 97,  121,
    57,  51,  13,  10,  50,  51,  55,  32,  50,  51,  55,  32,  50,  51,  55,  32,  9,   9,   103, 114, 101, 121, 57,
    51,  13,  10,  50,  52,  48,  32,  50,  52,  48,  32,  50,  52,  48,  32,  9,   9,   103, 114, 97,  121, 57,  52,
    13,  10,  50,  52,  48,  32,  50,  52,  48,  32,  50,  52,  48,  32,  9,   9,   103, 114, 101, 121, 57,  52,  13,
    10,  50,  52,  50,  32,  50,  52,  50,  32,  50,  52,  50,  32,  9,   9,   103, 114, 97,  121, 57,  53,  13,  10,
    50,  52,  50,  32,  50,  52,  50,  32,  50,  52,  50,  32,  9,   9,   103, 114, 101, 121, 57,  53,  13,  10,  50,
    52,  53,  32,  50,  52,  53,  32,  50,  52,  53,  32,  9,   9,   103, 114, 97,  121, 57,  54,  13,  10,  50,  52,
    53,  32,  50,  52,  53,  32,  50,  52,  53,  32,  9,   9,   103, 114, 101, 121, 57,  54,  13,  10,  50,  52,  55,
    32,  50,  52,  55,  32,  50,  52,  55,  32,  9,   9,   103, 114, 97,  121, 57,  55,  13,  10,  50,  52,  55,  32,
    50,  52,  55,  32,  50,  52,  55,  32,  9,   9,   103, 114, 101, 121, 57,  55,  13,  10,  50,  53,  48,  32,  50,
    53,  48,  32,  50,  53,  48,  32,  9,   9,   103, 114, 97,  121, 57,  56,  13,  10,  50,  53,  48,  32,  50,  53,
    48,  32,  50,  53,  48,  32,  9,   9,   103, 114, 101, 121, 57,  56,  13,  10,  50,  53,  50,  32,  50,  53,  50,
    32,  50,  53,  50,  32,  9,   9,   103, 114, 97,  121, 57,  57,  13,  10,  50,  53,  50,  32,  50,  53,  50,  32,
    50,  53,  50,  32,  9,   9,   103, 114, 101, 121, 57,  57,  13,  10,  50,  53,  53,  32,  50,  53,  53,  32,  50,
    53,  53,  32,  9,   9,   103, 114, 97,  121, 49,  48,  48,  13,  10,  50,  53,  53,  32,  50,  53,  53,  32,  50,
    53,  53,  32,  9,   9,   103, 114, 101, 121, 49,  48,  48,  13,  10,  49,  54,  57,  32,  49,  54,  57,  32,  49,
    54,  57,  9,   9,   100, 97,  114, 107, 32,  103, 114, 101, 121, 13,  10,  49,  54,  57,  32,  49,  54,  57,  32,
    49,  54,  57,  9,   9,   68,  97,  114, 107, 71,  114, 101, 121, 13,  10,  49,  54,  57,  32,  49,  54,  57,  32,
    49,  54,  57,  9,   9,   100, 97,  114, 107, 32,  103, 114, 97,  121, 13,  10,  49,  54,  57,  32,  49,  54,  57,
    32,  49,  54,  57,  9,   9,   68,  97,  114, 107, 71,  114, 97,  121, 13,  10,  48,  32,  32,  32,  32,  32,  48,
    32,  49,  51,  57,  9,   9,   100, 97,  114, 107, 32,  98,  108, 117, 101, 13,  10,  48,  32,  32,  32,  32,  32,
    48,  32,  49,  51,  57,  9,   9,   68,  97,  114, 107, 66,  108, 117, 101, 13,  10,  48,  32,  32,  32,  49,  51,
    57,  32,  49,  51,  57,  9,   9,   100, 97,  114, 107, 32,  99,  121, 97,  110, 13,  10,  48,  32,  32,  32,  49,
    51,  57,  32,  49,  51,  57,  9,   9,   68,  97,  114, 107, 67,  121, 97,  110, 13,  10,  49,  51,  57,  32,  32,
    32,  48,  32,  49,  51,  57,  9,   9,   100, 97,  114, 107, 32,  109, 97,  103, 101, 110, 116, 97,  13,  10,  49,
    51,  57,  32,  32,  32,  48,  32,  49,  51,  57,  9,   9,   68,  97,  114, 107, 77,  97,  103, 101, 110, 116, 97,
    13,  10,  49,  51,  57,  32,  32,  32,  48,  32,  32,  32,  48,  9,   9,   100, 97,  114, 107, 32,  114, 101, 100,
    13,  10,  49,  51,  57,  32,  32,  32,  48,  32,  32,  32,  48,  9,   9,   68,  97,  114, 107, 82,  101, 100, 13,
    10,  49,  52,  52,  32,  50,  51,  56,  32,  49,  52,  52,  9,   9,   108, 105, 103, 104, 116, 32,  103, 114, 101,
    101, 110, 13,  10,  49,  52,  52,  32,  50,  51,  56,  32,  49,  52,  52,  9,   9,   76,  105, 103, 104, 116, 71,
    114, 101, 101, 110, 13,  10,  13,  10,  13,  10,  0,
};

//
// V_GetColorStringByName
//
// Returns a string with 6 hexadecimal digits suitable for use with
// V_GetColorFromString. A given colorname is looked up in the X11R6RGB lump
// and its value is returned.
//
std::string V_GetColorStringByName(const std::string &name)
{
    char *rgbNames, *data, descr[5 * 3];
    int32_t   c[3], step;

    rgbNames = (char *)x11r6rgb;

    // skip past the header line
    data = strchr(rgbNames, '\n');
    step = 0;

    while ((data = COM_Parse(data)))
    {
        if (step < 3)
        {
            c[step++] = atoi(com_token);
        }
        else
        {
            step = 0;
            if (*data >= ' ') // In case this name contains a space...
            {
                char *newchar = com_token + strlen(com_token);

                while (*data >= ' ')
                    *newchar++ = *data++;
                *newchar = 0;
            }

            if (!stricmp(com_token, name.c_str()))
            {
                sprintf(descr, "%04x %04x %04x", (c[0] << 8) | c[0], (c[1] << 8) | c[1], (c[2] << 8) | c[2]);
                return descr;
            }
        }
    }
    return "";
}

VERSION_CONTROL(v_palette_cpp, "$Id: adae0296eb3e28c00375589b51c81413ecdda97c $")
