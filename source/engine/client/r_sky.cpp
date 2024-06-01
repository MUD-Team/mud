// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 6aecb96593367ac8b22b8259fdc61cd35d410667 $
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
//	Sky rendering. The DOOM sky is a texture map like any
//	wall, wrapping around. 1024 columns equal 360 degrees.
//	The default sky map is 256 columns and repeats 4 times
//	on a 320 screen.
//
//
//-----------------------------------------------------------------------------

#include "r_sky.h"

#include "m_fixed.h"
#include "mud_includes.h"
#include "mud_profiling.h"
#include "r_client.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_state.h"
#include "res_texture.h"
#include "w_wad.h"


extern int32_t *texturewidthmask;
extern fixed_t  FocalLengthX;
extern fixed_t  freelookviewheight;

EXTERN_CVAR(cl_mouselook)
EXTERN_CVAR(r_skypalette)

//
// sky mapping
//
texhandle_t skyflatnum;
texhandle_t sky1texture, sky2texture;
fixed_t     skytexturemid;
fixed_t     skyscale;
int32_t     skystretch;
fixed_t     skyheight;
fixed_t     skyiscale;

int32_t sky1shift, sky2shift;
fixed_t sky1pos = 0, sky1speed = 0;
fixed_t sky2pos = 0, sky2speed = 0;

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
static angle_t xtoviewangle[MAXWIDTH + 1];

CVAR_FUNC_IMPL(r_stretchsky)
{
    R_InitSkyMap();
}

char SKYFLATNAME[8] = "F_SKY1";

static tallpost_t *skyposts[MAXWIDTH];

//
// R_InitXToViewAngle
//
// Now generate xtoviewangle for sky texture mapping.
// [RH] Do not generate viewangletox, because texture mapping is no
// longer done with trig, so it's not needed.
//
static void R_InitXToViewAngle()
{
    static int32_t last_viewwidth = -1;
    static fixed_t last_focx      = -1;

    if (viewwidth != last_viewwidth || FocalLengthX != last_focx)
    {
        if (centerx > 0)
        {
            const fixed_t hitan     = finetangent[FINEANGLES / 4 + CorrectFieldOfView / 2];
            const int32_t t         = std::min<int32_t>((FocalLengthX >> FRACBITS) + centerx, viewwidth);
            const fixed_t slopestep = hitan / centerx;
            const fixed_t dfocus    = FocalLengthX >> DBITS;

            for (int32_t i = centerx, slope = 0; i <= t; i++, slope += slopestep)
                xtoviewangle[i] = (angle_t) - (int32_t)tantoangle[slope >> DBITS];

            for (int32_t i = t + 1; i <= viewwidth; i++)
                xtoviewangle[i] = ANG270 + tantoangle[dfocus / (i - centerx)];

            for (int32_t i = 0; i < centerx; i++)
                xtoviewangle[i] = (angle_t)(-(int32_t)xtoviewangle[viewwidth - i - 1]);
        }
        else
        {
            memset(xtoviewangle, 0, sizeof(angle_t) * viewwidth + 1);
        }

        last_viewwidth = viewwidth;
        last_focx      = FocalLengthX;
    }
}

//
//
// R_InitSkyMap
//
// Called whenever the view size changes.
//
// [ML] 5/11/06 - Remove sky2 stuffs
// [ML] 3/16/10 - Bring it back!

void R_GenerateLookup(int32_t texnum, int32_t *const errors); // from r_data.cpp

void R_InitSkyMap()
{
    fixed_t fskyheight;

    // [SL] 2011-11-30 - Don't run if we don't know what sky texture to use
    if (gamestate != GS_LEVEL)
        return;

    if (sky2texture && texturemanager.getTexture(sky1texture)->getFracHeight() !=
                           texturemanager.getTexture(sky2texture)->getFracHeight())
    {
        Printf(PRINT_HIGH, "\x1f+Both sky textures must be the same height.\x1f-\n");
        sky2texture = sky1texture;
    }

    fskyheight = texturemanager.getTexture(sky1texture)->getFracHeight();

    if (fskyheight <= (128 << FRACBITS))
    {
        skytexturemid = 200 / 2 * FRACUNIT;
        skystretch    = (r_stretchsky == 1) || consoleplayer().spectator || (r_stretchsky == 2 && cl_mouselook);
    }
    else
    {
        skytexturemid = 199 << FRACBITS; // textureheight[sky1texture]-1;
        skystretch    = 0;
    }
    skyheight = fskyheight << skystretch;

    if (viewwidth && viewheight)
    {
        skyiscale = (200 * FRACUNIT) / ((freelookviewheight * viewwidth) / viewwidth);
        skyscale  = (((freelookviewheight * viewwidth) / viewwidth) << FRACBITS) / (200);

        skyiscale = FixedMul(skyiscale, FixedDiv(FieldOfView, 2048));
        skyscale  = FixedMul(skyscale, FixedDiv(2048, FieldOfView));
    }

    // The DOOM sky map is 256*128*4 maps.
    // The Heretic sky map is 256*200*4 maps.
    sky1shift = 22 + skystretch - 16;
    sky2shift = 22 + skystretch - 16;
    if (texturemanager.getTexture(sky1texture)->getWidthMask() >= 127)
        sky1shift -= skystretch;
    if (texturemanager.getTexture(sky2texture)->getWidthMask() >= 127)
        sky2shift -= skystretch;

    R_InitXToViewAngle();
}

