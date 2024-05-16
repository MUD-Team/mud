// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 686011b85d769015f0aa65907035336a1d6e9867 $
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
//      Put all global state variables here.
//
//-----------------------------------------------------------------------------

#include "d_main.h"
#include "g_mapinfo.h"
#include "gstrings.h"
#include "i_system.h"
#include "mud_includes.h"
#include "p_acs.h"

// Game Mode - identify IWAD
GameMode_t    gamemode    = undetermined;
GameMission_t gamemission = none;

// Language.
CVAR_FUNC_IMPL(language)
{
    SetLanguageIDs();
    if (level.behavior != NULL)
    {
        level.behavior->PrepLocale(LanguageIDs[0], LanguageIDs[1], LanguageIDs[2], LanguageIDs[3]);
    }

    // Reload LANGUAGE strings.
    ::GStrings.loadStrings(false);
}

// Set if homebrew PWAD stuff has been added.
bool modifiedgame;

// Miscellaneous info that used to be constant
// Formerly in d_dehacked, subject to change - Dasho
struct DehInfo deh = {
    100, // .StartHealth
    50,  // .StartBullets
    100, // .MaxHealth
    200, // .MaxArmor
    1,   // .GreenAC
    2,   // .BlueAC
    200, // .MaxSoulsphere
    100, // .SoulsphereHealth
    200, // .MegasphereHealth
    100, // .GodHealth
    200, // .FAArmor
    2,   // .FAAC
    200, // .KFAArmor
    2,   // .KFAAC
    40,  // .BFGCells (No longer used)
    0,   // .Infight
};

VERSION_CONTROL(doomstat_cpp, "$Id: 686011b85d769015f0aa65907035336a1d6e9867 $")
