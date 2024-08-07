//----------------------------------------------------------------------------
//  EDGE Misc: Screenshots, Menu and defaults Code
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
// -MH- 1998/07/02  Added key_fly_up and key_fly_down
// -MH- 1998/07/02 "shootupdown" --> "true_3d_gameplay"
// -ACB- 2000/06/02 Removed Control Defaults
//

#include "m_misc.h"

#include <stdarg.h>

#include "con_main.h"
#include "defaults.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_input.h"
#include "e_player.h"
#include "epi.h"
#include "epi_endian.h"
#include "epi_filesystem.h"
#include "epi_lexer.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "g_game.h"
#include "im_data.h"
#include "im_funcs.h"
#include "m_argv.h"
#include "n_network.h"
#include "p_spec.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_modes.h"
#include "s_blit.h"
#include "s_sound.h"
#include "version.h"

extern ConsoleVariable config_filename;
extern epi::File *log_file;

bool show_old_config_warning = false;

extern ConsoleVariable midi_soundfont;
int                    var_sound_stereo      = 0;
int                    sound_mixing_channels = 0;
int                    screen_hud            = 0;

static bool done_first_init = false;

static ConfigurationDefault defaults[] = {
    {kConfigInteger, "screenwidth", &current_screen_width, EDGE_DEFAULT_SCREENWIDTH},
    {kConfigInteger, "screenheight", &current_screen_height, EDGE_DEFAULT_SCREENHEIGHT},
    {kConfigInteger, "screendepth", &current_screen_depth, EDGE_DEFAULT_SCREENBITS},
    {kConfigInteger, "displaymode", &current_window_mode, EDGE_DEFAULT_DISPLAYMODE},

    {kConfigInteger, "sound_stereo", &var_sound_stereo, EDGE_DEFAULT_SOUND_STEREO},
    {kConfigInteger, "mix_channels", &sound_mixing_channels, EDGE_DEFAULT_MIX_CHANNELS},

    // -ES- 1998/11/28 Save fade settings
    {kConfigInteger, "reduce_flash", &reduce_flash, 0},
    {kConfigBoolean, "respawnsetting", &global_flags.enemy_respawn_mode, EDGE_DEFAULT_RES_RESPAWN},
    {kConfigBoolean, "items_respawn", &global_flags.items_respawn, EDGE_DEFAULT_ITEMRESPAWN},
    {kConfigBoolean, "respawn", &global_flags.enemies_respawn, EDGE_DEFAULT_RESPAWN},
    {kConfigBoolean, "fast_monsters", &global_flags.fast_monsters, EDGE_DEFAULT_FASTPARM},
    {kConfigEnum, "autoaim", &global_flags.autoaim, EDGE_DEFAULT_AUTOAIM},

    // -KM- 1998/07/21 Save the blood setting
    {kConfigBoolean, "blood", &global_flags.more_blood, EDGE_DEFAULT_MORE_BLOOD},
    {kConfigBoolean, "weaponkick", &global_flags.kicking, EDGE_DEFAULT_KICKING},
    {kConfigBoolean, "weaponswitch", &global_flags.weapon_switch, EDGE_DEFAULT_WEAPON_SWITCH},
    {kConfigInteger, "smoothing", &image_smoothing, EDGE_DEFAULT_USE_SMOOTHING},

    // -KM- 1998/09/01 Useless mouse/joy stuff removed,
    //                 analogue binding added
    {kConfigInteger, "mouse_axis_x", &mouse_x_axis, EDGE_DEFAULT_MOUSE_XAXIS},
    {kConfigInteger, "mouse_axis_y", &mouse_y_axis, EDGE_DEFAULT_MOUSE_YAXIS},

    {kConfigInteger, "joystick_axis1", &joystick_axis[0], 7},
    {kConfigInteger, "joystick_axis2", &joystick_axis[1], 6},
    {kConfigInteger, "joystick_axis3", &joystick_axis[2], 1},
    {kConfigInteger, "joystick_axis4", &joystick_axis[3], 4},

    {kConfigInteger, "screen_hud", &screen_hud, EDGE_DEFAULT_SCREEN_HUD},

    // -------------------- VARS --------------------

    {kConfigBoolean, "show_obituaries", &show_obituaries, 1},
    {kConfigBoolean, "precache_sound_effects", &precache_sound_effects, 1},

    // -------------------- KEYS --------------------

    {kConfigKey, "key_right", &key_right, EDGE_DEFAULT_KEY_RIGHT},
    {kConfigKey, "key_left", &key_left, EDGE_DEFAULT_KEY_LEFT},
    {kConfigKey, "key_up", &key_up, EDGE_DEFAULT_KEY_UP},
    {kConfigKey, "key_down", &key_down, EDGE_DEFAULT_KEY_DOWN},
    {kConfigKey, "key_look_up", &key_look_up, EDGE_DEFAULT_KEY_LOOKUP},
    {kConfigKey, "key_look_down", &key_look_down, EDGE_DEFAULT_KEY_LOOKDOWN},
    {kConfigKey, "key_look_center", &key_look_center, EDGE_DEFAULT_KEY_LOOKCENTER},

    // -ES- 1999/03/28 Zoom Key
    {kConfigKey, "key_zoom", &key_zoom, EDGE_DEFAULT_KEY_ZOOM},
    {kConfigKey, "key_strafe_left", &key_strafe_left, EDGE_DEFAULT_KEY_STRAFELEFT},
    {kConfigKey, "key_strafe_right", &key_strafe_right, EDGE_DEFAULT_KEY_STRAFERIGHT},

    // -ACB- for -MH- 1998/07/02 Flying Keys
    {kConfigKey, "key_fly_up", &key_fly_up, EDGE_DEFAULT_KEY_FLYUP},
    {kConfigKey, "key_fly_down", &key_fly_down, EDGE_DEFAULT_KEY_FLYDOWN},

    {kConfigKey, "key_fire", &key_fire, EDGE_DEFAULT_KEY_FIRE},
    {kConfigKey, "key_use", &key_use, EDGE_DEFAULT_KEY_USE},
    {kConfigKey, "key_strafe", &key_strafe, EDGE_DEFAULT_KEY_STRAFE},
    {kConfigKey, "key_speed", &key_speed, EDGE_DEFAULT_KEY_SPEED},
    {kConfigKey, "key_autorun", &key_autorun, EDGE_DEFAULT_KEY_AUTORUN},
    {kConfigKey, "key_next_weapon", &key_next_weapon, EDGE_DEFAULT_KEY_NEXTWEAPON},
    {kConfigKey, "key_previous_weapon", &key_previous_weapon, EDGE_DEFAULT_KEY_PREVWEAPON},

    {kConfigKey, "key_180", &key_180, EDGE_DEFAULT_KEY_180},
    {kConfigKey, "key_map", &key_map, EDGE_DEFAULT_KEY_MAP},
    {kConfigKey, "key_talk", &key_talk, EDGE_DEFAULT_KEY_TALK},
    {kConfigKey, "key_console", &key_console, EDGE_DEFAULT_KEY_CONSOLE},               // -AJA- 2007/08/15.
    {kConfigKey, "key_pause", &key_pause, kPause},                                     // -AJA- 2010/06/13.

    {kConfigKey, "key_second_attack", &key_second_attack, EDGE_DEFAULT_KEY_SECONDATK}, // -AJA- 2000/02/08.
    {kConfigKey, "key_third_attack", &key_third_attack, 0},                            //
    {kConfigKey, "key_fourth_attack", &key_fourth_attack, 0},                          //
    {kConfigKey, "key_reload", &key_reload, EDGE_DEFAULT_KEY_RELOAD},                  // -AJA- 2004/11/11.
    {kConfigKey, "key_action1", &key_action1, EDGE_DEFAULT_KEY_ACTION1},               // -AJA- 2009/09/07
    {kConfigKey, "key_action2", &key_action2, EDGE_DEFAULT_KEY_ACTION2},               // -AJA- 2009/09/07

    // -AJA- 2010/06/13: weapon and automap keys
    {kConfigKey, "key_weapon1", &key_weapons[1], '1'},
    {kConfigKey, "key_weapon2", &key_weapons[2], '2'},
    {kConfigKey, "key_weapon3", &key_weapons[3], '3'},
    {kConfigKey, "key_weapon4", &key_weapons[4], '4'},
    {kConfigKey, "key_weapon5", &key_weapons[5], '5'},
    {kConfigKey, "key_weapon6", &key_weapons[6], '6'},
    {kConfigKey, "key_weapon7", &key_weapons[7], '7'},
    {kConfigKey, "key_weapon8", &key_weapons[8], '8'},
    {kConfigKey, "key_weapon9", &key_weapons[9], '9'},
    {kConfigKey, "key_weapon0", &key_weapons[0], '0'},

    {kConfigKey, "key_inventory_previous", &key_inventory_previous, EDGE_DEFAULT_KEY_PREVINV},
    {kConfigKey, "key_inventory_use", &key_inventory_use, EDGE_DEFAULT_KEY_USEINV},
    {kConfigKey, "key_inventory_next", &key_inventory_next, EDGE_DEFAULT_KEY_NEXTINV},

    {kConfigKey, "key_show_players", &key_show_players, kFunction12},
};

