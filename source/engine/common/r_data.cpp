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
