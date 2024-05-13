// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 304151525388c3810025f7b3b09512ee597010aa $
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
//	Multiplayer properties (?)
//
//-----------------------------------------------------------------------------

#pragma once

#include "c_cvars.h"
#include "doomdef.h"
#include "doomtype.h"
#include "teaminfo.h"

#define MAXPLAYERNAME 15

enum gender_t
{
    GENDER_MALE,
    GENDER_FEMALE,
    GENDER_NEUTER,

    NUMGENDER
};

enum colorpreset_t // Acts 19 quiz the order must match m_menu.cpp.
{
    COLOR_CUSTOM,
    COLOR_BLUE,
    COLOR_INDIGO,
    COLOR_GREEN,
    COLOR_BROWN,
    COLOR_RED,
    COLOR_GOLD,
    COLOR_JUNGLEGREEN,
    COLOR_PURPLE,
    COLOR_WHITE,
    COLOR_BLACK,

    NUMCOLOR
};

enum weaponswitch_t
{
    WPSW_NEVER,
    WPSW_ALWAYS,
    WPSW_PWO,
    WPSW_PWO_ALT, // PWO but never switch if holding +attack

    WPSW_NUMTYPES
};

struct UserInfo
{
    std::string    netname;
    team_t         team; // [Toke - Teams]
    fixed_t        aimdist;
    bool           predict_weapons;
    uint8_t           color[4];
    gender_t       gender;
    weaponswitch_t switchweapon;
    uint8_t           weapon_prefs[NUMWEAPONS];

    static const uint8_t weapon_prefs_default[NUMWEAPONS];

    UserInfo() : team(TEAM_NONE), aimdist(0), predict_weapons(true), gender(GENDER_MALE), switchweapon(WPSW_ALWAYS)
    {
        // default doom weapon ordering when player runs out of ammo
        memcpy(weapon_prefs, UserInfo::weapon_prefs_default, sizeof(weapon_prefs));
        memset(color, 0, 4);
    }
};

FArchive &operator<<(FArchive &arc, UserInfo &info);
FArchive &operator>>(FArchive &arc, UserInfo &info);

void D_SetupUserInfo(void);

void D_UserInfoChanged(cvar_t *info);

void D_SendServerInfoChange(const cvar_t *cvar, const char *value);
void D_DoServerInfoChange(uint8_t **stream);

void D_WriteUserInfoStrings(int32_t player, uint8_t **stream, bool compact = false);
void D_ReadUserInfoStrings(int32_t player, uint8_t **stream, bool update);
