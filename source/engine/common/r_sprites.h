// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 15bae3274a6c7467fb15aa241e37e93acf5607a9 $
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
//	Loading sprites, skins.
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_defs.h"

extern int32_t MaxVisSprites;

extern vissprite_t *vissprites;

extern spritedef_t *sprites;
extern int32_t      numsprites;

#define MAX_SPRITE_FRAMES 29 // [RH] Macro-ized as in BOOM.

// variables used to look up
//	and range check thing_t sprites patches
extern spriteframe_t sprtemp[MAX_SPRITE_FRAMES];
extern int32_t       maxframe;

extern vissprite_t *lastvissprite;

void R_CacheSprite(spritedef_t *sprite);
void R_InitSprites();
void R_InstallSpriteTex(const texhandle_t tex_id, uint32_t frame, uint32_t rot, bool flipped);
void R_InstallSprite(const char *name, int32_t num);
