// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 78554932593a4dead8644d10dd1689c12c64f358 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
#include "mud_includes.h"
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
