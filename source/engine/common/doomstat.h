// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: c19da900aee5a512d5f6691d50fac6ccb31efe95 $
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
//   All the global variables that store the internal state.
//   Theoretically speaking, the internal state of the engine
//    should be found by looking at the variables collected
//    here, and every relevant module will have to include
//    this header file.
//   In practice, things are a bit messy.
//
//-----------------------------------------------------------------------------

#pragma once

#include "c_cvars.h"
#include "d_net.h"
#include "d_netinf.h"
#include "doomdata.h"
#include "doomtype.h"
#include "g_level.h"
#include "res_texture.h"

// ------------------------
// Command line parameters.
//
extern bool devparm; // DEBUG: launched with -devparm

// -----------------------------------------------------
// Game Mode - identify IWAD as shareware, retail etc.
//
extern GameMode_t    gamemode;
extern GameMission_t gamemission;

// Set if homebrew PWAD stuff has been added.
extern bool modifiedgame;

// -------------------------------------------
// Selected skill type, map etc.
//

extern char startmap[8]; // [RH] Actual map name now

extern bool autostart;

// Selected by user.
EXTERN_CVAR(sv_skill)

// Bot game? Like netgame, but doesn't involve network communication.
extern bool multiplayer;

extern bool network_game;

// Game mode
EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_maxplayers)

#define GM_COOP   0.0f
#define GM_DM     1.0f
#define GM_TEAMDM 2.0f
// #define GM_CTF    3.0f

#define FPS_NONE    0
#define FPS_FULL    1
#define FPS_COUNTER 2

// -------------------------
// Internal parameters for sound rendering.
// These have been taken from the DOS version,
//	but are not (yet) supported with Linux
//	(e.g. no sound volume adjustment with menu.

// These are not used, but should be (menu).
// From m_menu.c:
//	Sound FX volume has default, 0 - 15
//	Music volume has default, 0 - 15
// These are multiplied by 8.

// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//	status bar explicitely.
extern bool statusbaractive;

extern bool paused; // Game Pause?

extern bool viewactive;

extern bool nodrawers;
extern bool noblit;

extern level_locals_t level;

// --------------------------------------
// DEMO playback/recording related stuff.
// No demo, there is a human player in charge?
// Disable save/end game?
extern bool usergame;

extern gamestate_t gamestate;

//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//	according to user inputs. Partly load from
//	WAD, partly set at startup time.

extern int32_t gametic;

// Player spawn spots for deathmatch.
extern std::vector<mapthing2_t> DeathMatchStarts;

// Player spawn spots.
#define MAXPLAYERSTARTS 64
extern std::vector<mapthing2_t> playerstarts;
extern std::vector<mapthing2_t> voodoostarts;

// ----------------------------------------------

// Intermission stats.
// Parameters for world map / intermission.
extern struct wbstartstruct_s wminfo;

// LUT of ammunition limits for each kind.
// This doubles with BackPack powerup item.
extern int32_t maxammo[NUMAMMO];

//-----------------------------------------
// Internal parameters, used for engine.
//

// if true, load all graphics at level load
extern bool precache;

EXTERN_CVAR(mouse_sensitivity) // removeme // ?

// Needed to store the number of the dummy sky flat.
// Used for rendering,
//	as well as tracking projectiles etc.
extern texhandle_t skyflatnum;

// ---- [RH] ----
EXTERN_CVAR(developer) // removeme

// [RH] Miscellaneous info for DeHackEd support
struct DehInfo
{
    int32_t StartHealth;
    int32_t StartBullets;
    int32_t MaxHealth;
    int32_t MaxArmor;
    int32_t GreenAC;
    int32_t BlueAC;
    int32_t MaxSoulsphere;
    int32_t SoulsphereHealth;
    int32_t MegasphereHealth;
    int32_t GodHealth;
    int32_t FAArmor;
    int32_t FAAC;
    int32_t KFAArmor;
    int32_t KFAAC;
    int32_t BFGCells;
    int32_t Infight;
};
extern struct DehInfo deh;
