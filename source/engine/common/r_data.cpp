// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 78554932593a4dead8644d10dd1689c12c64f358 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// Revision 1.3  1997/01/29 20:10
// DESCRIPTION:
//		Preparation of data for rendering,
//		generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------

#include "r_data.h"

#include <ctype.h>

#include <algorithm>

#include "cmdlib.h"
#include "i_system.h"
#include "m_fileio.h"
#include "odamex.h"
#include "r_local.h"
#include "r_sky.h"
#include "v_palette.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

//
// R_CalculateNewPatchSize
//
// Helper function for converting raw patches that use post_t into patches
// that use tallpost_t. Returns the lump size of the converted patch.
//
size_t R_CalculateNewPatchSize(patch_t *patch, size_t length)
{
    if (!patch)
        return 0;

    // sanity check to see if the postofs array fits in the patch lump
    if (length < patch->width() * sizeof(uint32_t))
        return 0;

    int32_t           numposts = 0, numpixels = 0;
    uint32_t *postofs = (uint32_t *)((uint8_t *)patch + 8);

    for (int32_t i = 0; i < patch->width(); i++)
    {
        size_t ofs = LELONG(postofs[i]);

        // check that the offset is valid
        if (ofs >= length)
            return 0;

        post_t *post = (post_t *)((uint8_t *)patch + ofs);

        while (post->topdelta != 0xFF)
        {
            if (ofs + post->length >= length)
                return 0; // patch is corrupt

            numposts++;
            numpixels += post->length;
            post = (post_t *)((uint8_t *)post + post->length + 4);
        }
    }

    // 8 byte patch header
    // 4 * width bytes for column offset table
    // 4 bytes per post for post header
    // 1 byte per pixel
    // 2 bytes per column for termination
    return 8 + 4 * patch->width() + 4 * numposts + numpixels + 2 * patch->width();
}

//
// R_ConvertPatch
//
// Converts a patch that uses post_t posts into a patch that uses tallpost_t.
//
void R_ConvertPatch(patch_t *newpatch, patch_t *rawpatch)
{
    if (!rawpatch || !newpatch)
        return;

    memcpy(newpatch, rawpatch, 8); // copy the patch header

    uint32_t *rawpostofs = rawpatch->ofs();
    uint32_t *newpostofs = newpatch->ofs();

    uint32_t curofs = rawpatch->datastart(); // keep track of the column offset

    for (int32_t i = 0; i < rawpatch->width(); i++)
    {
        int32_t newpost_top = -1;
        int32_t newpost_len = 0;

        int32_t abs_offset = 0;

        newpostofs[i]       = LELONG(curofs); // write the new offset for this column
        post_t     *rawpost = rawpatch->post(LELONG(rawpostofs[i]));
        tallpost_t *newpost = newpatch->tallpost(curofs);

        while (!rawpost->end())
        {
            // handle DeePsea tall patches where topdelta is treated as a relative
            // offset instead of an absolute offset
            abs_offset = rawpost->abs(abs_offset);
            if (newpost_top == -1)
                newpost_top = abs_offset;

            // watch for column overruns
            int32_t length = rawpost->length;
            if (abs_offset + length > rawpatch->height())
                length = rawpatch->height() - abs_offset;

            if (length < 0)
            {
                I_Error("%s: Patch appears to be corrupted.", __FUNCTION__);
            }

            // copy the pixels in the post
            memcpy(newpost->data() + newpost_len, rawpost->data(), length);
            newpost_len += length;

            // Should we finish the post?
            if (rawpost->next()->end() || abs_offset + length != rawpost->next()->abs(abs_offset))
            {
                newpost->topdelta = newpost_top;
                newpost->length   = newpost_len;

                curofs += newpost->size();
                newpost = newpost->next();

                newpost_top = -1;
                newpost_len = 0;
            }

            rawpost = rawpost->next();
        }

        newpost->writeend();
        curofs += 2;
    }
}

//
// R_GetPatchColumn
//
tallpost_t *R_GetPatchColumn(patch_t *patch, int32_t colnum)
{
    return (tallpost_t *)((uint8_t *)patch + LELONG(patch->columnofs[colnum]));
}

//
// R_GetTextureColumn
//
tallpost_t *R_GetTextureColumn(texhandle_t texnum, int32_t colnum)
{
    const Texture *tex = texturemanager.getTexture(texnum);
    colnum &= tex->getWidthMask();
    patch_t *texpatch = (patch_t *)tex->getData();

    return (tallpost_t *)((uint8_t *)texpatch + LELONG(texpatch->columnofs[colnum]));
}