static int total_defaults = sizeof(defaults) / sizeof(defaults[0]);

void SaveDefaults(void)
{
    epi::File *f = epi::FileOpen(config_filename.s_, epi::kFileAccessWrite);

    if (!f)
    {
        LogWarning("Couldn't open config file %s for writing.", config_filename.c_str());
        return; // can't write the file, but don't complain
    }

    f->WriteString(epi::StringFormat("#VERSION %d\n", kInternalConfigVersion));

    // console variables
    WriteConsoleVariables(f);

    // normal variables
    for (int i = 0; i < total_defaults; i++)
    {
        int v;

        switch (defaults[i].type)
        {
        case kConfigInteger:
            f->WriteString(epi::StringFormat("%s\t\t%i\n", defaults[i].name, *(int *)defaults[i].location));
            break;

        case kConfigBoolean:
            f->WriteString(epi::StringFormat("%s\t\t%i\n", defaults[i].name, *(bool *)defaults[i].location ? 1 : 0));
            break;

        case kConfigKey:
            v = *(int *)defaults[i].location;
            f->WriteString(epi::StringFormat("%s\t\t0x%X\n", defaults[i].name, v));
            break;
        }
    }

    delete f;
}

static void SetToBaseValue(ConfigurationDefault *def)
{
    switch (def->type)
    {
    case kConfigInteger:
    case kConfigKey:
        *(int *)(def->location) = def->default_value;
        break;

    case kConfigBoolean:
        *(bool *)(def->location) = def->default_value ? true : false;
        break;
    }
}

