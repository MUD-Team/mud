// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 508e0fbd0bbeb03854c5ba73aa616acf860fc1df $
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
//	DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//	plus functions to determine game mode (shareware, registered),
//	parse command line parameters, configure game parameters (turbo),
//	and call the startup functions.
//
//-----------------------------------------------------------------------------

#include "d_main.h"

#include <math.h>
#include <stdlib.h>

#include "win32inc.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif
#include <time.h>
#ifdef UNIX
#include <unistd.h>
#endif

#include <algorithm>

#include "c_dispatch.h"
#include "g_game.h"
#include "g_mapinfo.h"
#include "gi.h"
#include "gstrings.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "m_random.h"
#include "minilzo.h"
#include "mud_includes.h"
#include "p_setup.h"
#include "r_common.h"
#include "s_sound.h"
#include "sv_main.h"
#include "v_video.h"
#include "w_ident.h"
#include "w_wad.h"
#include "z_zone.h"

EXTERN_CVAR(sv_timelimit)
EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_monstersrespawn)
EXTERN_CVAR(sv_fastmonsters)

extern size_t got_heapsize;

void C_DoCommand(const char *cmd, uint32_t key = 0);

#ifdef UNIX
void daemon_init();
#endif

void D_DoomLoop(void);

extern gameinfo_t CommercialGameInfo;

extern bool          gameisdead;
extern DThinker      ThinkerCap;
extern dyncolormap_t NormalLight;

bool    devparm; // started game with -devparm
char    startmap[8];
event_t events[MAXEVENTS];

std::string LOG_FILE;

//
// D_DoomLoop
//
void D_DoomLoop(void)
{
    while (1)
    {
        D_RunTics(SV_RunTics, SV_DisplayTics);
    }
}

//
// D_Init
//
// Called to initialize subsystems when loading a new set of WAD resource
// files.
//
void D_Init()
{
    argb_t::setChannels(3, 2, 1, 0);
    // only print init messages during startup, not when changing WADs
    static bool first_time = true;

    SetLanguageIDs();

    M_ClearRandom();

    // [AM] Init rand() PRNG, needed for non-deterministic maplist shuffling.
    srand(time(NULL));

    // start the Zone memory manager
    Z_Init();
    if (first_time)
        Printf("Z_Init: Using native allocator with OZone bookkeeping.\n");

    // Load palette and set up colormaps
    V_InitPalette();

    Res_InitTextureManager();

    // [RH] Initialize localizable strings.
    ::GStrings.loadStrings(false);

    Table_InitTanToAngle();

    G_ParseMapInfo();
    G_ParseMusInfo();
    S_ParseSndInfo();

    if (first_time)
        Printf(PRINT_HIGH, "P_Init: Init Playloop state.\n");

    P_Init();

    first_time = false;
}

//
// D_Shutdown
//
// Called to shutdown subsystems when unloading a set of WAD resource files.
// Should be called prior to D_Init when loading a new set of WADs.
//
void STACK_ARGS D_Shutdown()
{
    if (gamestate == GS_LEVEL)
        G_ExitLevel(0, 0);

    // [ML] 9/11/10: Reset custom wad level information from MAPINFO et al.
    getLevelInfos().clear();
    getClusterInfos().clear();

    // stop sound effects and music
    S_Stop();

    DThinker::DestroyAllThinkers();

    // close all open WAD files
    W_Close();

    Res_ShutdownTextureManager();

    // reset the Zone memory manager
    Z_Close();

    // [AM] Level is now invalid due to torching zone memory.
    g_ValidLevel = false;

    // [AM] All of our dyncolormaps are freed, tidy up so we
    //      don't follow wild pointers.
    NormalLight.next = nullptr;
}

void D_Init_DEHEXTRA_Frames(void);

