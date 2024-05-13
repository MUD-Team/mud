// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
//
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
//  UIPublic header.
//
//-----------------------------------------------------------------------------

#pragma once

#include "../sdl/i_sdl.h"

class IRenderSurface;
class ISDL20Window;
class PixelFormat;

bool UI_Initialize();
void UI_Shutdown();

bool UI_RenderInitialized();

void UI_SetMode(uint16_t width, uint16_t height, const PixelFormat *format, ISDL20Window *window, bool vsync,
                const char *render_scale_quality = NULL);

void UI_PostEvent(SDL_Event &ev);

void UI_LoadCore();

void UI_OnResize();