void ResetDefaults(int dummy, ConsoleVariable *dummy_cvar)
{
    (void)dummy;
    (void)dummy_cvar;
    for (int i = 0; i < total_defaults; i++)
    {
        // don't reset the first five entries except at startup
        if (done_first_init && i < 5)
            continue;

        SetToBaseValue(defaults + i);
    }

    ResetAllConsoleVariables();

    // Set default SF2 location in midi_soundfont CVAR
    // We can't store this as a CVAR default since it is path-dependent
    midi_soundfont = epi::SanitizePath(epi::PathAppend(game_directory, "soundfont/Default.sf2"));

    // Needed so that Smoothing/Upscaling is properly reset
    DeleteAllImages();

    done_first_init = true;
}

static void ParseConfigBlock(epi::Lexer &lex)
{
    for (;;)
    {
        std::string key;
        std::string value;

        epi::TokenKind tok = lex.Next(key);

        if (key == "/") // CVAR keys will start with this, but we need to discard it
            continue;

        if (tok == epi::kTokenEOF)
            return;

        if (tok == epi::kTokenError)
            FatalError("ParseConfig: error parsing file!\n");

        tok = lex.Next(value);

        // The last line of the config writer causes a weird blank key with an
        // EOF value, so just return here
        if (tok == epi::kTokenEOF)
            return;

        if (tok == epi::kTokenError)
            FatalError("ParseConfig: malformed value for key %s!\n", key.c_str());

        if (tok == epi::kTokenString)
        {
            std::string try_cvar = key;
            try_cvar.append(" ").append(value);
            TryConsoleCommand(try_cvar.c_str());
        }
        else if (tok == epi::kTokenNumber)
        {
            for (int i = 0; i < total_defaults; i++)
            {
                if (0 == epi::StringCompare(key.c_str(), defaults[i].name))
                {
                    if (defaults[i].type == kConfigBoolean)
                    {
                        *(bool *)defaults[i].location = epi::LexInteger(value) ? true : false;
                    }
                    else /* kConfigInteger and
                            kConfigKey */
                    {
                        *(int *)defaults[i].location = epi::LexInteger(value);
                    }
                    break;
                }
            }
        }
    }
}

