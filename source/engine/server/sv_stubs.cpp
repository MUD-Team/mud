// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 001aed4792381e6fc731d141dab79fca36ef59c5 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	 Serverside function stubs
//
//-----------------------------------------------------------------------------

#include "d_player.h"
#include "mud_includes.h"
#include "v_palette.h"

bool menuactive;

void R_ExitLevel()
{
}
void D_SetupUserInfo(void)
{
}
void D_UserInfoChanged(cvar_t *cvar)
{
}
void D_DoServerInfoChange(uint8_t **stream)
{
}
void D_WriteUserInfoStrings(int32_t i, uint8_t **stream, bool compact)
{
}
void D_ReadUserInfoStrings(int32_t i, uint8_t **stream, bool update)
{
}

argb_t V_GetColorFromString(const std::string &str)
{
    return 0;
}

void PickupMessage(AActor *toucher, const char *message)
{
}
void WeaponPickupMessage(AActor *toucher, weapontype_t &Weapon)
{
}

void RefreshPalettes(void)
{
}

void V_RefreshColormaps()
{
}

void V_DynamicLightsCleanup()
{

}

void V_DynamicPaletteAddImage(uint8_t *raw_rgba_pixels, int32_t width, int32_t height)
{
    
}

CVAR_FUNC_IMPL(sv_allowwidescreen)
{
}

VERSION_CONTROL(sv_stubs_cpp, "$Id: 001aed4792381e6fc731d141dab79fca36ef59c5 $")
