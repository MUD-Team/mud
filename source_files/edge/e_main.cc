//---------------------------------------------------------------------------
//  EDGE Main Init + Program Loop Code
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//      EDGE main program (E_Main),
//      game loop (E_Loop) and startup functions.
//
// -MH- 1998/07/02 "shootupdown" --> "true_3d_gameplay"
// -MH- 1998/08/19 added up/down movement variables
//

#include "e_main.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <algorithm>
#include <vector>

#include "con_gui.h"
#include "con_main.h"
#include "con_var.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "e_input.h"
#include "epi.h"
#include "edge_profiling.h"
#include "epi_filesystem.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "g_game.h"
#include "i_defs_gl.h"
#include "i_movie.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "m_random.h"
#include "n_network.h"
#include "p_setup.h"
#include "p_spec.h"
#include "physfs.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "s_music.h"
#include "s_sound.h"
#include "sv_main.h"
#include "version.h"
#include "w_files.h"
#include "w_sprite.h"

extern ConsoleVariable double_framerate;
extern ConsoleVariable busy_wait;

extern ConsoleVariable gamma_correction;

ECFrameStats ec_frame_stats;

// Application active?
int app_state = kApplicationActive;

bool single_tics = false; // debug flag to cancel adaptiveness

// -ES- 2000/02/13 Takes screenshot every screenshot_rate tics.
// Must be used in conjunction with single_tics.
static int screenshot_rate;

// For screenies...
bool m_screenshot_required = false;

bool custom_MenuMain       = false;
bool custom_MenuEpisode    = false;
bool custom_MenuDifficulty = false;

epi::File *log_file   = nullptr;

GameFlags default_game_flags = {
    false,      // nomonsters
    false,      // fast_monsters

    false,      // respawn
    false,      // enemy_respawn_mode
    false,      // item respawn

    8,          // gravity
    false,      // more blood

    kAutoAimOn, // autoaim

    true,       // cheats
    false,      // limit_zoom

    true,       // kicking
    true,       // weapon_switch
    false,      // team_damage
};

// -KM- 1998/12/16 These flags are the users prefs and are copied to
//   gameflags when a new level is started.
// -AJA- 2000/02/02: Removed initialisation (done in code using
//       `default_game_flags').

GameFlags global_flags;

bool mus_pause_stop = false;

std::string epkfile;

std::string game_directory;
std::string home_directory;

// not using EDGE_DEFINE_CONSOLE_VARIABLE here since var name != cvar name
ConsoleVariable m_language("language", "ENGLISH", kConsoleVariableFlagArchive);