struct FakeCmap
{
    std::string name;
    argb_t      blend_color;
};

static FakeCmap *fakecmaps = NULL;

size_t     numfakecmaps;
int32_t        firstfakecmap;
shademap_t realcolormaps;

void R_ForceDefaultColormap(const char *name)
{
    std::string cmapfile = StrFormat("lumps/%s.cmp", name);

    if (M_FileExists(cmapfile))
    {
        PHYSFS_File *rawcmap = PHYSFS_openRead(cmapfile.c_str());
        if (rawcmap == NULL)
            I_FatalError("Error opening %s colormap file", cmapfile.c_str());
        if (PHYSFS_fileLength(rawcmap) < ((NUMCOLORMAPS + 1) * 256))
        {
            PHYSFS_close(rawcmap);
            I_FatalError("Malformed %s colormap file (too short)", cmapfile.c_str());
        }
        if (PHYSFS_readBytes(rawcmap, realcolormaps.colormap, (NUMCOLORMAPS + 1) * 256) != (NUMCOLORMAPS + 1) * 256)
        {
            PHYSFS_close(rawcmap);
            I_FatalError("Error loading %s colormap file", cmapfile.c_str());
        }
        PHYSFS_close(rawcmap);
    }
    else
        I_FatalError("Missing %s colormap file", cmapfile.c_str());

    BuildDefaultShademap(V_GetDefaultPalette(), realcolormaps);

    fakecmaps[0].name        = StdStringToUpper(name, 8); // denis - todo - string limit?
    fakecmaps[0].blend_color = argb_t(0, 255, 255, 255);
}

void R_SetDefaultColormap(const char *name)
{
    if (strnicmp(fakecmaps[0].name.c_str(), name, 8) != 0)
        R_ForceDefaultColormap(name);
}

void R_ReinitColormap()
{
    if (fakecmaps == NULL)
        return;

    std::string name = fakecmaps[0].name;
    if (name.empty())
        name = "COLORMAP";

    R_ForceDefaultColormap(name.c_str());
}

//
// R_ShutdownColormaps
//
// Frees the memory allocated specifically for the colormaps.
//
void R_ShutdownColormaps()
{
    if (realcolormaps.colormap)
    {
        Z_Free(realcolormaps.colormap);
        realcolormaps.colormap = NULL;
    }

    if (realcolormaps.shademap)
    {
        Z_Free(realcolormaps.shademap);
        realcolormaps.shademap = NULL;
    }

    if (fakecmaps)
    {
        delete[] fakecmaps;
        fakecmaps = NULL;
    }
}

//
// R_InitColormaps
//
void R_InitColormaps()
{
    // [RH] Try and convert BOOM colormaps into blending values.
    //		This is a really rough hack, but it's better than
    //		not doing anything with them at all (right?)
    int32_t lastfakecmap = W_CheckNumForName("C_END");
    firstfakecmap    = W_CheckNumForName("C_START");

    if (firstfakecmap == -1 || lastfakecmap == -1)
        numfakecmaps = 1;
    else
    {
        if (firstfakecmap > lastfakecmap)
            I_Error("no fake cmaps");

        numfakecmaps = lastfakecmap - firstfakecmap;
    }

    realcolormaps.colormap = (uint8_t *)Z_Malloc(256 * (NUMCOLORMAPS + 1) * numfakecmaps, PU_STATIC, 0);
    realcolormaps.shademap = (argb_t *)Z_Malloc(256 * sizeof(argb_t) * (NUMCOLORMAPS + 1) * numfakecmaps, PU_STATIC, 0);

    delete[] fakecmaps;
    fakecmaps = new FakeCmap[numfakecmaps];

    R_ForceDefaultColormap("COLORMAP");

    if (numfakecmaps > 1)
    {
        const palette_t *pal = V_GetDefaultPalette();

        for (uint32_t i = ++firstfakecmap, j = 1; j < numfakecmaps; i++, j++)
        {
            if (W_LumpLength(i) >= (NUMCOLORMAPS + 1) * 256)
            {
                uint8_t   *map      = (uint8_t *)W_CacheLumpNum(i, PU_CACHE);
                uint8_t   *colormap = realcolormaps.colormap + (NUMCOLORMAPS + 1) * 256 * j;
                argb_t *shademap = realcolormaps.shademap + (NUMCOLORMAPS + 1) * 256 * j;

                // Copy colormap data:
                memcpy(colormap, map, (NUMCOLORMAPS + 1) * 256);

                int32_t r = pal->basecolors[*map].getr();
                int32_t g = pal->basecolors[*map].getg();
                int32_t b = pal->basecolors[*map].getb();

                char name[9];
                W_GetLumpName(name, i);
                fakecmaps[j].name = StdStringToUpper(name, 8);

                for (int32_t k = 1; k < 256; k++)
                {
                    r = (r + pal->basecolors[map[k]].getr()) >> 1;
                    g = (g + pal->basecolors[map[k]].getg()) >> 1;
                    b = (b + pal->basecolors[map[k]].getb()) >> 1;
                }
                // NOTE(jsd): This alpha value is used for 32bpp in water areas.
                argb_t color             = argb_t(64, r, g, b);
                fakecmaps[j].blend_color = color;

                // Set up shademap for the colormap:
                for (int32_t k = 0; k < 256; ++k)
                    shademap[k] = alphablend1a(pal->basecolors[map[0]], color, j * (256 / numfakecmaps));
            }
        }
    }
}

