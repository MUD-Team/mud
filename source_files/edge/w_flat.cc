//----------------------------------------------------------------------------
//  EDGE Rendering Data Handling Code
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
//
// -ACB- 1998/09/09 Reformatted File Layout.
// -KM- 1998/09/27 Colourmaps can be dynamically changed.
// -ES- 2000/02/12 Moved most of this module to w_texture.c.

#include "w_flat.h"

#include <algorithm>
#include <vector>

#include "ddf_anim.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_search.h"
#include "epi.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_image.h"
#include "r_sky.h"
#include "w_files.h"
#include "w_sprite.h"


EDGE_DEFINE_CONSOLE_VARIABLE(precache_textures, "1", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(precache_sprites, "1", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(precache_models, "1", kConsoleVariableFlagArchive)

//
// AddFlatAnimation
//
// Here are the rules for flats, they get a bit hairy, but are the
// simplest thing which achieves expected behaviour:
//
// 1. When two flats in different wads have the same name, the flat
//    in the _later_ wad overrides the flat in the earlier wad.  This
//    allows pwads to replace iwad flats -- as is usual.  For general
//    use of flats (e.g. in levels) their order is not an issue.
//
// 2. The flat animation sequence is determined by the _earliest_ wad
//    which contains _both_ the start and the end flat.  The sequence
//    contained in that wad becomes the animation sequence (the list
//    of flat names).  These names are then looked up normally, so
//    flats in newer wads will get used if their name matches one in
//    the sequence.
//
// -AJA- 2001/01/28: reworked flat animations.
//
static void AddFlatAnimation(AnimationDefinition *anim)
{
    // -AJA- 2004/10/27: new SEQUENCE command for anims

    int total = (int)anim->pics_.size();

    if (total == 1)
        return;

    const Image **flats = new const Image *[total];

    for (int i = 0; i < total; i++)
    {
        flats[i] = ImageLookup(anim->pics_[i].c_str(), kImageNamespaceFlat, kImageLookupNull | kImageLookupExact);
    }

    AnimateImageSet(flats, total, anim->speed_);
    delete[] flats;
}




//
// AddTextureAnimation
//
// Here are the rules for textures:
//
// 1. The TEXTURE1/2 lumps require a PNAMES lump to complete their
//    meaning.  Some wads have the TEXTURE1/2 lump(s) but lack a
//    PNAMES lump -- in this case the next oldest PNAMES lump is used
//    (e.g. the one in the IWAD).
//
// 2. When two textures in different wads have the same name, the
//    texture in the _later_ wad overrides the one in the earlier wad,
//    as is usual.  For general use of textures (e.g. in levels),
//    their ordering is not an issue.
//
// 3. The texture animation sequence is determined by the _latest_ wad
//    whose TEXTURE1/2 lump contains _both_ the start and the end
//    texture.  The sequence within that lump becomes the animation
//    sequence (the list of texture names).  These names are then
//    looked up normally, so textures in newer wads can get used if
//    their name matches one in the sequence.
//
// -AJA- 2001/06/17: reworked texture animations.
//
static void AddTextureAnimation(AnimationDefinition *anim)
{
    // -AJA- 2004/10/27: new SEQUENCE command for anims

    int total = (int)anim->pics_.size();

    if (total == 1)
        return;

    const Image **texs = new const Image *[total];

    for (int i = 0; i < total; i++)
    {
        texs[i] = ImageLookup(anim->pics_[i].c_str(), kImageNamespaceTexture, kImageLookupNull | kImageLookupExact);
    }

    AnimateImageSet(texs, total, anim->speed_);
    delete[] texs;
}

//
// AddGraphicAnimation
//
static void AddGraphicAnimation(AnimationDefinition *anim)
{
    int total = (int)anim->pics_.size();

    EPI_ASSERT(total != 0);

    if (total == 1)
        return;

    const Image **users = new const Image *[total];

    for (int i = 0; i < total; i++)
    {
        users[i] = ImageLookup(anim->pics_[i].c_str(), kImageNamespaceGraphic, kImageLookupNull | kImageLookupExact);
    }

    AnimateImageSet(users, total, anim->speed_);
    delete[] users;
}

//
// InitializeAnimations
//
void InitializeAnimations(void)
{
    // loop through animdefs, and add relevant anims.
    // Note: reverse order, give priority to newer anims.
    for (std::vector<AnimationDefinition *>::reverse_iterator iter = animdefs.rbegin(), iter_end = animdefs.rend();
         iter != iter_end; iter++)
    {
        AnimationDefinition *A = *iter;

        EPI_ASSERT(A);

        switch (A->type_)
        {
        case AnimationDefinition::kAnimationTypeTexture:
            AddTextureAnimation(A);
            break;

        case AnimationDefinition::kAnimationTypeFlat:
            AddFlatAnimation(A);
            break;

        case AnimationDefinition::kAnimationTypeGraphic:
            AddGraphicAnimation(A);
            break;
        }
    }
}

static void PrecacheTextures(void)
{
    // maximum possible images
    int max_image = 1 + 3 * total_level_sides + 2 * total_level_sectors;
    int count     = 0;

    const Image **images = new const Image *[max_image];

    // Sky texture is always present.
    images[count++] = sky_image;

    // add in sidedefs
    for (int i = 0; i < total_level_sides; i++)
    {
        if (level_sides[i].top.image)
            images[count++] = level_sides[i].top.image;

        if (level_sides[i].middle.image)
            images[count++] = level_sides[i].middle.image;

        if (level_sides[i].bottom.image)
            images[count++] = level_sides[i].bottom.image;
    }

    EPI_ASSERT(count <= max_image);

    // add in planes
    for (int i = 0; i < total_level_sectors; i++)
    {
        if (level_sectors[i].floor.image)
            images[count++] = level_sectors[i].floor.image;

        if (level_sectors[i].ceiling.image)
            images[count++] = level_sectors[i].ceiling.image;
    }

    EPI_ASSERT(count <= max_image);

    // Sort the images, so we can ignore the duplicates

#define EDGE_CMP(a, b) (a < b)
    EDGE_QSORT(const Image *, images, count, 10);
#undef EDGE_CMP

    for (int i = 0; i < count; i++)
    {
        EPI_ASSERT(images[i]);

        if (i + 1 < count && images[i] == images[i + 1])
            continue;

        if (images[i] == sky_flat_image)
            continue;

        ImagePrecache(images[i]);
    }

    delete[] images;
}

//
// PrecacheLevelGraphics
//
// Preloads all relevant graphics for the level.
//
// -AJA- 2001/06/18: Reworked for image system.
//
void PrecacheLevelGraphics(void)
{
    if (precache_sprites.d_)
        PrecacheSprites();

    if (precache_textures.d_)
        PrecacheTextures();

/*
    if (precache_models.d_)
        PrecacheModels();
*/
    PrecacheSky();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