EDGE_DEFINE_CONSOLE_VARIABLE(log_filename, "edge-classic.log", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(config_filename, "edge-classic.cfg", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(game_name, "EDGE-Classic", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(edge_version, "1.38", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(team_name, "EDGE Team", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(application_name, "EDGE-Classic", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(homepage, "https://edge-classic.github.io", kConsoleVariableFlagNoReset)

EDGE_DEFINE_CONSOLE_VARIABLE(force_infighting, "0", kConsoleVariableFlagArchive)

EDGE_DEFINE_CONSOLE_VARIABLE(ddf_strict, "0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(ddf_lax, "0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(ddf_quiet, "0", kConsoleVariableFlagArchive)

EDGE_DEFINE_CONSOLE_VARIABLE(skip_intros, "0", kConsoleVariableFlagArchive)

class StartupProgress
{
  private:
    std::vector<std::string> startup_messages_;

  public:
    StartupProgress()
    {
    }

    ~StartupProgress()
    {
    }

    bool IsEmpty()
    {
        return startup_messages_.empty();
    }

    void Clear()
    {
        startup_messages_.clear();
    }

    void AddMessage(const char *message)
    {
        if (startup_messages_.size() >= 15)
            startup_messages_.erase(startup_messages_.begin());
        startup_messages_.push_back(message);
    }

    void DrawIt()
    {
        StartFrame();
/*
        if (gamma_correction.f_ < 0)
        {
            int col = (1.0f + gamma_correction.f_) * 255;
            glEnable(GL_BLEND);
            glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            HUDSolidBox(hud_x_left, 0, hud_x_right, 200, epi::MakeRGBA(col, col, col));
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_BLEND);
        }
        else if (gamma_correction.f_ > 0)
        {
            int col = gamma_correction.f_ * 255;
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ONE);
            HUDSolidBox(hud_x_left, 0, hud_x_right, 200, epi::MakeRGBA(col, col, col));
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_BLEND);
        }
*/
        FinishFrame();
    }
};

static StartupProgress startup_progress;

void StartupProgressMessage(const char *message)
{
    startup_progress.AddMessage(message);
    startup_progress.DrawIt();
}

//
// -ACB- 1999/09/20 Created. Sets Global Stuff.
//
static void SetGlobalVariables(void)
{
    int         p;
    std::string s;

    // Screen Resolution Check...
    if (FindArgument("borderless") > 0)
        current_window_mode = 2;
    else if (FindArgument("fullscreen") > 0)
        current_window_mode = 1;
    else if (FindArgument("windowed") > 0)
        current_window_mode = 0;

    s = ArgumentValue("width");
    if (!s.empty())
    {
        if (current_window_mode == 2)
            LogWarning("Current display mode set to borderless fullscreen. Provided "
                       "width of %d will be ignored!\n",
                       atoi(s.c_str()));
        else
            current_screen_width = atoi(s.c_str());
    }

    s = ArgumentValue("height");
    if (!s.empty())
    {
        if (current_window_mode == 2)
            LogWarning("Current display mode set to borderless fullscreen. Provided "
                       "height of %d will be ignored!\n",
                       atoi(s.c_str()));
        else
            current_screen_height = atoi(s.c_str());
    }

    p = FindArgument("res");
    if (p > 0 && p + 2 < int(program_argument_list.size()) && !ArgumentIsOption(p + 1) && !ArgumentIsOption(p + 2))
    {
        if (current_window_mode == 2)
            LogWarning("Current display mode set to borderless fullscreen. Provided "
                       "resolution of %dx%d will be ignored!\n",
                       atoi(program_argument_list[p + 1].c_str()), atoi(program_argument_list[p + 2].c_str()));
        else
        {
            current_screen_width  = atoi(program_argument_list[p + 1].c_str());
            current_screen_height = atoi(program_argument_list[p + 2].c_str());
        }
    }

    // Bits per pixel check....
    s = ArgumentValue("bpp");
    if (!s.empty())
    {
        current_screen_depth = atoi(s.c_str());

        if (current_screen_depth <= 4) // backwards compat
            current_screen_depth *= 8;
    }

    // restrict depth to allowable values
    if (current_screen_depth < 15)
        current_screen_depth = 15;
    if (current_screen_depth > 32)
        current_screen_depth = 32;

    // If borderless fullscreen mode, override any provided dimensions so
    // StartupGraphics will scale to native res
    if (current_window_mode == 2)
    {
        current_screen_width  = 100000;
        current_screen_height = 100000;
    }

    // sprite kludge (TrueBSP)
    p = FindArgument("spritekludge");
    if (p > 0)
    {
        if (p + 1 < int(program_argument_list.size()) && !ArgumentIsOption(p + 1))
            sprite_kludge = atoi(program_argument_list[p + 1].c_str());

        if (!sprite_kludge)
            sprite_kludge = 1;
    }

    s = ArgumentValue("screenshot");
    if (!s.empty())
    {
        screenshot_rate = atoi(s.c_str());
        single_tics     = true;
    }

    // -AJA- 1999/10/18: Reworked these with CheckBooleanParameter
    CheckBooleanParameter("sound", &no_sound, true);
    CheckBooleanParameter("music", &no_music, true);
    CheckBooleanParameter("items_respawn", &global_flags.items_respawn, false);
    CheckBooleanParameter("monsters", &global_flags.no_monsters, true);
    CheckBooleanParameter("fast", &global_flags.fast_monsters, false);
    CheckBooleanParameter("kick", &global_flags.kicking, false);
    CheckBooleanParameter("single_tics", &single_tics, false);
    CheckBooleanParameter("blood", &global_flags.more_blood, false);
    CheckBooleanParameter("cheats", &global_flags.cheats, false);
    CheckBooleanParameter("weaponswitch", &global_flags.weapon_switch, false);

    if (FindArgument("infight") > 0)
        force_infighting = 1;

    if (!global_flags.enemies_respawn)
    {
        if (FindArgument("newnmrespawn") > 0)
        {
            global_flags.enemy_respawn_mode = true;
            global_flags.enemies_respawn    = true;
        }
        else if (FindArgument("respawn") > 0)
        {
            global_flags.enemies_respawn = true;
        }
    }

    // check for strict and no-warning options
    CheckBooleanConsoleVariable("strict", &ddf_strict, false);
    CheckBooleanConsoleVariable("lax", &ddf_lax, false);
    CheckBooleanConsoleVariable("warn", &ddf_quiet, true);

    strict_errors = ddf_strict.d_ ? true : false;
    lax_errors    = ddf_lax.d_ ? true : false;
    no_warnings   = ddf_quiet.d_ ? true : false;
}

//
// SetLanguage
//
void SetLanguage(void)
{
    std::string want_lang = ArgumentValue("lang");
    if (!want_lang.empty())
        m_language = want_lang;

    if (language.Select(m_language.c_str()))
        return;

    LogWarning("Invalid language: '%s'\n", m_language.c_str());

    if (!language.Select(0))
        FatalError("Unable to select any language!");

    m_language = language.GetName();
}

//
// SpecialWadVerify
//
static void SpecialWadVerify(void)
{
    StartupProgressMessage("Verifying EDGE_DEFS version...");

    epi::File *data = OpenPackFile("/version.txt", "");

    if (!data)
        FatalError("Version file not found. Get edge_defs.epk at "
                   "https://github.com/edge-classic/EDGE-classic");

    // parse version number
    std::string verstring = data->ReadAsString();
    const char *s         = verstring.data();
    int         epk_ver   = atoi(s) * 100;

    while (epi::IsDigitASCII(*s))
        s++;
    s++;
    epk_ver += atoi(s);

    delete data;

    float real_ver = epk_ver / 100.0;

    LogPrint("EDGE_DEFS.EPK version %1.2f found.\n", real_ver);

    if (real_ver < edge_version.f_)
    {
        FatalError("EDGE_DEFS.EPK is an older version (got %1.2f, expected %1.2f)\n", real_ver, edge_version.f_);
    }
    else if (real_ver > edge_version.f_)
    {
        LogWarning("EDGE_DEFS.EPK is a newer version (got %1.2f, expected %1.2f)\n", real_ver, edge_version.f_);
    }
}

//
// ShowNotice
//
static void ShowNotice(void)
{
    ConsoleMessageColor(epi::MakeRGBA(64, 192, 255));

    LogPrint("%s", language["Notice"]);
}

static void DoSystemStartup(void)
{
    // startup the system now
    InitializeImages();

    LogDebug("- System startup begun.\n");

    SystemStartup();

    // -ES- 1998/09/11 Use ChangeResolution to enter gfx mode

    DumpResolutionList();

    // -KM- 1998/09/27 Change res now, so music doesn't start before
    // screen.  Reset clock too.
    LogDebug("- Changing Resolution...\n");

    SetInitialResolution();

    RendererInit();
    SoftInitializeResolution();

    LogDebug("- System startup done.\n");
}

//
// E_Display
//
// Draw current display, possibly wiping it from the previous
//
// -ACB- 1998/07/27 Removed doublebufferflag check (unneeded).

void EdgeDisplay(void)
{
    EDGE_ZoneScoped;

    // Start the frame - should we need to.
    StartFrame();    

    switch (game_state)
    {
    case kGameStateLevel:
        // Fudged for our in development 1360x768 resolution, the expand_w parameter to RenderView
        // is based on a bunch of tweaking for 320x200 virtual resolution
        RenderView(0, 0, 1360, 768, players[console_player]->map_object_, true, 1360.f/768.f * 0.85f);
        break;

    case kGameStateNothing:
        break;
    }

    // process mouse and keyboard events
    NetworkUpdate();    

    /*
    if (gamma_correction.f_ < 0)
    {
        int col = (1.0f + gamma_correction.f_) * 255;
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        HUDSolidBox(hud_x_left, 0, hud_x_right, 200, epi::MakeRGBA(col, col, col));
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
    }
    else if (gamma_correction.f_ > 0)
    {
        int col = gamma_correction.f_ * 255;
        glEnable(GL_BLEND);
        glBlendFunc(GL_DST_COLOR, GL_ONE);
        HUDSolidBox(hud_x_left, 0, hud_x_right, 200, epi::MakeRGBA(col, col, col));
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
    }
    */

    if (m_screenshot_required)
    {
        m_screenshot_required = false;
        TakeScreenshot(true);
    }
    else if (screenshot_rate && (game_state >= kGameStateLevel))
    {
        EPI_ASSERT(single_tics);

        if (level_time_elapsed % screenshot_rate == 0)
            TakeScreenshot(false);
    }

    FinishFrame(); // page flip or blit buffer
}

static std::string GetPrefDirectory()
{
    return std::string(PHYSFS_getPrefDir("MUD Team", "MUD Client"));
}

//
// Detects which directories to search for DDFs, WADs and other files in.
//
// -ES- 2000/01/01 Written.
//
static void InitializeDirectories(void)
{
    // Note: This might need adjusting for Apple
    std::string s = PHYSFS_getBaseDir();

    game_directory = s;

    if (PHYSFS_mount(game_directory.c_str(), "/", 0) == 0)
        FatalError("PHYSFS: Failed to mount install directory %s: %s\n", game_directory.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));

    if (FindArgument("portable") > 0)
            home_directory = game_directory;

    if (home_directory.empty())
        home_directory = GetPrefDirectory();

    if (game_directory != home_directory && PHYSFS_mount(home_directory.c_str(), "/", 0) == 0)
        FatalError("PHYSFS: Failed to mount user directory %s: %s\n", home_directory.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));

    if (PHYSFS_setWriteDir(home_directory.c_str()) == 0)
        FatalError("PHYSFS: Failed to set write directory %s: %s\n", home_directory.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));

    // edge_defs.epk file
    if (PHYSFS_isDirectory("edge_defs"))
        epkfile = "edge_defs";
    else
        epkfile = "edge_defs.epk";

    // These will make folders in the pref path if they don't already exist
    // "Failure" for already existing is acceptable in this case
    epi::MakeDirectory("cache");
    epi::MakeDirectory("savegame");
    epi::MakeDirectory("screenshot");
    epi::MakeDirectory("soundfont");

    SaveClearSlot("current");
}

std::string ParseEdgeGameFile(epi::Lexer &lex)
{
    std::string value;
    for (;;)
    {
        std::string key;

        epi::TokenKind tok = lex.Next(key);

        if (tok == epi::kTokenEOF)
        {
            value.clear();
            return value;
        }

        if (tok == epi::kTokenError)
            FatalError("ParseEdgeGameFile: error parsing file!\n");

        if (lex.Match("="))
        {
            lex.Next(value);
            if (!lex.Match(";"))
                FatalError("Malformed EDGEGAME file: missing ';'\n");
        }

        // The last line of the config writer causes a weird blank key with an
        // EOF value, so just return here
        if (tok == epi::kTokenEOF)
        {
            value.clear();
            return value;
        }

        if (tok == epi::kTokenError)
            FatalError("ParseEdgeGameFile: malformed value for key %s!\n", key.c_str());

        if (tok == epi::kTokenIdentifier && epi::StringCaseCompareASCII(key, "game_name") == 0)
        {
            return value;
        }
    }
}

// If a valid EDGEGAME is found, parse and return the game name
static std::string CheckPackForGameFiles(std::string check_pack)
{
    std::string title;
    if (PHYSFS_mount(check_pack.c_str(), "temp", 0) == 0)
        return title;
    if (!epi::FileExists("temp/EDGEGAME.txt"))
    {
        PHYSFS_unmount(check_pack.c_str());
        return title;
    }
    epi::File *eg_file = epi::FileOpen("temp/EDGEGAME.txt", epi::kFileAccessRead);
    if (!eg_file)
    {
        PHYSFS_unmount(check_pack.c_str());
        return title;
    }
    std::string edge_game = eg_file->ReadAsString();
    delete eg_file;
    PHYSFS_unmount(check_pack.c_str());
    epi::Lexer lex(edge_game);
    title = ParseEdgeGameFile(lex);
    return title;
}

//
// Adds main game content and edge_defs folder/EPK
//
static void IdentifyVersion(void)
{
    if (PHYSFS_isDirectory(epkfile.c_str()))
        AddDataFile(epi::PathAppend(game_directory, epkfile));
    else
    {
        if (!epi::FileExists(epkfile))
            FatalError("IdentifyVersion: Could not find required %s.%s!\n", kRequiredEPK, "epk");
        AddDataFile(epi::PathAppend(game_directory, epkfile));
    }

    LogDebug("- Identify Version\n");

    // Check -iwad parameter, find out if it is the IWADs directory
    std::string iwad_par;

    std::string s = ArgumentValue("game");

    if (s.empty()) // legacy/potential launcher compat
        s = ArgumentValue("iwad");

    iwad_par = s;

    // If explicitly passed a parameter for the game selection, validate it and treat
    // as a fatal error if the path specified is not a valid game
    if (!iwad_par.empty())
    {
        std::string game_check;
        if (epi::GetExtension(iwad_par).empty())
        {
            game_check = CheckPackForGameFiles(iwad_par);
            if (game_check.empty())
                FatalError("Folder %s passed via -game parameter, but no "
                           "EDGEGAME file detected!\n",
                           iwad_par.c_str());
            else
            {
                game_name = game_check;
                AddDataFile(iwad_par);
                LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                return;
            }
        }
        else if (epi::StringCaseCompareASCII(epi::GetExtension(iwad_par), ".epk") == 0)
        {
            game_check = CheckPackForGameFiles(iwad_par);
            if (game_check.empty())
                FatalError("EPK %s passed via -game parameter, but no "
                           "EDGEGAME file detected!\n",
                           iwad_par.c_str());
            else
            {
                game_name = game_check;
                AddDataFile(iwad_par);
                LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                return;
            }
        }
        else
            FatalError("%s is not a valid extension for a game file! (%s)\n", epi::GetExtension(iwad_par).c_str(),
                       iwad_par.c_str());
    }

    // If we have made it here, we could not locate a valid game anywhere
    FatalError("IdentifyVersion: Could not locate a valid game file!\n");
}

static void CheckTurbo(void)
{
    int turbo_scale = 100;

    int p = FindArgument("turbo");

    if (p > 0)
    {
        if (p + 1 < int(program_argument_list.size()) && !ArgumentIsOption(p + 1))
            turbo_scale = atoi(program_argument_list[p + 1].c_str());
        else
            turbo_scale = 200;

        if (turbo_scale < 10)
            turbo_scale = 10;
        if (turbo_scale > 400)
            turbo_scale = 400;

        ConsoleMessageLDF("TurboScale", turbo_scale);
    }

    SetTurboScale(turbo_scale);
}

static void ShowDateAndVersion(void)
{
    time_t cur_time;
    char   timebuf[100];

    time(&cur_time);
    strftime(timebuf, 99, "%I:%M %p on %d/%b/%Y", localtime(&cur_time));

    LogDebug("[Log file created at %s]\n\n", timebuf);

    LogPrint("%s v%s compiled on " __DATE__ " at " __TIME__ "\n", application_name.c_str(), edge_version.c_str());
    LogPrint("%s homepage is at %s\n", application_name.c_str(), homepage.c_str());

    LogPrint("Executable path: '%s'\n", executable_path.c_str());

    DumpArguments();
}

static void SetupLogFile(void)
{
    // -AJA- 2003/11/08 The log file gets all ConsolePrints, LogPrints,
    //                  LogWarnings and FatalErrors.

    log_file   = nullptr;

    if (FindArgument("nolog") < 0)
    {
        log_file = epi::FileOpen(log_filename.s_, epi::kFileAccessWrite);

        if (!log_file)
            FatalError("[EdgeStartup] Unable to create log file\n");
    }
}

static void AddSingleCommandLineFile(std::string name)
{
    // Bit of an assumption, but still
    if (epi::GetExtension(name).empty())
    {
        AddDataFile(name);
        return;
    }

    std::string ext = epi::GetExtension(name);

    epi::StringLowerASCII(ext);

    if (ext == ".rts" || ext == ".ddf" || ext == ".ldf")
        FatalError("Loose script files are not supported\n");

    if (ext == ".pk3" || ext == ".epk" || ext == ".zip")
        AddDataFile(name);
    else
    {
        LogDebug("unknown file type: %s\n", name.c_str());
        return;
    }
}

static void AddCommandLineFiles(void)
{
    // first handle "loose" files (arguments before the first option)

    int p;

    for (p = 1; p < int(program_argument_list.size()) && !ArgumentIsOption(p); p++)
    {
        AddSingleCommandLineFile(program_argument_list[p]);
    }

    // next handle the -file option (we allow multiple uses)

    p = FindArgument("file");

    while (p > 0 && p < int(program_argument_list.size()) &&
           (!ArgumentIsOption(p) || epi::StringCompare(program_argument_list[p], "-file") == 0))
    {
        // the parms after p are wadfile/lump names,
        // go until end of parms or another '-' preceded parm
        if (!ArgumentIsOption(p))
            AddSingleCommandLineFile(program_argument_list[p]);

        p++;
    }
}

static void InitializeDDF(void)
{
    LogDebug("- Initialising DDF\n");

    DDFInit();
}

void EdgeShutdown(void)
{
    LevelShutdown();
    RendererShutdown();
    NetworkShutdown();
}

static void EdgeStartup(void)
{
    ConsoleInit();

    // -AJA- 2000/02/02: initialise global gameflags to defaults
    global_flags = default_game_flags;

    InitializeDirectories();

    // Version check ?
    if (FindArgument("version") > 0)
    {
        // -AJA- using FatalError here, since LogPrint crashes this early on
        FatalError("\n%s version is %s\n", application_name.c_str(), edge_version.c_str());
    }

    SetupLogFile();

    ShowDateAndVersion();

    LoadDefaults();

    HandleProgramArguments();
    SetGlobalVariables();

    DoSystemStartup();

    InitializeDDF();
    IdentifyVersion();
    AddCommandLineFiles();
    CheckTurbo();

    ProcessMultipleFiles();
    DDFParseEverything();
    ProcessPackContents();
    StartupMusic();

    DDFCleanUp();
    SetLanguage();

    CreateUserImages();    

    ConsoleStart();
    SpecialWadVerify();
    ShowNotice();

    PrecacheSounds();
    InitializeSprites();

    RendererStartup();
    PlayerStateInit();
    InitializeSwitchList();
    InitializeAnimations();
    InitializeSound();
    NetworkInitialize();
    CheatInitialize();
}

static void InitialState(void)
{
    LogDebug("- Setting up Initial State...\n");

    std::string ps;

    // do loadgames first, as they contain all of the
    // necessary state already (in the savegame).

    if (FindArgument("playdemo") > 0 || FindArgument("timedemo") > 0 || FindArgument("record") > 0)
    {
        FatalError("Demos are no longer supported\n");
    }

    ps = ArgumentValue("loadgame");
    if (!ps.empty())
    {
        DeferredLoadGame(atoi(ps.c_str()));
        return;
    }

    bool warp = false;

    // get skill / episode / map from parms
    std::string warp_map;
    SkillLevel  warp_skill      = kSkillMedium;
    int         warp_deathmatch = 0;

    int bots = 0;

    ps = ArgumentValue("bots");
    if (!ps.empty())
        bots = atoi(ps.c_str());

    ps = ArgumentValue("warp");
    if (!ps.empty())
    {
        warp     = true;
        warp_map = ps;
    }

    if (!warp || !warp_map.length())
    {
        warp = true;
        warp_map = "1";
        LogWarning("Warping to first map, as we don't currently have a main menu\n");
    }

    // -KM- 1999/01/29 Use correct skill: 1 is easiest, not 0
    ps = ArgumentValue("skill");
    if (!ps.empty())
    {
        warp       = true;
        warp_skill = (SkillLevel)(atoi(ps.c_str()) - 1);
    }

    // deathmatch check...
    int pp = FindArgument("deathmatch");
    if (pp > 0)
    {
        warp_deathmatch = 1;

        if (pp + 1 < int(program_argument_list.size()) && !ArgumentIsOption(pp + 1))
            warp_deathmatch = GLM_MAX(1, atoi(program_argument_list[pp + 1].c_str()));

        warp = true;
    }
    else if (FindArgument("altdeath") > 0)
    {
        warp_deathmatch = 2;

        warp = true;
    }

    // start the appropriate game based on parms
    if (!warp)
    {
        LogDebug("- Startup: showing title screen.\n");        
        startup_progress.Clear();
        return;
    }

    NewGameParameters params;

    params.skill_      = warp_skill;
    params.deathmatch_ = warp_deathmatch;
    params.level_skip_ = true;

    if (warp_map.length() > 0)
        params.map_ = LookupMap(warp_map.c_str());
    else
        params.map_ = LookupMap("1");

    if (!params.map_)
        FatalError("-warp: no such level '%s'\n", warp_map.c_str());

    EPI_ASSERT(MapExists(params.map_));
    EPI_ASSERT(params.map_->episode_);

    params.random_seed_ = PureRandomNumber();

    params.SinglePlayer(bots);

    DeferredNewGame(params);
}

//
// ---- MAIN ----
//
// -ACB- 1998/08/10 Removed all reference to a gamemap, episode and mission
//                  Used LanguageLookup() for lang specifics.
//
// -ACB- 1998/09/06 Removed all the unused code that no longer has
//                  relevance.
//
// -ACB- 1999/09/04 Removed statcopy parm check - UNUSED
//
// -ACB- 2004/05/31 Moved into a namespace, the c++ revolution begins....
//
void EdgeInit(int argc, const char **argv)
{
    // Seed RandomByte RNG
    InitRandomState();

    // Implemented here - since we need to bring the memory manager up first
    // -ACB- 2004/05/31
    ParseArguments(argc, argv);

    EdgeStartup();

    InitialState();

    ConsoleMessageColor(SG_YELLOW_RGBA32);
    LogPrint("%s v%s initialisation complete.\n", application_name.c_str(), edge_version.c_str());

    LogDebug("- Entering game loop...\n");
}

void EdgeTick()
{
    if (!(app_state & kApplicationPendingQuit))
    {
        // We always do this once here, although the engine may
        // makes in own calls to keep on top of the event processing
        ControlGetEvents();

        if (app_state & kApplicationActive)
            EdgeTicker();
        else if (!busy_wait.d_)
        {
            SleepForMilliseconds(5);
        }
    }
}

//
// Called when this application has lost focus (i.e. an ALT+TAB event)
//
void EdgeIdle(void)
{
    ReleaseAllKeys();
}

//
// This Function is called for a single loop in the system.
//
// -ACB- 1999/09/24 Written
// -ACB- 2004/05/31 Namespace'd
//
void EdgeTicker(void)
{
    EDGE_ZoneScoped;

    DoBigGameStuff();

    // Update display, next frame, with current state.
    EdgeDisplay();

    // this also runs the responder chain via ProcessInputEvents
    int counts = TryRunTicCommands();

    // run the tics
    for (; counts > 0; counts--)
    {
        // run a step in the physics (etc)
        GameTicker();

        // user interface stuff (skull anim, etc)
        ConsoleTicker();        
        SoundTicker();
        MusicTicker();

        // process mouse and keyboard events
        NetworkUpdate();
    }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
