// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: eaa8e9d9e8d0f2cda24d398dda19c9fe1f1b227e $
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
//  [Blair] Define the ZDoom (Doom in Hexen) format doom map spec.
//  Includes sector specials, linedef types, line crossing.
//
//-----------------------------------------------------------------------------

#pragma once

#include "actor.h"
#include "doomtype.h"
#include "m_fixed.h"
#include "p_spec.h"
#include "map_defs.h"

void OnChangedSwitchTexture(line_t *line, int32_t useAgain);
void SV_OnActivatedLine(line_t *line, AActor *mo, const int32_t side, const LineActivationType activationType,
                        const bool bossaction);
bool EV_DoZDoomDoor(DDoor::EVlDoor type, line_t *line, AActor *mo, uint8_t tag, uint8_t speed_byte, int32_t topwait,
                    zdoom_lock_t lock, uint8_t lightTag, bool boomgen, int32_t topcountdown);
bool EV_DoZDoomPillar(DPillar::EPillar type, line_t *line, int32_t tag, fixed_t speed, fixed_t floordist,
                      fixed_t ceilingdist, int32_t crush, bool hexencrush);
bool EV_DoZDoomElevator(line_t *line, DElevator::EElevator type, fixed_t speed, fixed_t height, int32_t tag);
bool EV_DoZDoomDonut(int32_t tag, line_t *line, fixed_t pillarspeed, fixed_t slimespeed);
bool EV_ZDoomCeilingCrushStop(int32_t tag, bool remove);
void P_HealMobj(AActor *mo, int32_t num);
void EV_LightSetMinNeighbor(int32_t tag);
void EV_LightSetMaxNeighbor(int32_t tag);
void P_ApplySectorFriction(int32_t tag, int32_t value, bool use_thinker);
void P_SetupSectorDamage(sector_t *sector, int16_t amount, uint8_t interval, uint8_t leakrate, uint32_t flags);
void P_AddSectorSecret(sector_t *sector);
void P_SpawnLightFlash(sector_t *sector);
void P_SpawnStrobeFlash(sector_t *sector, int32_t utics, int32_t ltics, bool inSync);
void P_SpawnFireFlicker(sector_t *sector);
bool P_CrossZDoomSpecialLine(line_t *line, int32_t side, AActor *thing, bool bossaction);
bool P_ActivateZDoomLine(line_t *line, AActor *mo, int32_t side, uint32_t activationType);
void P_CollectSecretZDoom(sector_t *sector, player_t *player);
bool P_TestActivateZDoomLine(line_t *line, AActor *mo, int32_t side, uint32_t activationType);
bool P_ExecuteZDoomLineSpecial(int32_t special, int16_t *args, line_t *line, int32_t side, AActor *mo);
bool EV_DoZDoomFloor(DFloor::EFloor floortype, line_t *line, int32_t tag, fixed_t speed, fixed_t height, int32_t crush,
                     int32_t change, bool hexencrush, bool hereticlower);
bool EV_DoZDoomCeiling(DCeiling::ECeiling type, line_t *line, uint8_t tag, fixed_t speed, fixed_t speed2,
                       fixed_t height, int32_t crush, uint8_t silent, int32_t change, crushmode_e crushmode);
void P_SetTransferHeightBlends(side_t *sd, const mapsidedef_t *msd);
void SetTextureNoErr(texhandle_t *texture, uint32_t *color, char *name);

int32_t                  P_Random(AActor *actor);
const LineActivationType P_LineActivationTypeForSPACFlag(const uint32_t activationType);
void                     P_SpawnPhasedLight(sector_t *sector, int32_t base, int32_t index);
void                     P_SpawnLightSequence(sector_t *sector);
AActor                  *P_GetPushThing(int32_t s);
void                     P_PostProcessZDoomLinedefSpecial(line_t *line);
