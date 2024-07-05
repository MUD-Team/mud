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

#include "am_map.h"
#include "con_gui.h"
#include "con_main.h"
#include "con_var.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "e_input.h"
#include "edge_profiling.h"
#include "epi_file.h"
#include "epi_filesystem.h"
#include "epi_sdl.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "f_finale.h"
#include "f_interm.h"
#include "g_game.h"
#include "hu_draw.h"
#include "hu_stuff.h"
#include "i_defs_gl.h"
#include "i_movie.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_cheat.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_random.h"
#include "n_network.h"
#include "p_setup.h"
#include "p_spec.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "s_music.h"
#include "s_sound.h"
#include "script/compat/lua_compat.h"
#include "sv_main.h"
#include "version.h"
#include "w_files.h"
#include "w_model.h"
#include "w_sprite.h"
#include "w_texture.h"
#include "w_wad.h"

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

FILE *log_file   = nullptr;
FILE *debug_file = nullptr;

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

bool mus_pause_stop  = false;

std::string configuration_file;
std::string epkfile;

std::string cache_directory;
std::string game_directory;
std::string home_directory;
std::string save_directory;
std::string screenshot_directory;

// not using EDGE_DEFINE_CONSOLE_VARIABLE here since var name != cvar name
ConsoleVariable m_language("language", "ENGLISH", kConsoleVariableFlagArchive);

