// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 1ad69ee7ce8cf2366030a32559c84fc94a124fe9 $
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
//	Common functions to determine game mode (shareware, registered),
//	parse command line parameters, and handle wad changes.
//
//-----------------------------------------------------------------------------

#include "win32inc.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif
#ifdef UNIX
#include <dirent.h>
#include <unistd.h>
#endif
#include <stdlib.h>

#include <algorithm>
#include <sstream>

#include "c_console.h"
#include "d_main.h"
#include "g_game.h"
#include "g_spawninv.h"
#include "gi.h"
#include "gstrings.h"
#include "i_system.h"
#include "m_alloc.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_resfile.h"
#include "mud_includes.h"
#include "r_common.h"
#include "s_sound.h"
#include "w_ident.h"
#include "w_wad.h"
#include "z_zone.h"

OResFiles  wadfiles;
OWantFiles missingfiles;

bool        lastWadRebootSuccess = true;
extern bool step_mode;

bool  capfps = true;
float maxfps = 35.0f;

//
// D_GetTitleString
//
// Returns the proper name of the game currently loaded into gameinfo & gamemission
//
std::string D_GetTitleString()
{
    if (gamemission == commercial_freedoom)
        return "FreeDoom";

    return gameinfo.titleString;
}

//
// D_PrintIWADIdentity
//
static void D_PrintIWADIdentity()
{
    if (clientside)
    {
        Printf(PRINT_HIGH, "\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
                           "\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

        if (gamemode == undetermined)
            Printf_Bold("Game mode indeterminate, no standard wad found.\n\n");
        else
            Printf_Bold("%s\n\n", D_GetTitleString().c_str());
    }
    else
    {
        if (gamemode == undetermined)
            Printf(PRINT_HIGH, "Game mode indeterminate, no standard wad found.\n");
        else
            Printf(PRINT_HIGH, "%s\n", D_GetTitleString().c_str());
    }
}

//
// D_CleanseFileName
//
// Strips a file name of path information and transforms it into uppercase
//
std::string D_CleanseFileName(const std::string &filename, const std::string &ext)
{
    std::string newname(filename);

    M_FixPathSep(newname);
    if (ext.length())
        M_AppendExtension(newname, "." + ext);

    size_t slash = newname.find_last_of(PATHSEPCHAR);

    if (slash != std::string::npos)
        newname = newname.substr(slash + 1, newname.length() - slash);

    std::transform(newname.begin(), newname.end(), newname.begin(), toupper);

    return newname;
}

//
// D_FindIWAD
//
// Tries to find an IWAD from a set of known IWAD file names.
//
// Note; this function will likely evolve to search the /assets search
// path for a game manifest or something - Dasho
static bool FindIWAD(OResFile &out)
{
    char **rc = PHYSFS_enumerateFiles("/");
    char **i;
    bool   found_iwad = false;

    if (rc == NULL)
        return false;

    for (i = rc; *i != NULL; i++)
    {
        std::string ext;
        if (!M_ExtractFileExtension(*i, ext))
            continue;
        if (stricmp(ext.c_str(), "wad") != 0)
            continue;
        OWantFile wantfile;
        if (!OWantFile::make(wantfile, *i))
            continue;
        // Resolve the file.
        if (!M_ResolveWantedFile(out, wantfile))
            continue;
        if (W_IsIWAD(out))
        {
            found_iwad = true;
            break;
        }
    }

    PHYSFS_freeList(rc);

    return found_iwad;
}

/**
 * @brief Load files that are assumed to be resolved, in the correct order,
 *        and complete.
 *
 * @param newwadfiles New set of WAD files.
 */
static void LoadResolvedFiles(const OResFiles &newwadfiles)
{
    if (newwadfiles.size() < 1)
    {
        I_Error("Tried to load resources without an IWAD.");
    }

    ::wadfiles = newwadfiles;

    // Now scan the contents of the IWAD to determine which one it is
    W_ConfigureGameInfo(::wadfiles.at(0));

    // print info about the IWAD to the console
    D_PrintIWADIdentity();

    // set the window title based on which IWAD we're using
    I_SetTitleString(D_GetTitleString().c_str());

    ::modifiedgame = (::wadfiles.size() > 1); // more than the IWAD?

    W_InitMultipleFiles(::wadfiles);

    // [RH] Initialize localizable strings.
    // [SL] It is necessary to load the strings here since a dehacked patch
    // might change the strings
    ::GStrings.loadStrings(false);
}

//
// D_LoadResourceFiles
//
// Performs the grunt work of loading WAD and DEH/BEX files.
// The global wadfiles vector is filled with the list
// of loaded filenames and the missingfiles vector is also filled if
// applicable.
//
void D_LoadResourceFiles(const OWantFiles &newwadfiles)
{
    OResFile next_iwad;

    ::missingfiles.clear();

    // Resolve wanted wads.
    OResFiles resolved_wads;
    resolved_wads.reserve(newwadfiles.size());
    for (OWantFiles::const_iterator it = newwadfiles.begin(); it != newwadfiles.end(); ++it)
    {
        OResFile file;
        if (!M_ResolveWantedFile(file, *it))
        {
            ::missingfiles.push_back(*it);
            Printf(PRINT_WARNING, "Could not resolve resource file \"%s\".", it->getWantedPath().c_str());
            continue;
        }
        resolved_wads.push_back(file);
    }

    // IWAD //

    bool got_next_iwad = false;
    if (resolved_wads.size() >= 1)
    {
        // See if the first WAD we passed was an IWAD.
        const OResFile &possible_iwad = resolved_wads.at(0);
        if (W_IsIWAD(possible_iwad))
        {
            next_iwad     = possible_iwad;
            got_next_iwad = true;
            resolved_wads.erase(resolved_wads.begin());
        }
    }

    if (!got_next_iwad)
    {
        // Not provided an IWAD filename and an IWAD is not currently loaded?
        // Try to find *any* IWAD using FindIWAD.
        got_next_iwad = FindIWAD(next_iwad);
    }

    if (!got_next_iwad)
    {
        I_Error("Could not resolve an IWAD file.  Please ensure at least "
                "one IWAD is someplace where Odamex can find it.\n");
    }

    resolved_wads.insert(resolved_wads.begin(), next_iwad);
    LoadResolvedFiles(resolved_wads);
}

/**
 * @brief Check to see if the list of WAD files and patches matches the
 *        currently loaded files.
 *
 * @detail Note that this relies on the hashes being equal, so if you want
 *         resources to not be reloaded, ensure the hashes are equal by the
 *         time they reach this spot.
 *
 * @param newwadfiles WAD files to check.
 * @return True if everything checks out.
 */
static bool CheckWantedMatchesLoaded(const OWantFiles &newwadfiles)
{
    // Cheking sizes is a good first approximation.

    if (newwadfiles.size() != ::wadfiles.size())
    {
        return false;
    }

    for (size_t idx = 0; idx < ::wadfiles.size(); idx++)
    {
        const OMD5Hash &wadfilehash = ::wadfiles.at(idx).getMD5();
        const OMD5Hash &newfilehash = newwadfiles.at(idx).getWantedMD5();
        // Wadfile hash empty? If the existing wadfile is a path
        // just check basenames for equality and hope they have the same content for now - Dasho
        if (wadfilehash.empty())
        {
            if (::wadfiles.at(idx).getType() != OFILE_FOLDER)
                return false;
            else if (!newfilehash.empty()) // should be empty if also a path
                return false;
            else if (::wadfiles.at(idx).getBasename() != newwadfiles.at(idx).getBasename())
                return false;
        }
        // Don't think our "wanted files" do any MD5 stuff yet, so if the wanted file hash is empty
        // just check basenames - Dasho
        else if (newfilehash.empty())
        {
            if (::wadfiles.at(idx).getBasename() != newwadfiles.at(idx).getBasename())
                return false;
        }
        else if (wadfilehash != newfilehash)
        {
            return false;
        }
    }

    return true;
}

//
// D_DoomWadReboot
// [denis] change wads at runtime
// Returns false if there are missing files and fills the missingfiles
// vector
//
// [SL] passing an IWAD as newwadfiles[0] is now optional
//
bool D_DoomWadReboot(const OWantFiles &newwadfiles)
{
    // already loaded these?
    if (::lastWadRebootSuccess && CheckWantedMatchesLoaded(newwadfiles))
    {
        // fast track if files have not been changed
        Printf("Currently loaded resources match server checksums.\n\n");
        return true;
    }

    ::lastWadRebootSuccess = false;

    D_Shutdown();

    gamestate_t oldgamestate = ::gamestate;
    ::gamestate              = GS_STARTUP; // prevent console from trying to use nonexistant font

    // Load all the WAD and DEH/BEX files
    OResFiles oldwadfiles = ::wadfiles;

    D_LoadResourceFiles(newwadfiles);

    // get skill / episode / map from parms
    strcpy(startmap, "MAP01");

    D_Init();

    // preserve state
    ::lastWadRebootSuccess = ::missingfiles.empty();

    ::gamestate = oldgamestate; // GS_STARTUP would prevent netcode connecting properly

    return ::missingfiles.empty();
}

//
// AddCommandLineOptionFiles
//
// Adds the full path of all the file names following the given command line
// option parameter (eg, "-file") matching the specified extension to the
// filenames vector.
//
static void AddCommandLineOptionFiles(OWantFiles &out, const std::string &option)
{
    DArgs files = Args.GatherFiles(option.c_str());
    for (size_t i = 0; i < files.NumArgs(); i++)
    {
        OWantFile file;
        OWantFile::make(file, files.GetArg(i));
        out.push_back(file);
    }

    files.FlushArgs();
}

//
// D_AddWadCommandLineFiles
//
// Add the WAD files specified with -file.
// Call this from D_DoomMain
//
void D_AddWadCommandLineFiles(OWantFiles &out)
{
    AddCommandLineOptionFiles(out, "-file");
}

// ============================================================================
//
// TaskScheduler class
//
// ============================================================================
//
// Attempts to schedule a task (indicated by the function pointer passed to
// the concrete constructor) at a specified interval. For uncapped rates, that
// interval is simply as often as possible. For capped rates, the interval is
// specified by the rate parameter.
//

class TaskScheduler
{
  public:
    virtual ~TaskScheduler()
    {
    }
    virtual void    run()                = 0;
    virtual dtime_t getNextTime() const  = 0;
    virtual float   getRemainder() const = 0;
};

class UncappedTaskScheduler : public TaskScheduler
{
  public:
    UncappedTaskScheduler(void (*task)()) : mTask(task)
    {
    }

    virtual ~UncappedTaskScheduler()
    {
    }

    virtual void run()
    {
        mTask();
    }

    virtual dtime_t getNextTime() const
    {
        return I_GetTime();
    }

    virtual float getRemainder() const
    {
        return 0.0f;
    }

  private:
    void (*mTask)();
};

class CappedTaskScheduler : public TaskScheduler
{
  public:
    CappedTaskScheduler(void (*task)(), float rate, int32_t max_count)
        : mTask(task), mMaxCount(max_count), mFrameDuration(I_ConvertTimeFromMs(1000) / rate),
          mAccumulator(mFrameDuration), mPreviousFrameStartTime(I_GetTime())
    {
    }

    virtual ~CappedTaskScheduler()
    {
    }

    virtual void run()
    {
        mFrameStartTime = I_GetTime();
        mAccumulator += mFrameStartTime - mPreviousFrameStartTime;
        mPreviousFrameStartTime = mFrameStartTime;

        int32_t count = mMaxCount;

        while (mAccumulator >= mFrameDuration && count--)
        {
            mTask();
            mAccumulator -= mFrameDuration;
        }
    }

    virtual dtime_t getNextTime() const
    {
        return mFrameStartTime + mFrameDuration - mAccumulator;
    }

    virtual float getRemainder() const
    {
        // mAccumulator can be greater than mFrameDuration so only get the
        // time remaining until the next frame
        dtime_t remaining_time = mAccumulator % mFrameDuration;
        return (float)(double(remaining_time) / mFrameDuration);
    }

  private:
    void (*mTask)();
    const int32_t mMaxCount;
    const dtime_t mFrameDuration;
    dtime_t       mAccumulator;
    dtime_t       mFrameStartTime;
    dtime_t       mPreviousFrameStartTime;
};

static TaskScheduler *simulation_scheduler;
static TaskScheduler *display_scheduler;

//
// D_InitTaskSchedulers
//
// Checks for external changes to the rate for the simulation and display
// tasks and instantiates the appropriate TaskSchedulers.
//
static void D_InitTaskSchedulers(void (*sim_func)(), void (*display_func)())
{
    bool capped_simulation = true;
    bool capped_display    = capfps;

    static bool  previous_capped_simulation = !capped_simulation;
    static bool  previous_capped_display    = !capped_display;
    static float previous_maxfps            = -1.0f;

    if (capped_simulation != previous_capped_simulation)
    {
        previous_capped_simulation = capped_simulation;

        delete simulation_scheduler;

        if (capped_simulation)
            simulation_scheduler = new CappedTaskScheduler(sim_func, TICRATE, 4);
        else
            simulation_scheduler = new UncappedTaskScheduler(sim_func);
    }

    if (capped_display != previous_capped_display || maxfps != previous_maxfps)
    {
        previous_capped_display = capped_display;
        previous_maxfps         = maxfps;

        delete display_scheduler;

        if (capped_display)
            display_scheduler = new CappedTaskScheduler(display_func, maxfps, 1);
        else
            display_scheduler = new UncappedTaskScheduler(display_func);
    }
}

void STACK_ARGS D_ClearTaskSchedulers()
{
    delete simulation_scheduler;
    delete display_scheduler;
    simulation_scheduler = NULL;
    display_scheduler    = NULL;
}

//
// D_RunTics
//
// The core of the main game loop.
// This loop allows the game simulation timing to be decoupled from the display
// timing. If the the user selects a capped framerate and isn't using the
// -timedemo parameter, both the simulation and display functions will be called
// TICRATE times a second. If the framerate is uncapped, the simulation function
// will still be called TICRATE times a second but the display function will
// be called as often as possible. After each iteration through the loop,
// the program yields briefly to the operating system.
//
void D_RunTics(void (*sim_func)(), void (*display_func)())
{
    D_InitTaskSchedulers(sim_func, display_func);

    simulation_scheduler->run();

#ifdef CLIENT_APP
    // Use linear interpolation for rendering entities if the display
    // framerate is not synced with the simulation frequency.
    // Ch0wW : if you experience a spinning effect while trying to pause the frame,
    // don't forget to add your condition here.
    if ((maxfps == TICRATE && capfps) || paused || step_mode)
        render_lerp_amount = FRACUNIT;
    else
        render_lerp_amount = simulation_scheduler->getRemainder() * FRACUNIT;
#endif

    display_scheduler->run();

    // Sleep until the next scheduled task.
    dtime_t simulation_wake_time = simulation_scheduler->getNextTime();
    dtime_t display_wake_time    = display_scheduler->getNextTime();
    dtime_t wake_time            = std::min<dtime_t>(simulation_wake_time, display_wake_time);

    const dtime_t max_sleep_amount = 1000LL * 1000LL; // 1ms

    // Sleep in 1ms increments until the next scheduled task
    for (dtime_t now = I_GetTime(); wake_time > now; now = I_GetTime())
    {
        dtime_t sleep_amount = std::min<dtime_t>(max_sleep_amount, wake_time - now);
        I_Sleep(sleep_amount);
    }
}

VERSION_CONTROL(d_main_cpp, "$Id: 1ad69ee7ce8cf2366030a32559c84fc94a124fe9 $")