static void ParseConfig(const std::string &data, bool check_config_version)
{
    epi::Lexer lex(data);

    // Check the first line of a config file for the #VERSION entry. If not
    // present, assume it is from a version that predates this concept
    if (check_config_version)
    {
        std::string    version;
        epi::TokenKind tok = lex.Next(version);

        if (tok != epi::kTokenSymbol || version != "#")
        {
            show_old_config_warning = true;
        }

        tok = lex.Next(version);

        if (tok != epi::kTokenIdentifier || version != "version")
        {
            show_old_config_warning = true;
        }

        tok = lex.Next(version);

        if (tok != epi::kTokenNumber || epi::LexInteger(version) < kInternalConfigVersion)
        {
            show_old_config_warning = true;
        }
    }

    for (;;)
    {
        std::string    section;
        epi::TokenKind tok = lex.Next(section);

        if (tok == epi::kTokenEOF)
            return;

        // process the block
        ParseConfigBlock(lex);
    }
}

void LoadDefaults(void)
{
    // set everything to base values
    ResetDefaults(0);

    LogPrint("LoadDefaults from %s\n", config_filename.c_str());

    epi::File *file = epi::FileOpen(config_filename.s_, epi::kFileAccessRead);

    if (!file)
    {
        LogWarning("Couldn't open config file %s for reading.\n", config_filename.c_str());
        LogWarning("Resetting config to RECOMMENDED values...\n");
        return;
    }
    // load the file into this string
    std::string data = file->ReadAsString();

    delete file;

    ParseConfig(data, true);

    return;
}

void TakeScreenshot(bool show_msg)
{
    const char *extension = "png";

    std::string fn;

    // find a file name to save it to
    for (int i = 1; i <= 9999; i++)
    {
        std::string base(epi::StringFormat("shot%02d.%s", i, extension));

        fn = epi::PathAppend("screenshot", base);

        if (!epi::FileExists(fn))
        {
            break; // should be able to use this name
        }
    }

    ImageData *img = new ImageData(current_screen_width, current_screen_height, 3);

    ReadScreen(0, 0, current_screen_width, current_screen_height, img->PixelAt(0, 0));

    // ReadScreen produces a bottom-up image, need to invert it
    img->Invert();

    bool result;

    result = SavePNG(fn, img);

    if (show_msg)
    {
        if (result)
            LogPrint("Captured to file: %s\n", fn.c_str());
        else
            LogPrint("Error saving file: %s\n", fn.c_str());
    }

    delete img;
}

void WarningOrError(const char *error, ...)
{
    // Either displays a warning or produces a fatal error, depending
    // on whether the "-strict" option is used.

    char message_buf[4096];

    message_buf[4095] = 0;

    va_list argptr;

    va_start(argptr, error);
    vsprintf(message_buf, error, argptr);
    va_end(argptr);

    // I hope nobody is printing strings longer than 4096 chars...
    EPI_ASSERT(message_buf[4095] == 0);

    if (strict_errors)
        FatalError("%s", message_buf);
    else if (!no_warnings)
        LogWarning("%s", message_buf);
}

void DebugOrError(const char *error, ...)
{
    // Either writes a debug message or produces a fatal error, depending
    // on whether the "-strict" option is used.

    char message_buf[4096];

    message_buf[4095] = 0;

    va_list argptr;

    va_start(argptr, error);
    vsprintf(message_buf, error, argptr);
    va_end(argptr);

    // I hope nobody is printing strings longer than 4096 chars...
    EPI_ASSERT(message_buf[4095] == 0);

    if (strict_errors)
        FatalError("%s", message_buf);
    else if (!no_warnings)
        LogDebug("%s", message_buf);
}

void LogDebug(const char *message, ...)
{
    //
    // -ACB- 1999/09/22: From #define to Procedure
    // -AJA- 2001/02/07: Moved here from platform codes.
    //
    if (!log_file)
        return;

    char message_buf[4096];

    message_buf[4095] = 0;

    // Print the message into a text string
    va_list argptr;

    va_start(argptr, message);
    vsprintf(message_buf, message, argptr);
    va_end(argptr);

    // I hope nobody is printing strings longer than 4096 chars...
    EPI_ASSERT(message_buf[4095] == 0);

    log_file->WriteString(message_buf);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