//
// D_DoomMain
//
// [NightFang] - Cause I cant call ArgsSet from g_level.cpp
// [ML] 23/1/07 - Add Response file support back in
//
void D_DoomMain()
{
    uint32_t p;

    gamestate = GS_STARTUP;

    // [RH] Initialize items. Still only used for the give command. :-(
    InitItems();
    // Initialize all extra frames
    D_Init_DEHEXTRA_Frames();

    M_FindResponseFile();             // [ML] 23/1/07 - Add Response file support back in

    if (lzo_init() != LZO_E_OK)       // [RH] Initialize the minilzo package.
        I_Error("Could not initialize LZO routines");

    C_ExecCmdLineParams(false, true); // [Nes] test for +logfile command

    // Always log by default
    if (!LOG.is_open())
        C_DoCommand("logfile");

    OWantFiles newwadfiles;

    const char *iwad_filename_cstr = Args.CheckValue("-iwad");
    if (iwad_filename_cstr)
    {
        OWantFile file;
        OWantFile::make(file, iwad_filename_cstr);
        newwadfiles.push_back(file);
    }

    D_AddWadCommandLineFiles(newwadfiles);

    D_LoadResourceFiles(newwadfiles);

    // Ch0wW: Loading the config here fixes the "addmap" issue.
    M_LoadDefaults();                 // load before initing other systems
    C_ExecCmdLineParams(true, false); // [RH] do all +set commands on the command line

    Printf(PRINT_HIGH, "I_Init: Init hardware.\n");
    I_Init();

    // [SL] Call init routines that need to be reinitialized every time WAD changes
    D_Init();
    atterm(D_Shutdown);

    Printf(PRINT_HIGH, "SV_InitNetwork: Checking network game status.\n");
    SV_InitNetwork();

    // Base systems have been inited; enable cvar callbacks
    cvar_t::EnableCallbacks();

    // [RH] User-configurable startup strings. Because BOOM does.
    if (GStrings(STARTUP1)[0])
        Printf(PRINT_HIGH, "%s\n", GStrings(STARTUP1));
    if (GStrings(STARTUP2)[0])
        Printf(PRINT_HIGH, "%s\n", GStrings(STARTUP2));
    if (GStrings(STARTUP3)[0])
        Printf(PRINT_HIGH, "%s\n", GStrings(STARTUP3));
    if (GStrings(STARTUP4)[0])
        Printf(PRINT_HIGH, "%s\n", GStrings(STARTUP4));
    if (GStrings(STARTUP5)[0])
        Printf(PRINT_HIGH, "%s\n", GStrings(STARTUP5));

    // developer mode
    devparm = Args.CheckParm("-devparm");

    if (devparm)
        DPrintf("%s", GStrings(D_DEVSTR)); // D_DEVSTR

    // Nomonsters
    if (Args.CheckParm("-nomonsters"))
        sv_nomonsters = 1;

    // Respawn
    if (Args.CheckParm("-respawn"))
        sv_monstersrespawn = 1;

    // Fast
    if (Args.CheckParm("-fast"))
        sv_fastmonsters = 1;

    // get skill / episode / map from parms
    strcpy(startmap, "MAP01");

    const char *val = Args.CheckValue("-skill");
    if (val)
        sv_skill.Set(val[0] - '0');

    p = Args.CheckParm("-timer");
    if (p && p < Args.NumArgs() - 1)
    {
        float time = atof(Args.GetArg(p + 1));
        Printf(PRINT_HIGH, "Levels will end after %g minute%s.\n", time, time > 1 ? "s" : "");
        sv_timelimit.Set(time);
    }

    if (Args.CheckValue("-avg"))
    {
        Printf(PRINT_HIGH, "Austin Virtual Gaming: Levels will end after 20 minutes\n");
        sv_timelimit.Set(20);
    }

    // [RH] Lock any cvars that should be locked now that we're
    // about to begin the game.
    cvar_t::EnableNoSet();

    // [RH] Now that all game subsystems have been initialized,
    // do all commands on the command line other than +set
    C_ExecCmdLineParams(false, false);

    Printf(PRINT_HIGH, "========== MUD Server Initialized ==========\n");

#ifdef UNIX
    if (Args.CheckParm("-fork"))
        daemon_init();
#endif

    p = Args.CheckParm("-warp");
    if (p && p < Args.NumArgs() - 1)
    {
        int32_t ep, map;

        ep  = 1;
        map = atoi(Args.GetArg(p + 1));

        strncpy(startmap, CalcMapName(ep, map), 8);
    }

    // [RH] Hack to handle +map
    p = Args.CheckParm("+map");
    if (p && p < Args.NumArgs() - 1)
    {
        strncpy(startmap, Args.GetArg(p + 1), 8);
        ((char *)Args.GetArg(p))[0] = '-';
    }

    level.mapname = startmap;

    G_ChangeMap();

    D_DoomLoop(); // never returns
}

VERSION_CONTROL(d_main_cpp, "$Id: 508e0fbd0bbeb03854c5ba73aa616acf860fc1df $")
