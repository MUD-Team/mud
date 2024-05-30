// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 309553abfd782610a6419696f1c7ac781bb65246 $
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
//		DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//		plus functions to determine game mode (shareware, registered),
//		parse command line parameters, configure game parameters (turbo),
//		and call the startup functions.
//
//-----------------------------------------------------------------------------

#include <algorithm>

#include "win32inc.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif
#ifdef UNIX
#include <dirent.h>
#include <unistd.h>
#endif
#include <math.h>

#include "c_bind.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "c_effect.h"
#include "cl_download.h"
#include "cl_main.h"
#include "d_main.h"
#include "g_game.h"
#include "g_mapinfo.h"
#include "gi.h"
#include "gstrings.h"
#include "i_input.h"
#include "i_music.h"
#include "i_system.h"
#include "i_video.h"
#include "m_alloc.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "m_random.h"
#include "minilzo.h"
#include "mud_profiling.h"
#include "mud_includes.h"
#include "p_setup.h"
#include "r_local.h"
#include "r_sky.h"
#include "res_texture.h"
#include "s_sound.h"
#include "stats.h"
#include "v_video.h"
#include "w_ident.h"
#include "w_wad.h"
#include "z_zone.h"
#include "script/lua_client_public.h"

extern size_t got_heapsize;

void D_CheckNetGame(void);
void D_DoomLoop(void);

extern int32_t           testingmode;
extern bool          gameisdead;
extern bool          M_DemoNoPlay; // [RH] if true, then skip any demos in the loop
extern DThinker      ThinkerCap;
extern dyncolormap_t NormalLight;

bool        devparm;    // started game with -devparm
const char *D_DrawIcon; // [RH] Patch name of icon to draw on next refresh

char    startmap[8];
bool    autostart;
event_t events[MAXEVENTS];
int32_t     eventhead;
int32_t     eventtail;
bool    demotest = false;

static int32_t pagetic;

EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_monstersrespawn)
EXTERN_CVAR(sv_fastmonsters)
EXTERN_CVAR(sv_allowredscreen)
EXTERN_CVAR(snd_sfxvolume)   // maximum volume for sound
EXTERN_CVAR(snd_musicvolume) // maximum volume for music

EXTERN_CVAR(vid_ticker)
EXTERN_CVAR(vid_defwidth)
EXTERN_CVAR(vid_defheight)
EXTERN_CVAR(vid_widescreen)
EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_vsync)
EXTERN_CVAR(g_resetinvonexit)

std::string LOG_FILE;

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents(void)
{
    event_t *ev;

    for (; eventtail != eventhead; eventtail = ++eventtail < MAXEVENTS ? eventtail : 0)
    {
        ev = &events[eventtail];
        G_Responder(ev);
    }
}

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent(const event_t *ev)
{
    if (ev->type == ev_mouse && gamestate == GS_LEVEL && !paused)
    {
        G_Responder((event_t *)ev);
        return;
    }

    events[eventhead] = *ev;

    if (++eventhead >= MAXEVENTS)
        eventhead = 0;
}

