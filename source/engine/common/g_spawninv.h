// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 3bb926844ba6045b01f0b1afa9c7a68050fc512f $
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
//   Spawn inventory.
//
//-----------------------------------------------------------------------------

#pragma once

#include "d_player.h"

void G_SetupSpawnInventory();
void G_GiveSpawnInventory(player_t& player);
void G_GiveBetweenInventory(player_t& player);
