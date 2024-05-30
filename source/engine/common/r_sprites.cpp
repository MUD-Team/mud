// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 875d809dc604f6d3d7e5cfe89d80c1206cb918c2 $
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
//		Loading sprites, skins.
//
//-----------------------------------------------------------------------------

#include "r_sprites.h"

#include "i_system.h"
#include "m_alloc.h"
#include "m_fileio.h"
#include "mud_includes.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#define SPRITE_NEEDS_INFO INT32_MAX

//
// INITIALIZATION FUNCTIONS
//
spritedef_t *sprites;
int32_t          numsprites;

spriteframe_t sprtemp[MAX_SPRITE_FRAMES];
int32_t           maxframe;

void R_CacheSprite(spritedef_t *sprite)
{
    DPrintf("cache sprite %s\n", sprite - sprites < NUMSPRITES ? sprnames[sprite - sprites] : "");
    for (int32_t i = 0; i < sprite->numframes; i++)
    {
        for (int32_t r = 0; r < 8; r++)
        {
            if (sprite->spriteframes[i].width[r] == SPRITE_NEEDS_INFO)
            {
                if (sprite->spriteframes[i].texes[r] == TextureManager::NO_TEXTURE_HANDLE)
                    I_Error("Sprite %d, rotation %d has no lump", i, r);
                patch_t *patch                       = (patch_t *)texturemanager.getTexture(sprite->spriteframes[i].texes[r])->getData();
                sprite->spriteframes[i].width[r]     = patch->width() << FRACBITS;
                sprite->spriteframes[i].offset[r]    = patch->leftoffset() << FRACBITS;
                sprite->spriteframes[i].topoffset[r] = patch->topoffset() << FRACBITS;
            }
        }
    }
}

//
// R_InstallSpriteTex
// Local function for R_InitSprites.
//
// [RH] Removed checks for coexistance of rotation 0 with other
//		rotations and made it look more like BOOM's version.
//
void R_InstallSpriteTex(const texhandle_t tex_id, uint32_t frame, uint32_t rot, bool flipped)
{
	if (frame >= MAX_SPRITE_FRAMES || rot > 8)
		I_Error ("R_InstallSpriteTex: Bad frame characters in resource ID %i", (int32_t)tex_id);

	if (static_cast<int32_t>(frame) > maxframe)
		maxframe = frame;

	if (rot == 0)
	{
		// the resource should be used for all rotations
        // false=0, true=1, but array initialised to -1
        // allows doom to have a "no value set yet" boolean value!
		for (int32_t r = 7; r >= 0; r--)
		{
			if (sprtemp[frame].texes[r] == TextureManager::NO_TEXTURE_HANDLE)
			{
				sprtemp[frame].texes[r] = tex_id;
				sprtemp[frame].flip[r] = (uint8_t)flipped;
				sprtemp[frame].rotate = false;
				sprtemp[frame].width[r] = SPRITE_NEEDS_INFO;
			}
		}
		
		return;
	}
	else if (sprtemp[frame].texes[--rot] == TextureManager::NO_TEXTURE_HANDLE)
	{
		// the lump is only used for one rotation
		sprtemp[frame].texes[rot] = tex_id;
		sprtemp[frame].flip[rot] = (uint8_t)flipped;
		sprtemp[frame].rotate = true;
		sprtemp[frame].width[rot] = SPRITE_NEEDS_INFO;
	}
}

// [RH] Seperated out of R_InitSpriteDefs()
void R_InstallSprite(const char* name, int32_t num)
{
	char sprname[5];
	int32_t frame;

	if (maxframe == -1)
	{
		sprites[num].numframes = 0;
		return;
	}

	strncpy(sprname, name, 4);
	sprname[4] = 0;

	maxframe++;

	for (frame = 0; frame < maxframe; frame++)
	{
		switch ((int32_t)sprtemp[frame].rotate)
		{
		case -1:
			// no rotations were found for that frame at all
			I_Error("R_InstallSprite: No patches found for %s frame %c", sprname,
			             frame + 'A');
			break;

		case 0:
			// only the first rotation is needed
			break;

		case 1:
			// must have all 8 frames
			{
				for (int32_t rotation = 0; rotation < 8; rotation++)
				{
					if (sprtemp[frame].texes[rotation] == TextureManager::NO_TEXTURE_HANDLE)
						I_Error(
						    "R_InstallSprite: Sprite %s frame %c is missing rotations",
						    sprname, frame + 'A');
				}
			}
			break;
		}
	}

	// allocate space for the frames present and copy sprtemp to it
	sprites[num].numframes = maxframe;
	sprites[num].spriteframes =
	    (spriteframe_t*)Z_Malloc(maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
	memcpy(sprites[num].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
}

//
// GAME FUNCTIONS
//
int32_t          MaxVisSprites;
vissprite_t *vissprites;
vissprite_t *lastvissprite;

//
// R_InitSprites
// Called at program start.
//
void R_InitSprites()
{
    MaxVisSprites = 128; // [RH] This is the initial default value. It grows as needed.

    M_Free(vissprites);

    vissprites    = (vissprite_t *)Malloc(MaxVisSprites * sizeof(vissprite_t));
    lastvissprite = &vissprites[MaxVisSprites];
}

VERSION_CONTROL(r_sprites_cpp, "$Id: 875d809dc604f6d3d7e5cfe89d80c1206cb918c2 $")
