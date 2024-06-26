// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 563de46526650199469a3e951bc7b05162f01a56 $
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
//	The Give commands (?)
//
//-----------------------------------------------------------------------------

// [AM] Seems like this header should be "p_interaction.h"...

#pragma once

#include "d_player.h"

#define BONUSADD 6

bool         P_GiveFrags(player_t *player, int32_t num);
bool         P_GiveKills(player_t *player, int32_t num);
bool         P_GiveDeaths(player_t *player, int32_t num);
bool         P_GiveMonsterDamage(player_t *player, int32_t num);
bool         P_GiveTeamPoints(player_t *player, int32_t num);
bool         P_GiveLives(player_t *player, int32_t num);
int32_t      P_GetFragCount(const player_t *player);
int32_t      P_GetPointCount(const player_t *player);
int32_t      P_GetDeathCount(const player_t *player);
ItemEquipVal P_GiveAmmo(player_t *player, ammotype_t ammotype, float num);
ItemEquipVal P_GiveWeapon(player_t *player, weapontype_t weapon, bool dropped);
ItemEquipVal P_GiveArmor(player_t *player, int32_t armortype);
ItemEquipVal P_GiveCard(player_t *player, card_t card);
ItemEquipVal P_GivePower(player_t *player, int32_t /*powertype_t*/ power);
void         P_KillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill);
void         P_HealMobj(AActor *mo, int32_t num);
