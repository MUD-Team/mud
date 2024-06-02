// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: fc2f0d4a3a5606a0c277b3c12ba871f2dbaad6d7 $
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
//  [Blair] Define the Doom (Doom in Doom) format doom map spec.
//  Includes sector specials, linedef types, line crossing.
//  "Attempts" to be Boom compatible, hence the name.
//
//-----------------------------------------------------------------------------

#pragma once

#include "d_player.h"
#include "map_defs.h"

void OnChangedSwitchTexture(line_t *line, int32_t useAgain);
void G_SecretExitLevel(int32_t position, int32_t drawscores);
void P_DamageMobj(AActor *target, AActor *inflictor, AActor *source, int32_t damage, int32_t mod, int32_t flags);
bool P_CrossCompatibleSpecialLine(line_t *line, int32_t side, AActor *thing, bool bossaction);
const uint32_t P_TranslateCompatibleLineFlags(const uint32_t flags, const bool reserved);
void           P_ApplyGeneralizedSectorDamage(player_t *player, int32_t bits);
void           P_CollectSecretBoom(sector_t *sector, player_t *player);
void           P_PlayerInCompatibleSector(player_t *player);
bool           P_ActorInCompatibleSector(AActor *actor);
bool           P_UseCompatibleSpecialLine(AActor *thing, line_t *line, int32_t side, bool bossaction);
bool           P_ShootCompatibleSpecialLine(AActor *thing, line_t *line);
bool           EV_DoGenDoor(line_t *line);
bool           EV_DoGenFloor(line_t *line);
bool           EV_DoGenCeiling(line_t *line);
bool           EV_DoGenLift(line_t *line);
bool           EV_DoGenStairs(line_t *line);
bool           P_CanUnlockGenDoor(line_t *line, player_t *player);
bool           EV_DoGenLockedDoor(line_t *line);
bool           EV_DoGenCrusher(line_t *line);
int32_t        EV_DoDonut(line_t *line);
void           P_CollectSecretVanilla(sector_t *sector, player_t *player);
void           EV_StartLightStrobing(int32_t tag, int32_t upper, int32_t lower, int32_t utics, int32_t ltics);
void           EV_StartLightStrobing(int32_t tag, int32_t utics, int32_t ltics);
void           P_SetTransferHeightBlends(side_t *sd, const mapsidedef_t *msd);
void           SetTextureNoErr(texhandle_t *texture, uint32_t *color, char *name);
void           P_AddSectorSecret(sector_t *sector);
void           P_SpawnLightFlash(sector_t *sector);
void           P_SpawnStrobeFlash(sector_t *sector, int32_t utics, int32_t ltics, bool inSync);
void           P_SpawnFireFlicker(sector_t *sector);
AActor        *P_GetPushThing(int32_t);
void           P_PostProcessCompatibleLinedefSpecial(line_t *line);
bool           P_IsTeleportLine(const int16_t special);