//
// R_BlastSkyColumn
//
static inline void R_BlastSkyColumn(void (*drawfunc)(void))
{
    if (dcol.yl <= dcol.yh)
    {
        dcol.source      = dcol.post->data();
        dcol.texturefrac = dcol.texturemid + (dcol.yl - centery + 1) * dcol.iscale;
        drawfunc();
    }
}

inline void SkyColumnBlaster()
{
    R_BlastSkyColumn(colfunc);
}

//
// R_RenderSkyRange
//
// [RH] Can handle parallax skies. Note that the front sky is *not* masked in
// in the normal convention for patches, but uses color 0 as a transparent
// color.
// [ML] 5/11/06 - Removed sky2
//
void R_RenderSkyRange(visplane_t *pl)
{
    if (pl->minx > pl->maxx)
        return;

    MUD_ZoneScoped;

    int32_t     columnmethod = 2;
    texhandle_t skytex;
    fixed_t     front_offset = 0;
    angle_t     skyflip      = 0;

    if (pl->picnum == skyflatnum)
    {
        // use sky1
        skytex = sky1texture;
    }
    else if (pl->picnum == int32_t(PL_SKYFLAT))
    {
        // use sky2
        skytex = sky2texture;
    }
    else
    {
        // MBF's linedef-controlled skies
        int16_t       picnum = (pl->picnum & ~PL_SKYFLAT) - 1;
        const line_t *line   = &lines[picnum < numlines ? picnum : 0];

        // Sky transferred from first sidedef
        const side_t *side = *line->sidenum + sides;

        // Texture comes from upper texture of reference sidedef
        skytex = side->toptexture;

        // Horizontal offset is turned into an angle offset,
        // to allow sky rotation as well as careful positioning.
        // However, the offset is scaled very small, so that it
        // allows a long-period of sky rotation.
        front_offset = (-side->textureoffset) >> 6;

        // Vertical offset allows careful sky positioning.
        skytexturemid = side->rowoffset - 28 * FRACUNIT;

        // We sometimes flip the picture horizontally.
        //
        // Doom always flipped the picture, so we make it optional,
        // to make it easier to use the new feature, while to still
        // allow old sky textures to be used.
        skyflip = line->args[2] ? 0u : ~0u;
    }

    R_ResetDrawFuncs();

    const palette_t *pal = V_GetDefaultPalette();

    dcol.iscale        = skyiscale >> skystretch;
    dcol.texturemid    = skytexturemid;
    dcol.textureheight = texturemanager.getTexture(skytex)->getFracHeight();
    skyplane           = pl;

    // set up the appropriate colormap for the sky
    if (fixedlightlev)
    {
        dcol.colormap = shaderef_t(&pal->maps, fixedlightlev);
    }
    else if (fixedcolormap.isValid() && r_skypalette)
    {
        dcol.colormap = fixedcolormap;
    }
    else
    {
        // [SL] 2011-06-28 - Emulate vanilla Doom's handling of skies
        // when the player has the invulnerability powerup
        dcol.colormap = shaderef_t(&pal->maps, 0);
    }

    // determine which texture posts will be used for each screen
    // column in this range.
    for (int32_t x = pl->minx; x <= pl->maxx; x++)
    {
        int32_t colnum = ((((viewangle + xtoviewangle[x]) ^ skyflip) >> sky1shift) + front_offset) >> FRACBITS;
        skyposts[x]    = R_GetTextureColumn(skytex, colnum);
    }

    R_RenderColumnRange(pl->minx, pl->maxx, (int32_t *)pl->top, (int32_t *)pl->bottom, skyposts, SkyColumnBlaster,
                        false, columnmethod);

    R_ResetDrawFuncs();
}

VERSION_CONTROL(r_sky_cpp, "$Id: 6aecb96593367ac8b22b8259fdc61cd35d410667 $")
