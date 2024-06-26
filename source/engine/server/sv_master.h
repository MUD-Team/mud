// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: f4050b49a60c65dc159d9a02b0d23df737777418 $
//
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
//	SV_MASTER
//
//-----------------------------------------------------------------------------

#pragma once

#include "c_console.h"
#include "d_player.h"
#include "p_local.h"
#include "physfs.h"
#include "sv_main.h"

bool SV_AddMaster(const char *masterip);
void SV_InitMasters();
bool SV_AddMaster(const char *masterip);
void SV_ListMasters();
bool SV_RemoveMaster(const char *masterip);
void SV_UpdateMasterServers(void);
void SV_UpdateMaster(void);
void SV_ArchiveMasters(PHYSFS_File *fp);
