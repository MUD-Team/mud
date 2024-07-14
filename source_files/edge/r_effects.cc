//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Screen Effects)
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

#include "dm_state.h"
#include "e_player.h"
#include "epi.h"
#include "i_defs_gl.h"
#include "m_misc.h"
#include "r_colormap.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_texgl.h"


int render_view_extra_light = 0;

float render_view_red_multiplier = 1;
float render_view_green_multiplier = 1;
float render_view_blue_multiplier = 1;

const Colormap *render_view_effect_colormap = nullptr;


EDGE_DEFINE_CONSOLE_VARIABLE(power_fade_out, "1", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(debug_fullbright, "0", kConsoleVariableFlagCheat)

//----------------------------------------------------------------------------
//  FUZZY Emulation
//----------------------------------------------------------------------------

const Image *fuzz_image;

static float fuzz_y_offset;

void FuzzUpdate(void)
{
    if (!fuzz_image)
    {
        fuzz_image = ImageLookup("FUZZ_MAP", kImageNamespaceTexture, kImageLookupExact | kImageLookupNull);
        if (!fuzz_image)
            FatalError("Cannot find essential image: FUZZ_MAP\n");
    }

    fuzz_y_offset = ((render_frame_count * 3) & 1023) / 256.0;
}

void FuzzAdjust(HMM_Vec2 *tc, MapObject *mo)
{
    tc->X += fmod(mo->x / 520.0, 1.0);
    tc->Y += fmod(mo->y / 520.0, 1.0) + fuzz_y_offset;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
