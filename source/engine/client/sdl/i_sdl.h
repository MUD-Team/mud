// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: c349d27163a28ee63ed0249d46fe5f9e7f3a20b4 $
//
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
// SDL header wrapper
//
//-----------------------------------------------------------------------------

#pragma once

#include <SDL.h>

#if (SDL_MAJOR_VERSION == 2)
#define SDL20
#if (SDL_MINOR_VERSION > 0 || SDL_PATCHLEVEL >= 16)
#define SDL2016
#endif
#endif

#if (SDL_VERSION > SDL_VERSIONNUM(1, 2, 7))
#include "SDL_cpuinfo.h"
#endif
