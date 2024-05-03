// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 828fea1709808f2cd0c48177d75ab1bb5004f4c0 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//   Duh.
//
//-----------------------------------------------------------------------------

#pragma once

#include "d_event.h"
#include "d_player.h"

//
// GAME
//
void G_DeathMatchSpawnPlayer(player_t &player);
void G_DoReborn(player_t &player);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_LoadGame(char *name);

void G_DoLoadGame(void);

// Called by M_Responder.
void G_BuildSaveName(std::string &name, int slot);
void G_SaveGame(int slot, char *description);

void G_WorldDone(void);

void G_Ticker(void);
BOOL G_Responder(event_t *ev);

void G_ScreenShot(const char *filename);

void G_PlayerFinishLevel(player_t &player);

void G_AddViewAngle(int yaw);
void G_AddViewPitch(int pitch);
bool G_ShouldIgnoreMouseInput();

extern int mapchange;