//
// D_DisplayTicker
//
// Called once every gametic to provide timing for display functions
//
void D_DisplayTicker()
{
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//
void D_Display()
{
    if (nodrawers || I_IsHeadless())
        return; // for comparative timing / profiling

    MUD_ZoneScoped;

    BEGIN_STAT(D_Display);

    // video mode must be changed before surfaces are locked in I_BeginUpdate
    V_AdjustVideoMode();

    I_BeginUpdate();

    // We always want to service downloads, even outside of a specific
    // download gamestate.
    CL_DownloadTick();

    switch (gamestate)
    {
    case GS_CONNECTING:
    case GS_CONNECTED:
        I_FinishUpdate();
        return;

    case GS_LEVEL:
        if (!gametic || !g_ValidLevel)
            break;

        // Drawn to R_GetRenderingSurface()
        // R_RenderPlayerView(&displayplayer());

        // C_DrawMid();
        // C_DrawGMid();
        break;

    default:
        break;
    }

    // draw pause pic
    if (paused)
    {
    }

    // [RH] Draw icon, if any
    if (D_DrawIcon)
    {
    }

    I_FinishUpdate(); // page flip or blit buffer

    END_STAT(D_Display);
}

//
//  D_DoomLoop
//
void D_DoomLoop(void)
{
    while (1)
    {
        D_RunTics(CL_RunTics, CL_DisplayTics);
    }
}

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker(void)
{
}

//
// D_PageDrawer
//
void D_PageDrawer()
{
    IRenderSurface *primary_surface = IRenderSurface::getCurrentRenderSurface();
    if (!primary_surface || !primary_surface->getWidth() || !primary_surface->getHeight())
    {
        return;
    }
    int32_t             surface_width = primary_surface->getWidth(), surface_height = primary_surface->getHeight();
    primary_surface->clear(); // ensure black background in matted modes
}

//
// D_Close
//
void D_Close()
{
    D_ClearTaskSchedulers();
}

//
// D_StartTitle
//
void D_StartTitle(void)
{
    // CL_QuitNetGame(NQ_SILENT);
    gameaction   = ga_nothing;        
}

bool HashOk(std::string &required, std::string &available)
{
    if (!required.length())
        return false;

    return required == available;
}

void CL_NetDemoPlay(const std::string &filename);

//
// D_Init
//
// Called to initialize subsystems when loading a new set of WAD resource
// files.
//
void D_Init()
{
    // only print init messages during startup, not when changing WADs
    static bool first_time = true;

    SetLanguageIDs();

    M_ClearRandom();

    // start the Zone memory manager
    Z_Init();
    if (first_time)
        Printf("Z_Init: Using native allocator with OZone bookkeeping.\n");

    // Load palette and set up colormaps
    V_Init();    

    if (first_time)
    	Printf(PRINT_HIGH, "Res_InitTextureManager: Init image resource management.\n");
    Res_InitTextureManager();

    // init the renderer
    if (first_time)
        Printf(PRINT_HIGH, "R_Init: Init DOOM refresh daemon.\n");
    R_Init();

    G_ParseMapInfo();
    G_ParseMusInfo();
    S_ParseSndInfo();

    // init the menu subsystem
    if (first_time)
        Printf(PRINT_HIGH, "M_Init: Init miscellaneous info.\n");

    if (first_time)
        Printf(PRINT_HIGH, "P_Init: Init Playloop state.\n");
    P_InitEffects();
    P_Init();

    // init sound and music
    if (first_time)
    {
        Printf(PRINT_HIGH, "S_Init: Setting up sound.\n");
        Printf(PRINT_HIGH, "S_Init: default sfx volume is %g\n", (float)snd_sfxvolume);
        Printf(PRINT_HIGH, "S_Init: default music volume is %g\n", (float)snd_musicvolume);
    }
    S_Init(snd_sfxvolume, snd_musicvolume);

    //	R_InitViewBorder();

    // init the status bar
    if (first_time)
        Printf(PRINT_HIGH, "ST_Init: Init status bar.\n");    

    first_time = false;
}

//
// D_Shutdown
//
// Called to shutdown subsystems when unloading a set of WAD resource files.
// Should be called prior to D_Init when loading a new set of WADs.
//
void D_Shutdown()
{    
    if (gamestate == GS_LEVEL)
        G_ExitLevel(0, 0);

    getLevelInfos().clear();
    getClusterInfos().clear();

    //	R_ShutdownViewBorder();

    // stop sound effects and music
    S_Stop();
    S_Deinit();

    DThinker::DestroyAllThinkers();

    // close all open WAD files
    W_Close();

    R_Shutdown();

    Res_ShutdownTextureManager();

    //	R_ShutdownColormaps();

    V_Close();

    // reset the Zone memory manager
    Z_Close();

    // [AM] Level is now invalid due to torching zone memory.
    g_ValidLevel = false;

    // [AM] All of our dyncolormaps are freed, tidy up so we
    //      don't follow wild pointers.
    NormalLight.next = nullptr;
}

void C_DoCommand(const char *cmd, uint32_t key);
void D_Init_DEHEXTRA_Frames(void);

void D_DoomMainShutdown()
{
    D_Close();
    D_Shutdown();
    I_ShutdownHardware();
    LUA_CloseClientState();    
    CL_DownloadShutdown();
}

//
// D_DoomMain
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
        C_DoCommand("logfile", 0);

    M_LoadDefaults();                 // load before initing other systems

    C_BindingsInit();                 // Ch0wW : Initialize bindings

    C_ExecCmdLineParams(true, false); // [RH] do all +set commands on the command line

    std::string              iwad;
    std::vector<std::string> pwads;
    const char              *iwadParam = Args.CheckValue("-iwad");
    if (iwadParam)
    {
        iwad = iwadParam;
    }

    OWantFiles newwadfiles;

    if (!iwad.empty())
    {
        OWantFile file;
        OWantFile::make(file, iwad);
        newwadfiles.push_back(file);
    }

    if (!pwads.empty())
    {
        for (size_t i = 0; i < pwads.size(); i++)
        {
            OWantFile file;
            OWantFile::make(file, pwads[i]);
            newwadfiles.push_back(file);
        }
    }

    D_AddWadCommandLineFiles(newwadfiles);

    D_LoadResourceFiles(newwadfiles);    

    Printf(PRINT_HIGH, "I_Init: Init hardware.\n");    
    I_Init();
    I_InitInput();

    // [SL] Call init routines that need to be reinitialized every time WAD changes    
    D_Init();

    // Base systems have been inited; enable cvar callbacks
    cvar_t::EnableCallbacks();

    LUA_OpenClientState();
\
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
        Printf(PRINT_HIGH, "%s", GStrings(D_DEVSTR)); // D_DEVSTR

    // set the default value for vid_ticker based on the presence of -devparm
    if (devparm)
        vid_ticker.SetDefault("1");

    // Nomonsters
    sv_nomonsters = Args.CheckParm("-nomonsters");

    // Respawn
    sv_monstersrespawn = Args.CheckParm("-respawn");

    // Fast
    sv_fastmonsters = Args.CheckParm("-fast");

    // Pistol start
    g_resetinvonexit = Args.CheckParm("-pistolstart");

    // get skill / episode / map from parms
    strcpy(startmap, "MAP01");

    const char *val = Args.CheckValue("-skill");
    if (val)
        sv_skill.Set(val[0] - '0');

    p = Args.CheckParm("-warp");
    if (p && p < Args.NumArgs() - 1)
    {
        int32_t ep, map;

        ep  = 1;
        map = atoi(Args.GetArg(p + 1));

        strncpy(startmap, CalcMapName(ep, map), 8);
        autostart = true;
    }

    // [RH] Hack to handle +map
    p = Args.CheckParm("+map");
    if (p && p < Args.NumArgs() - 1)
    {
        strncpy(startmap, Args.GetArg(p + 1), 8);
        ((char *)Args.GetArg(p))[0] = '-';
        autostart                   = true;
    }

    // NOTE(jsd): Set up local player color
    EXTERN_CVAR(cl_color);

    I_FinishClockCalibration();

    // Initialize HTTP subsystem
    CL_DownloadInit();

    Printf(PRINT_HIGH, "D_CheckNetGame: Checking network game status.\n");
    D_CheckNetGame();

    // [RH] Lock any cvars that should be locked now that we're
    // about to begin the game.
    cvar_t::EnableNoSet();

    // [RH] Now that all game subsystems have been initialized,
    // do all commands on the command line other than +set
    C_ExecCmdLineParams(false, false);

    // --- initialization complete ---

    Printf_Bold("\n\35\36\36\36\36 MUD Client Initialized \36\36\36\36\37\n");
    if (gamestate != GS_CONNECTING)
        Printf(PRINT_HIGH, "Type connect <address> to connect to a game.\n");
    Printf(PRINT_HIGH, "\n");

    if (autostart)
    {
        // single player warp (like in g_level)
        serverside = true;

        // Enable serverside settings to make them fully client-controlled.
        sv_allowexit      = 1;
        sv_allowredscreen = 1;

        players.clear();
        players.push_back(player_t());
        players.back().playerstate = PST_REBORN;
        consoleplayer_id = displayplayer_id = players.back().id = 1;

        G_InitNew(startmap);
    }
    else if (gamestate != GS_CONNECTING)
    {
        D_StartTitle();                 // start up intro loop
    }

    //void LUA_MainLoop();
    //LUA_MainLoop();
    
    //D_DoomLoop(); // never returns
}

VERSION_CONTROL(d_main_cpp, "$Id: 309553abfd782610a6419696f1c7ac781bb65246 $")
