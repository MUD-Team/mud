// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ede3a8513c411cfa4937430b3c7be22787295262 $
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
//		DOOM Network game communication and protocol,
//		all OS independent parts.
//
//-----------------------------------------------------------------------------

#include "cl_main.h"
#include "d_netinf.h"
#include "g_game.h"
#include "gi.h"
#include "i_input.h"
#include "i_system.h"
#include "m_argv.h"
#include "mud_includes.h"
#include "p_local.h"

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run

int32_t lastnettic;
int32_t skiptics;

bool step_mode = false;

void D_ProcessEvents(void);
void G_BuildTiccmd(ticcmd_t *cmd);

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
void NetUpdate(void)
{
    I_GetEvents();
    D_ProcessEvents();
    G_BuildTiccmd(&consoleplayer().netcmds[gametic % BACKUPTICS]);
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//
void D_CheckNetGame(void)
{
    CL_InitNetwork();

    D_SetupUserInfo();

    step_mode = ((Args.CheckParm("-stepmode")) != 0);
}

VERSION_CONTROL(d_net_cpp, "$Id: ede3a8513c411cfa4937430b3c7be22787295262 $")