//
// R_ColormapNumForname
//
// [RH] Returns an index into realcolormaps. Multiply it by
//		256*(NUMCOLORMAPS+1) to find the start of the colormap to use.
//
// COLORMAP always returns 0.
//
int32_t R_ColormapNumForName(const char *name)
{
    if (strnicmp(name, "COLORMAP", 8) != 0)
    {
        int32_t lump = W_CheckNumForName(name, ns_colormaps);

        if (lump != -1)
            return lump - firstfakecmap + 1;
    }

    return 0;
}

//
// R_BlendForColormap
//
// Returns a blend value to approximate the given colormap index number.
// Invalid values return the color white with 0% opacity.
//
argb_t R_BlendForColormap(uint32_t index)
{
    if (index > 0 && index < numfakecmaps)
        return fakecmaps[index].blend_color;

    return argb_t(0, 255, 255, 255);
}

//
// R_ColormapForBlend
//
// Returns the colormap index number that has the given blend color value.
//
int32_t R_ColormapForBlend(const argb_t blend_color)
{
    for (uint32_t i = 1; i < numfakecmaps; i++)
        if (fakecmaps[i].blend_color == blend_color)
            return i;
    return 0;
}

//
// R_InitData
// Locates all the lumps
//	that will be used by all views
// Must be called after W_Init.
//
void R_InitData()
{
    // haleyjd 01/28/10: also initialize tantoangle_acc table
    Table_InitTanToAngle();
}

//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//
// [RH] Rewrote this using Lee Killough's code in BOOM as an example.

void R_PrecacheLevel(void)
{
    int32_t   i;

    // Precache flats.
    for (i = numsectors - 1; i >= 0; i--)
    {
        texturemanager.getTexture(sectors[i].floorpic);
        texturemanager.getTexture(sectors[i].ceilingpic);
    }

    // Precache textures.
    for (i = numsides - 1; i >= 0; i--)
    {
        texturemanager.getTexture(sides[i].toptexture);
        texturemanager.getTexture(sides[i].midtexture);
        texturemanager.getTexture(sides[i].bottomtexture);
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //	indicate a sky floor/ceiling as a flat,
    //	while the sky texture is stored like
    //	a wall texture, with an episode dependend
    //	name.
    //
    // [RH] Possibly two sky textures now.
    // [ML] 5/11/06 - Not anymore!

    texturemanager.getTexture(sky1texture);
    texturemanager.getTexture(sky2texture);

    // Precache sprites
    uint8_t *hitlist = new uint8_t[numsprites];
    memset(hitlist, 0, numsprites);

    {
        AActor                  *actor;
        TThinkerIterator<AActor> iterator;

        while ((actor = iterator.Next()))
            hitlist[actor->sprite] = 1;
    }

    for (i = numsprites - 1; i >= 0; i--)
    {
        if (hitlist[i])
            R_CacheSprite(sprites + i);
    }

    delete[] hitlist;
}

// Utility function,
//	called by R_PointToAngle.
uint32_t SlopeDiv(uint32_t num, uint32_t den)
{
    uint32_t ans;

    if (den < 512)
        return SLOPERANGE;

    ans = (num << 3) / (den >> 8);

    return ans <= SLOPERANGE ? ans : SLOPERANGE;
}

VERSION_CONTROL(r_data_cpp, "$Id: 78554932593a4dead8644d10dd1689c12c64f358 $")
