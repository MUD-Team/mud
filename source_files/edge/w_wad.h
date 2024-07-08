//----------------------------------------------------------------------------
//  EDGE WAD Support Code
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

#include <vector>

#include "dm_defs.h"
#include "epi_file.h"

int CheckLumpNumberForName(const char *name);

int CheckXGLLumpNumberForName(const char *name);
int CheckMapLumpNumberForName(const char *name);

int GetLumpLength(int lump);

uint8_t *LoadLumpIntoMemory(int lump, int *length = nullptr);

bool        IsLumpIndexValid(int lump);
bool        VerifyLump(int lump, const char *name);

bool IsFileInAddon(const char *name);

bool IsFileAnywhere(const char *name);

void BuildXGLNodes(void);

int GetKindForLump(int lump);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
