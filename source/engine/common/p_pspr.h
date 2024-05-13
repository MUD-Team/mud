// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ed6594d9cad51747617853dce995e5c44d9215dd $
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
//  Sprite animation.
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomdef.h"
#include "info.h"
#include "m_fixed.h"
#include "tables.h"

//
// Frame flags:
// handles maximum brightness (torches, muzzle flare, light sources)
//
#define FF_FULLBRIGHT 0x8000 // flag in thing->frame
#define FF_FRAMEMASK  0x7fff

//
// Overlay psprites are scaled shapes
// drawn directly on the view screen,
// coordinates are given for a 320*200 view screen.
//
typedef enum
{
    ps_weapon,
    ps_flash,
    NUMPSPRITES

} psprnum_t;

void A_ForceWeaponFire(AActor *mo, weapontype_t weapon, int32_t tic);

inline FArchive &operator<<(FArchive &arc, psprnum_t i)
{
    return arc << (uint8_t)i;
}
inline FArchive &operator>>(FArchive &arc, psprnum_t &out)
{
    uint8_t in;
    arc >> in;
    out = (psprnum_t)in;
    return arc;
}

typedef struct pspdef_s
{
    state_t *state; // a NULL state means not active
    int32_t      tics;

    fixed_t sx;
    fixed_t sy;
} pspdef_t;

FArchive &operator<<(FArchive &, pspdef_t &);
FArchive &operator>>(FArchive &, pspdef_t &);
