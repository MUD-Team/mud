// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d497fdc440bba6e6655cfffadd4712f02f01fa6c $
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
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

#include "i_video.h"
#include "mud_includes.h"
#include "v_video.h"

// Functions for v_video.cpp support

void r_dimpatchD_c(IRenderSurface *surface, argb_t color, int32_t alpha, int32_t x1, int32_t y1, int32_t w, int32_t h)
{
    const int32_t surface_pitch_pixels = surface->getPitchInPixels();

    argb_t *line = (argb_t *)surface->getBuffer() + y1 * surface_pitch_pixels;

    for (int32_t y = y1; y < y1 + h; y++)
    {
        for (int32_t x = x1; x < x1 + w; x++)
            line[x] = alphablend1a(line[x], color, alpha);

        line += surface_pitch_pixels;
    }
}

VERSION_CONTROL(r_drawt_cpp, "$Id: d497fdc440bba6e6655cfffadd4712f02f01fa6c $")
