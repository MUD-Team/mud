// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: eaa8e9d9e8d0f2cda24d398dda19c9fe1f1b227e $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
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
#include "r_defs.h"

void OnChangedSwitchTexture(line_t *line, int useAgain);
void SV_OnActivatedLine(line_t *line, AActor *mo, const int side, const LineActivationType activationType,
                        const bool bossaction);
bool EV_DoZDoomDoor(DDoor::EVlDoor type, line_t *line, AActor *mo, uint8_t tag, uint8_t speed_byte, int topwait,
                    zdoom_lock_t lock, uint8_t lightTag, bool boomgen, int topcountdown);
bool EV_DoZDoomPillar(DPillar::EPillar type, line_t *line, int tag, fixed_t speed, fixed_t floordist,
                      fixed_t ceilingdist, int crush, bool hexencrush);
bool EV_DoZDoomElevator(line_t *line, DElevator::EElevator type, fixed_t speed, fixed_t height, int tag);
bool EV_DoZDoomDonut(int tag, line_t *line, fixed_t pillarspeed, fixed_t slimespeed);
bool EV_ZDoomCeilingCrushStop(int tag, bool remove);
void P_HealMobj(AActor *mo, int num);
void EV_LightSetMinNeighbor(int tag);
void EV_LightSetMaxNeighbor(int tag);
void P_ApplySectorFriction(int tag, int value, bool use_thinker);
void P_SetupSectorDamage(sector_t *sector, int16_t amount, uint8_t interval, uint8_t leakrate, uint32_t flags);
void P_AddSectorSecret(sector_t *sector);
void P_SpawnLightFlash(sector_t *sector);
void P_SpawnStrobeFlash(sector_t *sector, int utics, int ltics, bool inSync);
void P_SpawnFireFlicker(sector_t *sector);
bool P_CrossZDoomSpecialLine(line_t *line, int side, AActor *thing, bool bossaction);
bool P_ActivateZDoomLine(line_t *line, AActor *mo, int side, uint32_t activationType);
void P_CollectSecretZDoom(sector_t *sector, player_t *player);
bool P_TestActivateZDoomLine(line_t *line, AActor *mo, int side, uint32_t activationType);
bool P_ExecuteZDoomLineSpecial(int special, int16_t *args, line_t *line, int side, AActor *mo);
bool EV_DoZDoomFloor(DFloor::EFloor floortype, line_t *line, int tag, fixed_t speed, fixed_t height, int crush,
                     int change, bool hexencrush, bool hereticlower);
bool EV_DoZDoomCeiling(DCeiling::ECeiling type, line_t *line, uint8_t tag, fixed_t speed, fixed_t speed2, fixed_t height,
                       int crush, uint8_t silent, int change, crushmode_e crushmode);
void P_SetTransferHeightBlends(side_t *sd, const mapsidedef_t *msd);
void SetTextureNoErr(texhandle_t *texture, uint32_t *color, char *name);

int                      P_Random(AActor *actor);
const LineActivationType P_LineActivationTypeForSPACFlag(const uint32_t activationType);
void                     P_SpawnPhasedLight(sector_t *sector, int base, int index);
void                     P_SpawnLightSequence(sector_t *sector);
AActor                  *P_GetPushThing(int s);
void                     P_PostProcessZDoomLinedefSpecial(line_t *line);
