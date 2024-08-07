//----------------------------------------------------------------------------
//  DDF Collections
//----------------------------------------------------------------------------
//
//  Copyright (c) 2022-2024 The EDGE Team.
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

#pragma once

#include <stdint.h>

#include <string>

enum DDFType
{
    kDDFTypeUnknown = -1,
    kDDFTypeAnim    = 0,
    kDDFTypeAttack,
    kDDFTypeColourMap,
    kDDFTypeFlat,
    kDDFTypeFont,
    kDDFTypeGame,
    kDDFTypeImage,
    kDDFTypeLanguage,
    kDDFTypeLevel,
    kDDFTypeLine,
    kDDFTypeMovie,
    kDDFTypePlaylist,
    kDDFTypeSFX,
    kDDFTypeSector,
    kDDFTypeStyle,
    kDDFTypeSwitch,
    kDDFTypeThing,
    kDDFTypeWeapon,
    kTotalDDFTypes
};

struct DDFFile
{
    DDFType     type;
    std::string data;
    uint64_t    data_hash;
};

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
