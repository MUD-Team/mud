// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 0143c3dc5cc13ff19fe30d2fadbc1002c5f71b51 $
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
//	 Clientside function stubs
//
//-----------------------------------------------------------------------------

#include <stdarg.h>

#include "actor.h"
#include "cmdlib.h"
#include "d_player.h"
#include "mud_includes.h"

// Unnatural Level Progression.  True if we've used 'map' or another command
// to switch to a specific map out of order, otherwise false.
bool unnatural_level_progression;

FORMAT_PRINTF(2, 3)
void STACK_ARGS SV_BroadcastPrintf(int32_t printlevel, const char *format, ...)
{
    if (!serverside)
        return;

    // Local game, print the message normally.
    std::string str;
    va_list     va;
    va_start(va, format);
    VStrFormat(str, format, va);
    va_end(va);

    Printf(printlevel, "%s", str.c_str());
}

FORMAT_PRINTF(1, 2)
void STACK_ARGS SV_BroadcastPrintf(const char *format, ...)
{
    if (!serverside)
        return;

    // Local game, print the message normally.
    std::string str;
    va_list     va;
    va_start(va, format);
    VStrFormat(str, format, va);
    va_end(va);

    Printf(PRINT_HIGH, "%s", str.c_str());
}

void D_SendServerInfoChange(const cvar_t *cvar, const char *value)
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

void SV_SpawnMobj(AActor *mobj)
{
}
void SV_TouchSpecial(AActor *special, player_t *player)
{
}
ItemEquipVal SV_FlagTouch(player_t &player, team_t f, bool firstgrab)
{
    return IEV_NotEquipped;
}
void SV_SocketTouch(player_t &player, team_t f)
{
}
void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill)
{
}
void SV_SendDamagePlayer(player_t *player, AActor *inflictor, int32_t healthDamage, int32_t armorDamage)
{
}
void SV_SendDamageMobj(AActor *target, int32_t pain)
{
}
void SV_UpdateFrags(player_t &player)
{
}
void SV_ActorTarget(AActor *actor)
{
}
void SV_SendDestroyActor(AActor *mo)
{
}
void SV_ExplodeMissile(AActor *mo)
{
}
void SV_SendPlayerInfo(player_t &player)
{
}
void SV_PreservePlayer(player_t &player)
{
}
void SV_BroadcastSector(int32_t sectornum)
{
}
void SV_UpdateMobj(AActor *mo)
{
}
void SV_UpdateMobjState(AActor *mo)
{
}

void CTF_RememberFlagPos(mapthing2_t *mthing)
{
}
void CTF_SpawnFlag(team_t f)
{
}
bool SV_AwarenessUpdate(player_t &pl, AActor *mo)
{
    return true;
}
void SV_SendPackets(void)
{
}
void SV_SendExecuteLineSpecial(uint8_t special, line_t *line, AActor *activator, int32_t arg0, int32_t arg1, int32_t arg2, int32_t arg3,
                               int32_t arg4)
{
}

void SV_UpdateMonsterRespawnCount()
{
}
void SV_Sound(AActor *mo, uint8_t channel, const char *name, uint8_t attenuation)
{
}

CVAR_FUNC_IMPL(sv_sharekeys)
{
}

VERSION_CONTROL(cl_stubs_cpp, "$Id: 0143c3dc5cc13ff19fe30d2fadbc1002c5f71b51 $")