EDGE_DEFINE_CONSOLE_VARIABLE(log_filename, "edge-classic.log", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(config_filename, "edge-classic.cfg", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(debug_filename, "debug.txt", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(game_name, "EDGE-Classic", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(edge_version, "1.38", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(team_name, "EDGE Team", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(application_name, "EDGE-Classic", kConsoleVariableFlagNoReset)
EDGE_DEFINE_CONSOLE_VARIABLE(homepage, "https://edge-classic.github.io", kConsoleVariableFlagNoReset)

EDGE_DEFINE_CONSOLE_VARIABLE_CLAMPED(title_scaling, "0", kConsoleVariableFlagArchive, 0, 1)

EDGE_DEFINE_CONSOLE_VARIABLE(force_infighting, "0", kConsoleVariableFlagArchive)

EDGE_DEFINE_CONSOLE_VARIABLE(ddf_strict, "0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(ddf_lax, "0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(ddf_quiet, "0", kConsoleVariableFlagArchive)

EDGE_DEFINE_CONSOLE_VARIABLE(skip_intros, "0", kConsoleVariableFlagArchive)

static const Image *loading_image = nullptr;
const Image        *menu_backdrop = nullptr;

static void TitleDrawer(void);

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
        HUDFrameSetup();
        if (loading_image)
        {
            if (title_scaling.d_) // Fill Border
                HUDStretchImage(-320, -200, 960, 600, loading_image, 0, 0);
            HUDDrawImageTitleWS(loading_image);
            HUDSolidBox(25, 25, 295, 175, SG_BLACK_RGBA32);
        }
        int y = 26;
        for (int i = 0; i < (int)startup_messages_.size(); i++)
        {
            if (startup_messages_[i].size() > 32)
                HUDDrawText(26, y, startup_messages_[i].substr(0, 29).append("...").c_str());
            else
                HUDDrawText(26, y, startup_messages_[i].c_str());
            y += 10;
        }

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
    CheckBooleanParameter("rotate_map", &rotate_map, false);
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

    CheckBooleanParameter("automap_keydoor_blink", &automap_keydoor_blink, false);

    if (FindArgument("infight") > 0)
        force_infighting = 1;

    if (FindArgument("dlights") > 0)
        use_dynamic_lights = 1;
    else if (FindArgument("nodlights") > 0)
        use_dynamic_lights = 0;

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

    epi::File *data = OpenFileFromPack("/version.txt");

    if (!data)
        FatalError("Version file not found. Get edge_defs.epk at "
                   "https://github.com/edge-classic/EDGE-classic");

    // parse version number
    std::string verstring = data->ReadText();
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

static void DisplayPauseImage(void)
{
    static const Image *pause_image = nullptr;

    if (!pause_image)
        pause_image = ImageLookup("M_PAUSE");

    // make sure image is centered horizontally

    float w = pause_image->ScaledWidthActual();
    float h = pause_image->ScaledHeightActual();

    float x = 160 - w / 2;
    float y = 10;

    HUDStretchImage(x, y, w, h, pause_image, 0.0, 0.0);
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

    HUDFrameSetup();

    switch (game_state)
    {
    case kGameStateLevel:
        PaletteTicker();

        LuaRunHUD();

        HUDDrawer();
        break;

    case kGameStateIntermission:
        IntermissionDrawer();
        break;

    case kGameStateFinale:
        FinaleDrawer();
        break;

    case kGameStateTitleScreen:
        TitleDrawer();
        break;

    case kGameStateNothing:
        break;
    }

    if (paused)
        DisplayPauseImage();

    // menus go directly to the screen
    MenuDrawer(); // menu is drawn even on top of everything (except console)

    // process mouse and keyboard events
    NetworkUpdate();

    ConsoleDrawer();

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

//
//  TITLE LOOP
//
static int title_game;
static int title_pic;
static int title_countdown;

static const Image *title_image = nullptr;

static void TitleDrawer(void)
{
    if (title_image)
    {
        if (title_scaling.d_) // Fill Border
            HUDStretchImage(-320, -200, 960, 600, title_image, 0, 0);
        HUDDrawImageTitleWS(title_image);
    }
    else
    {
        HUDSolidBox(0, 0, 320, 200, SG_BLACK_RGBA32);
    }
}

//
// This cycles through the title sequences.
// -KM- 1998/12/16 Fixed for DDF.
//
void PickLoadingScreen(void)
{
    // force pic overflow -> first available titlepic
    title_game = gamedefs.size() - 1;
    title_pic  = 29999;

    // prevent an infinite loop
    for (int loop = 0; loop < 100; loop++)
    {
        GameDefinition *g = gamedefs[title_game];
        EPI_ASSERT(g);

        if (title_pic >= (int)g->titlepics_.size())
        {
            title_game = (title_game + 1) % (int)gamedefs.size();
            title_pic  = 0;
            continue;
        }

        // ignore non-existing episodes.  Doesn't include title-only ones
        // like [EDGE].
        if (title_pic == 0 && g->firstmap_ != "")
        {
            MapDefinition *ddf_map_name_check = mapdefs.Lookup(g->firstmap_.c_str());
            if (ddf_map_name_check)
            {
                if (CheckLumpNumberForName(ddf_map_name_check->lump_.c_str()) == -1)
                {
                    title_game = (title_game + 1) % gamedefs.size();
                    title_pic  = 0;
                    continue;
                }
            }
            else if (CheckLumpNumberForName(g->firstmap_.c_str()) == -1)
            {
                title_game = (title_game + 1) % gamedefs.size();
                title_pic  = 0;
                continue;
            }
        }

        // ignore non-existing images
        loading_image = ImageLookup(g->titlepics_[title_pic].c_str(), kImageNamespaceGraphic, kImageLookupNull);

        if (!loading_image)
        {
            title_pic++;
            continue;
        }

        // found one !!
        title_game = gamedefs.size() - 1;
        title_pic  = 29999;
        return;
    }

    // not found
    title_game    = gamedefs.size() - 1;
    title_pic     = 29999;
    loading_image = nullptr;
}

//
// Find and create a desaturated version of the first
// titlepic corresponding to a gamedef with actual maps.
// This is used as the menu backdrop
//
static void PickMenuBackdrop(void)
{
    // force pic overflow -> first available titlepic
    title_game = gamedefs.size() - 1;
    title_pic  = 29999;

    // prevent an infinite loop
    for (int loop = 0; loop < 100; loop++)
    {
        GameDefinition *g = gamedefs[title_game];
        EPI_ASSERT(g);

        if (title_pic >= (int)g->titlepics_.size())
        {
            title_game = (title_game + 1) % (int)gamedefs.size();
            title_pic  = 0;
            continue;
        }

        // ignore non-existing episodes.
        if (title_pic == 0 && (g->firstmap_ == "" || CheckLumpNumberForName(g->firstmap_.c_str()) == -1))
        {
            title_game = (title_game + 1) % gamedefs.size();
            title_pic  = 0;
            continue;
        }

        // ignore non-existing images
        const Image *menu_image =
            ImageLookup(g->titlepics_[title_pic].c_str(), kImageNamespaceGraphic, kImageLookupNull);

        if (!menu_image)
        {
            title_pic++;
            continue;
        }

        // found one !!
        title_game                       = gamedefs.size() - 1;
        title_pic                        = 29999;
        Image *new_backdrop              = new Image;
        new_backdrop->name_              = menu_image->name_;
        new_backdrop->actual_height_     = menu_image->actual_height_;
        new_backdrop->actual_width_      = menu_image->actual_width_;
        new_backdrop->cache_             = menu_image->cache_;
        new_backdrop->is_empty_          = menu_image->is_empty_;
        new_backdrop->is_font_           = menu_image->is_font_;
        new_backdrop->offset_x_          = menu_image->offset_x_;
        new_backdrop->offset_y_          = menu_image->offset_y_;
        new_backdrop->opacity_           = menu_image->opacity_;
        new_backdrop->height_ratio_      = menu_image->height_ratio_;
        new_backdrop->width_ratio_       = menu_image->width_ratio_;
        new_backdrop->scale_x_           = menu_image->scale_x_;
        new_backdrop->scale_y_           = menu_image->scale_y_;
        new_backdrop->source_            = menu_image->source_;
        new_backdrop->source_palette_    = menu_image->source_palette_;
        new_backdrop->source_type_       = menu_image->source_type_;
        new_backdrop->total_height_      = menu_image->total_height_;
        new_backdrop->total_width_       = menu_image->total_width_;
        new_backdrop->animation_.current = new_backdrop;
        new_backdrop->grayscale_         = true;
        menu_backdrop                    = new_backdrop;
        return;
    }

    // if we get here just use the loading image if it exists
    title_game = gamedefs.size() - 1;
    title_pic  = 29999;
    if (loading_image)
    {
        Image *new_backdrop              = new Image;
        new_backdrop->name_              = loading_image->name_;
        new_backdrop->actual_height_     = loading_image->actual_height_;
        new_backdrop->actual_width_      = loading_image->actual_width_;
        new_backdrop->cache_             = loading_image->cache_;
        new_backdrop->is_empty_          = loading_image->is_empty_;
        new_backdrop->is_font_           = loading_image->is_font_;
        new_backdrop->offset_x_          = loading_image->offset_x_;
        new_backdrop->offset_y_          = loading_image->offset_y_;
        new_backdrop->opacity_           = loading_image->opacity_;
        new_backdrop->height_ratio_      = loading_image->height_ratio_;
        new_backdrop->width_ratio_       = loading_image->width_ratio_;
        new_backdrop->scale_x_           = loading_image->scale_x_;
        new_backdrop->scale_y_           = loading_image->scale_y_;
        new_backdrop->source_            = loading_image->source_;
        new_backdrop->source_palette_    = loading_image->source_palette_;
        new_backdrop->source_type_       = loading_image->source_type_;
        new_backdrop->total_height_      = loading_image->total_height_;
        new_backdrop->total_width_       = loading_image->total_width_;
        new_backdrop->animation_.current = new_backdrop;
        new_backdrop->grayscale_         = true;
        menu_backdrop                    = new_backdrop;
    }
    else
        menu_backdrop = nullptr;
}

//
// This cycles through the title sequences.
// -KM- 1998/12/16 Fixed for DDF.
//
void AdvanceTitle(void)
{
    title_pic++;

    // prevent an infinite loop
    for (int loop = 0; loop < 100; loop++)
    {
        GameDefinition *g = gamedefs[title_game];
        EPI_ASSERT(g);

        // Only play title movies once
        if (!g->titlemovie_.empty() && !g->movie_played_)
        {
            if (skip_intros.d_)
                g->movie_played_ = true;
            else
            {
                PlayMovie(g->titlemovie_);
                g->movie_played_ = true;
            }
            continue;
        }

        if (show_old_config_warning && startup_progress.IsEmpty())
        {
            StartMenuMessage(language["OldConfig"], nullptr, false);
            show_old_config_warning = false;
        }

        if (title_pic >= (int)g->titlepics_.size())
        {
            title_game = (title_game + 1) % (int)gamedefs.size();
            title_pic  = 0;
            continue;
        }

        // ignore non-existing episodes.  Doesn't include title-only ones
        // like [EDGE].
        if (title_pic == 0 && g->firstmap_ != "")
        {
            MapDefinition *ddf_map_name_check = mapdefs.Lookup(g->firstmap_.c_str());
            if (ddf_map_name_check)
            {
                if (CheckLumpNumberForName(ddf_map_name_check->lump_.c_str()) == -1)
                {
                    title_game = (title_game + 1) % gamedefs.size();
                    title_pic  = 0;
                    continue;
                }
            }
            else if (CheckLumpNumberForName(g->firstmap_.c_str()) == -1)
            {
                title_game = (title_game + 1) % gamedefs.size();
                title_pic  = 0;
                continue;
            }
        }

        // ignore non-existing images
        title_image = ImageLookup(g->titlepics_[title_pic].c_str(), kImageNamespaceGraphic, kImageLookupNull);

        if (!title_image)
        {
            title_pic++;
            continue;
        }

        // found one !!
        if (title_pic == 0 && g->titlemusic_ > 0)
            ChangeMusic(g->titlemusic_, false);

        title_countdown = g->titletics_ * (double_framerate.d_ ? 2 : 1);
        return;
    }

    // not found

    title_image     = nullptr;
    title_countdown = kTicRate * (double_framerate.d_ ? 2 : 1);
}

void StartTitle(void)
{
    game_action = kGameActionNothing;
    game_state  = kGameStateTitleScreen;

    paused = false;

    title_countdown = 1;

    AdvanceTitle();
}

void TitleTicker(void)
{
    if (title_countdown > 0)
    {
        title_countdown--;

        if (title_countdown == 0)
            AdvanceTitle();
    }
}

//
// Detects which directories to search for DDFs, WADs and other files in.
//
// -ES- 2000/01/01 Written.
//
static void InitializeDirectories(void)
{
    // Get the App Directory from parameter.

    // Note: This might need adjusting for Apple
    std::string s = SDL_GetBasePath();

    game_directory = s;
    s              = ArgumentValue("game");
    if (!s.empty())
        game_directory = s;

    // add parameter file "appdir/parms" if it exists.
    std::string parms = epi::PathAppend(game_directory, "parms");

    if (epi::TestFileAccess(parms))
    {
        // Insert it right after the app parameter
        ApplyResponseFile(parms);
    }

    // config file - check for portable config
    s = ArgumentValue("config");
    if (!s.empty())
    {
        configuration_file = s;
    }
    else
    {
        configuration_file = epi::PathAppend(game_directory, config_filename.s_);
        if (epi::TestFileAccess(configuration_file) || FindArgument("portable") > 0)
            home_directory = game_directory;
        else
            configuration_file.clear();
    }

    if (home_directory.empty())
    {
        s = ArgumentValue("home");
        if (!s.empty())
            home_directory = s;
    }

#ifdef _WIN32
    if (home_directory.empty())
        home_directory = SDL_GetPrefPath(nullptr, application_name.c_str());
#else
    if (home_directory.empty())
        home_directory = SDL_GetPrefPath(team_name.c_str(), application_name.c_str());
#endif

    if (!epi::IsDirectory(home_directory))
    {
        if (!epi::MakeDirectory(home_directory))
            FatalError("InitializeDirectories: Could not create directory at %s!\n", home_directory.c_str());
    }

    if (configuration_file.empty())
        configuration_file = epi::PathAppend(home_directory, config_filename.s_);

    // edge_defs.epk file
    s = ArgumentValue("defs");
    if (!s.empty())
    {
        epkfile = s;
    }
    else
    {
        std::string defs_test = epi::PathAppend(game_directory, "edge_defs");
        if (epi::IsDirectory(defs_test))
            epkfile = defs_test;
        else
            epkfile.append(".epk");
    }

    // cache directory
    cache_directory = epi::PathAppend(home_directory, kCacheDirectory);

    if (!epi::IsDirectory(cache_directory))
        epi::MakeDirectory(cache_directory);

    // savegame directory
    save_directory = epi::PathAppend(home_directory, kSaveGameDirectory);

    if (!epi::IsDirectory(save_directory))
        epi::MakeDirectory(save_directory);

    SaveClearSlot("current");

    // screenshot directory
    screenshot_directory = epi::PathAppend(home_directory, kScreenshotDirectory);

    if (!epi::IsDirectory(screenshot_directory))
        epi::MakeDirectory(screenshot_directory);
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
    // if we get here, clear and return the empty string
    value.clear();
    return value;
}

// If a valid EDGEGAME is found, parse and return the game name
static std::string CheckPackForGameFiles(std::string check_pack, FileKind check_kind)
{
    DataFile *check_pack_df = new DataFile(check_pack, check_kind);
    EPI_ASSERT(check_pack_df);
    PopulatePackOnly(check_pack_df);
    std::string title;
    if (FindStemInPack(check_pack_df->pack_, "EDGEGAME"))
    {
        epi::File *eg_file = OpenPackMatch(check_pack_df->pack_, "EDGEGAME", {".txt", ".cfg"});
        if (!eg_file)
        {
            delete check_pack_df;
            return title;
        }
        std::string edge_game;
        edge_game.resize(eg_file->GetLength());
        eg_file->Read(edge_game.data(), eg_file->GetLength());
        delete eg_file;
        delete check_pack_df;
        epi::Lexer lex(edge_game);
        title = ParseEdgeGameFile(lex);
        return title;
    }
    return title;
}

//
// Adds main game content and edge_defs folder/EPK
//
static void IdentifyVersion(void)
{
    if (epi::IsDirectory(epkfile))
        AddDataFile(epkfile, kFileKindEFolder);
    else
    {
        if (!epi::TestFileAccess(epkfile))
            FatalError("IdentifyVersion: Could not find required %s.%s!\n", kRequiredEPK, "epk");
        AddDataFile(epkfile, kFileKindEEPK);
    }

    LogDebug("- Identify Version\n");

    // Check -iwad parameter, find out if it is the IWADs directory
    std::string              iwad_par;

    std::string s = ArgumentValue("game");

    if (s.empty()) // legacy/potential launcher compat
        s = ArgumentValue("iwad");

    iwad_par = s;

    // If explicitly passed a parameter for the game selection, validate it and treat
    // as a fatal error if the path specified is not a valid game
    if (!iwad_par.empty())
    {
        std::string game_check;
        if (epi::IsDirectory(iwad_par))
        {
            game_check = CheckPackForGameFiles(iwad_par, kFileKindIFolder);
            if (game_check.empty())
                FatalError("Folder %s passed via -game parameter, but no "
                           "EDGEGAME file detected!\n",
                           iwad_par.c_str());
            else
            {
                game_name = game_check;
                AddDataFile(iwad_par, kFileKindIFolder);
                LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                return;
            }
        }
        else if (epi::StringCaseCompareASCII(epi::GetExtension(iwad_par), ".epk") == 0)
        {
            game_check = CheckPackForGameFiles(iwad_par, kFileKindIPK);
            if (game_check.empty())
                FatalError("EPK %s passed via -game parameter, but no "
                           "EDGEGAME file detected!\n",
                           iwad_par.c_str());
            else
            {
                game_name = game_check;
                AddDataFile(iwad_par, kFileKindIPK);
                LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                return;
            }
        }
        else if (epi::StringCaseCompareASCII(epi::GetExtension(iwad_par), ".wad") == 0)
        {
            epi::File *game_test = epi::FileOpen(iwad_par, epi::kFileAccessRead | epi::kFileAccessBinary);
            game_check           = CheckForEdgeGameLump(game_test);
            delete game_test;
            if (game_check.empty())
                FatalError("WAD %s passed via -gane parameter, but no "
                           "EDGEGAME lump detected!\n",
                           iwad_par.c_str());
            else
            {
                game_name = game_check;
                AddDataFile(iwad_par, kFileKindIWAD);
                LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                return;
            }
        }
        else
            FatalError("%s is not a valid extension for a game file! (%s)\n", epi::GetExtension(iwad_par).c_str(), iwad_par.c_str());
    }
    else
    {
        // In the absence of the -game or -iwad parameter, check files/dirs added via
        // drag-and-drop for valid games and remove them from the arg list if they
        // are valid to avoid them potentially being added as PWADs.
        // This will use the first valid file/folder found, if any.
        for (size_t p = 1; p < program_argument_list.size() && !ArgumentIsOption(p); p++)
        {
            std::string dnd        = program_argument_list[p];
            std::string game_check;
            if (epi::IsDirectory(dnd))
            {
                game_check = CheckPackForGameFiles(dnd, kFileKindIFolder);
                if (!game_check.empty())
                {
                    game_name = game_check;
                    AddDataFile(dnd, kFileKindIFolder);
                    program_argument_list.erase(program_argument_list.begin() + p--);
                    LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                    return;
                }
            }
            else if (epi::StringCaseCompareASCII(epi::GetExtension(dnd), ".epk") == 0)
            {
                game_check = CheckPackForGameFiles(dnd, kFileKindIPK);
                if (!game_check.empty())
                {
                    game_name = game_check;
                    AddDataFile(dnd, kFileKindIPK);
                    program_argument_list.erase(program_argument_list.begin() + p--);
                    LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                    return;
                }
            }
            else if (epi::StringCaseCompareASCII(epi::GetExtension(dnd), ".wad") == 0)
            {
                epi::File *game_test = epi::FileOpen(dnd, epi::kFileAccessRead | epi::kFileAccessBinary);
                game_check           = CheckForEdgeGameLump(game_test);
                delete game_test;
                if (!game_check.empty())
                {
                    game_name = game_check;
                    AddDataFile(dnd, kFileKindIWAD);
                    program_argument_list.erase(program_argument_list.begin() + p--);
                    LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                    return;
                }
            }
        }
    }

    // If we have made it here, attempt to autodetect a valid game in the user's
    // home and game directories
    std::string location;
    int max = 1;

    if (epi::StringCompare(game_directory, home_directory) != 0)
        max++;

    for (int i = 0; i < max; i++)
    {
        location = (i == 0 ? game_directory : home_directory);

        std::vector<epi::DirectoryEntry> fsd;

        std::string game_check;

        if (ReadDirectory(fsd, location, "*.wad"))
        {
            for (size_t j = 0; j < fsd.size(); j++)
            {
                if (!fsd[j].is_dir)
                {
                    epi::File *game_test =
                        epi::FileOpen(fsd[j].name, epi::kFileAccessRead | epi::kFileAccessBinary);
                    game_check = CheckForEdgeGameLump(game_test);
                    delete game_test;
                    if (!game_check.empty())
                    {
                        game_name = game_check;
                        AddDataFile(fsd[j].name, kFileKindIWAD);
                        LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                        return;
                    }
                }
            }
        }
        if (ReadDirectory(fsd, location, "*.epk"))
        {
            for (size_t j = 0; j < fsd.size(); j++)
            {
                if (!fsd[j].is_dir)
                {
                    game_check = CheckPackForGameFiles(fsd[j].name, kFileKindIPK);
                    if (!game_check.empty())
                    {
                        game_name = game_check;
                        AddDataFile(fsd[j].name, kFileKindIPK);
                        LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                        return;
                    }
                }
            }
        }
        // Check directories (only at top level of home/game directory)
        if (ReadDirectory(fsd, location, "*.*"))
        {
            for (size_t j = 0; j < fsd.size(); j++)
            {
                if (fsd[j].is_dir)
                {
                    game_check = CheckPackForGameFiles(fsd[j].name, kFileKindIFolder);
                    if (!game_check.empty())
                    {
                        game_name = game_check;
                        AddDataFile(fsd[j].name, kFileKindIFolder);
                        LogDebug("LOADED GAME = [ %s ]\n", game_name.c_str());
                        return;
                    }
                }
            }
        }
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
    LogDebug("[Debug file created at %s]\n\n", timebuf);

    LogPrint("%s v%s compiled on " __DATE__ " at " __TIME__ "\n", application_name.c_str(), edge_version.c_str());
    LogPrint("%s homepage is at %s\n", application_name.c_str(), homepage.c_str());

    LogPrint("Executable path: '%s'\n", executable_path.c_str());

    DumpArguments();
}

static void SetupLogAndDebugFiles(void)
{
    // -AJA- 2003/11/08 The log file gets all ConsolePrints, LogPrints,
    //                  LogWarnings and FatalErrors.

    std::string log_fn   = epi::PathAppend(home_directory, log_filename.s_);
    std::string debug_fn = epi::PathAppend(home_directory, debug_filename.s_);

    log_file   = nullptr;
    debug_file = nullptr;

    if (FindArgument("nolog") < 0)
    {
        log_file = epi::FileOpenRaw(log_fn, epi::kFileAccessWrite);

        if (!log_file)
            FatalError("[EdgeStartup] Unable to create log file\n");
    }

    //
    // -ACB- 1998/09/06 Only used for debugging.
    //                  Moved here to setup debug file for DDF Parsing...
    //
    // -ES- 1999/08/01 Debugfiles can now be used without -DDEVELOPERS, and
    //                 then logs all the ConsolePrints, LogPrints and
    //                 FatalErrors.
    //
    // -ACB- 1999/10/02 Don't print to console, since we don't have a console
    // yet.

    /// int p = FindArgument("debug");
    if (true)
    {
        debug_file = epi::FileOpenRaw(debug_fn, epi::kFileAccessWrite);

        if (!debug_file)
            FatalError("[EdgeStartup] Unable to create debug_file");
    }
}

static void AddSingleCommandLineFile(std::string name, bool ignore_unknown)
{
    if (epi::IsDirectory(name))
    {
        AddDataFile(name, kFileKindFolder);
        return;
    }

    std::string ext = epi::GetExtension(name);

    epi::StringLowerASCII(ext);

    if (ext == ".edm")
        FatalError("Demos are not supported\n");
    else if (ext == ".rts")
        FatalError("Radius Trigger Scripts are not supported\n");

    FileKind kind;

    if (ext == ".wad")
        kind = kFileKindPWAD;
    else if (ext == ".pk3" || ext == ".epk" || ext == ".zip" || ext == ".vwad")
        kind = kFileKindEPK;
    else if (ext == ".ddf" || ext == ".ldf")
        kind = kFileKindDDF;
    else
    {
        if (!ignore_unknown)
            FatalError("unknown file type: %s\n", name.c_str());
        return;
    }

    std::string filename = epi::PathAppendIfNotAbsolute(game_directory, name);
    AddDataFile(filename, kind);
}

static void AddCommandLineFiles(void)
{
    // first handle "loose" files (arguments before the first option)

    int p;

    for (p = 1; p < int(program_argument_list.size()) && !ArgumentIsOption(p); p++)
    {
        AddSingleCommandLineFile(program_argument_list[p], false);
    }

    // next handle the -file option (we allow multiple uses)

    p = FindArgument("file");

    while (p > 0 && p < int(program_argument_list.size()) &&
           (!ArgumentIsOption(p) || epi::StringCompare(program_argument_list[p], "-file") == 0))
    {
        // the parms after p are wadfile/lump names,
        // go until end of parms or another '-' preceded parm
        if (!ArgumentIsOption(p))
            AddSingleCommandLineFile(program_argument_list[p], false);

        p++;
    }

    // directories....

    p = FindArgument("dir");

    while (p > 0 && p < int(program_argument_list.size()) &&
           (!ArgumentIsOption(p) || epi::StringCompare(program_argument_list[p], "-dir") == 0))
    {
        // the parms after p are directory names,
        // go until end of parms or another '-' preceded parm
        if (!ArgumentIsOption(p))
        {
            std::string dirname = epi::PathAppendIfNotAbsolute(game_directory, program_argument_list[p]);
            AddDataFile(dirname, kFileKindFolder);
        }

        p++;
    }

    // handle -ddf option (backwards compatibility)

    std::string ps = ArgumentValue("ddf");

    if (!ps.empty())
    {
        std::string filename = epi::PathAppendIfNotAbsolute(game_directory, ps);
        AddDataFile(filename, kFileKindFolder);
    }
}

static void AddAutoload(void)
{
    std::vector<epi::DirectoryEntry> fsd;
    std::string                      folder = epi::PathAppend(game_directory, "autoload");

    if (!ReadDirectory(fsd, folder, "*.*"))
    {
        LogWarning("Failed to read %s directory!\n", folder.c_str());
    }
    else
    {
        for (size_t i = 0; i < fsd.size(); i++)
        {
            if (!fsd[i].is_dir)
                AddSingleCommandLineFile(fsd[i].name, true);
        }
    }
    fsd.clear();

    // Check if autoload folder stuff is in home_directory as well, make the
    // folder/subfolder if they don't exist (in home_directory only)
    folder = epi::PathAppend(home_directory, "autoload");
    if (!epi::IsDirectory(folder))
        epi::MakeDirectory(folder);

    if (!ReadDirectory(fsd, folder, "*.*"))
    {
        LogWarning("Failed to read %s directory!\n", folder.c_str());
    }
    else
    {
        for (size_t i = 0; i < fsd.size(); i++)
        {
            if (!fsd[i].is_dir)
                AddSingleCommandLineFile(fsd[i].name, true);
        }
    }
}

static void InitializeDDF(void)
{
    LogDebug("- Initialising DDF\n");

    DDFInit();
}

void EdgeShutdown(void)
{
    StopMusic();

    // Pause to allow sounds to finish
    for (int loop = 0; loop < 30; loop++)
    {
        SoundTicker();
        SleepForMilliseconds(50);
    }

    LevelShutdown();
    ShutdownSound();
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

    SetupLogAndDebugFiles();

    ShowDateAndVersion();

    LoadDefaults();

    HandleProgramArguments();
    SetGlobalVariables();

    DoSystemStartup();

    InitializeDDF();
    IdentifyVersion();
    AddAutoload();
    AddCommandLineFiles();
    CheckTurbo();

    ProcessMultipleFiles();
    DDFParseEverything();
    // Must be done after WAD and DDF loading to check for potential
    // overrides of lump-specific image/sound/DDF defines
    DoPackSubstitutions();
    StartupMusic();
    InitializePalette();

    DDFCleanUp();
    SetLanguage();

    InitializeFlats();
    InitializeTextures();
    CreateUserImages();
    PickLoadingScreen();
    PickMenuBackdrop();

    HUDInit();
    ConsoleStart();
    SpecialWadVerify();
    BuildXGLNodes();
    ShowNotice();

    PrecacheSounds();
    InitializeSprites();
    ProcessTXHINamespaces();
    InitializeModels();

    MenuInitialize();
    RendererStartup();
    PlayerStateInit();
    InitializeSwitchList();
    InitializeAnimations();
    InitializeSound();
    NetworkInitialize();
    CheatInitialize();
    LuaInit();
    LuaLoadScripts();
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
            warp_deathmatch = HMM_MAX(1, atoi(program_argument_list[pp + 1].c_str()));

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
        StartTitle();
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
void EdgeMain(int argc, const char **argv)
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

    while (!(app_state & kApplicationPendingQuit))
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
        MenuTicker();
        SoundTicker();
        MusicTicker();

        // process mouse and keyboard events
        NetworkUpdate();
    }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
