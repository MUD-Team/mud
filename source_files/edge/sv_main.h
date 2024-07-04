//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Main defs)
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

#pragma once

#include "dm_defs.h"
#include "e_player.h"
#include "p_local.h"

class MapDefinition;

const char *SaveSlotName(int slot);
const char *SaveMapName(const MapDefinition *map);

std::string SaveFilename(const char *slot_name, const char *map_name);

void SaveClearSlot(const char *slot_name);
void SaveCopySlot(const char *src_name, const char *dest_name);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
