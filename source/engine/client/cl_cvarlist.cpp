// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d7013c30215a6872874d95987b634b912ae90e7b $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Client console variables
//
//-----------------------------------------------------------------------------

#include "i_music.h"
#include "mud_includes.h"
#include "s_sound.h"

// Console
// -------

CVAR(print_stdout, "0", "Print console text to stdout", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(con_notifytime, "3.0", "Number of seconds to display messages to top of the HUD", CVARTYPE_FLOAT,
           CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 1.0f, 10.0f)

CVAR_RANGE(con_midtime, "3.0", "Number of seconds to display messages in the middle of the screen", CVARTYPE_FLOAT,
           CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 1.0f, 10.0f)

CVAR_RANGE(con_scrlock, "1", "", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 2.0f)

CVAR_RANGE(con_buffersize, "1024", "Size of console scroll-back buffer", CVARTYPE_INT,
           CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 512.0f, 65536.0f)

CVAR(con_coloredmessages, "1", "Activates colored messages in printed messages", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(message_showpickups, "1", "Show item pickup messages on the message line.", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(message_showobituaries, "0", "Show player death messages on the message line.", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

// Intermission
// ------------

// Determines whether to draw the scores on intermission.
CVAR(wi_oldintermission, "0",
     "Use Vanilla's intermission screen if there are 4 players or less on cooperative gamemodes.", CVARTYPE_BOOL,
     CVAR_CLIENTARCHIVE)

// Menus
// -----

CVAR_RANGE(ui_dimamount, "0.7", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(ui_dimcolor, "00 00 00", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

// Gameplay/Other
// --------------

CVAR(cl_connectalert, "1", "Plays a sound when a player joins", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(cl_disconnectalert, "1", "Plays a sound when a player quits", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(cl_chatsounds, "1",
           "Plays a sound when a chat message appears (0 = never, 1 = always, "
           "2 = only teamchat)",
           CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 2.0f)

CVAR_RANGE(cl_switchweapon, "1",
           "Switch upon weapon pickup (0 = never, 1 = always, "
           "2 = use weapon preferences, 3 = use PWO but holding fire cancels it)",
           CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 3.0f)

CVAR_RANGE(cl_weaponpref_fst, "0", "Weapon preference level for fists", CVARTYPE_BYTE,
           CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(cl_weaponpref_csw, "3", "Weapon preference level for chainsaw", CVARTYPE_BYTE,
           CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(cl_weaponpref_pis, "4", "Weapon preference level for pistol", CVARTYPE_BYTE,
           CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(cl_weaponpref_sg, "5", "Weapon preference level for shotgun", CVARTYPE_BYTE,
           CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(cl_weaponpref_ssg, "7", "Weapon preference level for super shotgun", CVARTYPE_BYTE,
           CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(cl_weaponpref_cg, "6", "Weapon preference level for chaingun", CVARTYPE_BYTE,
           CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(cl_weaponpref_rl, "1", "Weapon preference level for rocket launcher", CVARTYPE_BYTE,
           CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(cl_weaponpref_pls, "8", "Weapon preference level for plasma rifle", CVARTYPE_BYTE,
           CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(cl_weaponpref_bfg, "2", "Weapon preference level for BFG9000", CVARTYPE_BYTE,
           CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_FUNC_DECL(use_joystick, "1", "", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(joy_active, "0", "Selects the joystick device to use", CVARTYPE_INT,
               CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(joy_strafeaxis, "0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(joy_forwardaxis, "1", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(joy_turnaxis, "2", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(joy_lookaxis, "3", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(joy_sensitivity, "10.0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(joy_fastsensitivity, "15.0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(joy_freelook, "0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE)
CVAR(joy_invert, "0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE)

CVAR_RANGE(joy_deadzone, "0.20", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 0.75f)

CVAR_RANGE(joy_lefttrigger_deadzone, "0.2",
           "Sets the required pressure to trigger a press on the left trigger (Analog controllers only)",
           CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.01f, 1.0f)

CVAR_RANGE(joy_righttrigger_deadzone, "0.2",
           "Sets the required pressure to trigger a press on the right trigger (Analog controllers only)",
           CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.01f, 1.0f)

CVAR(show_messages, "1", "", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(mute_spectators, "0", "Mute spectators chat.", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(mute_enemies, "0", "Mute enemy players chat.", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

// Maximum number of clients who can connect to the server
CVAR(sv_maxclients, "0", "maximum clients who can connect to server", CVARTYPE_BYTE,
     CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Maximum amount of players who can join the game, others are spectators
CVAR(sv_maxplayers, "0", "maximum players who can join the game, others are spectators", CVARTYPE_BYTE,
     CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Maximum number of players that can be on a team
CVAR(sv_maxplayersperteam, "0", "Maximum number of players that can be on a team", CVARTYPE_BYTE,
     CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
CVAR_RANGE(sv_teamsinplay, "2", "Teams that are enabled", CVARTYPE_BYTE,
           CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE, 2.0f, 3.0f)

// Netcode Settings
// --------------

CVAR(cl_downloadsites,
     "https://static.allfearthesentinel.net/wads/ https://doomshack.org/wads/ "
     "http://grandpachuck.org/files/wads/ https://wads.doomleague.org/ "
     "http://files.funcrusher.net/wads/ https://doomshack.org/uploads/ "
     "https://doom.dogsoft.net/getwad.php?search=",
     "A list of websites to download WAD files from.  These websites are used if the "
     "server doesn't provide any websites to download files from, or the file can't be "
     "found on any of their sites.  The list of sites is separated by spaces.  These "
     "websites are tried in random order, and their WAD files must not be compressed "
     "with ZIP.",
     CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE_FUNC_DECL(cl_interp, "1", "Interpolate enemy player positions", CVARTYPE_INT,
                     CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 4.0f)

CVAR_RANGE(cl_prednudge, "0.70", "Smooth out collisions", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE,
           0.05f, 1.0f)

CVAR(cl_predictweapons, "1", "Draw weapon effects immediately", CVARTYPE_BOOL, CVAR_USERINFO | CVAR_CLIENTARCHIVE)

CVAR(cl_netgraph, "0", "Show a graph of network related statistics", CVARTYPE_BOOL, CVAR_NULL)

CVAR(cl_serverdownload, "1",
     "Enable or disable downloading game files and resources from the server"
     "(requires downloading enabled on server)",
     CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(cl_forcedownload, "0",
     "Forces the client to download the last WAD file when connecting "
     "to a server, even if the client already has that file "
     "(requires developer 1).",
     CVARTYPE_BOOL, CVAR_NULL)

// Client Preferences
// ------------------

CVAR_FUNC_DECL(cl_name, "Player", "", CVARTYPE_STRING, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(cl_color, "40 cf 00", "", CVARTYPE_STRING, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(cl_customcolor, "40 cf 00", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE)

CVAR(cl_colorpreset, "custom", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE)

CVAR(cl_gender, "male", "", CVARTYPE_STRING, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(cl_team, "blue", "", CVARTYPE_STRING, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE(cl_autoaim, "5000", "", CVARTYPE_FLOAT, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f,
           5000.0f)

CVAR(chasedemo, "0", "", CVARTYPE_BOOL, CVAR_NULL)

CVAR(cl_run, "1", "Always run", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE) // Always run? // [Toke - Defaults]

CVAR(in_autosr50, "1", "+strife activates automatic SR50", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(cl_showspawns, "0", "Show spawn points as particle fountains", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE | CVAR_LATCH)

// Mouse settings
// --------------

CVAR_FUNC_DECL(mouse_type, "1", "Use vanilla Doom or ZDoom mouse sensitivity scaling (DEPRECATED)", CVARTYPE_BYTE,
               CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE(mouse_sensitivity, "1.0", "Overall mouse sensitivity", CVARTYPE_FLOAT,
           CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 100.0f)

CVAR_FUNC_DECL(cl_mouselook, "1", "Look up or down with mouse", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(m_pitch, "1.0", "Vertical mouse sensitivity", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE,
           0.0f, 100.0f)

CVAR_RANGE(m_yaw, "1.0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 100.0f)

CVAR_RANGE(m_forward, "1.0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 100.0f)

CVAR_RANGE(m_side, "2.0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 100.0f)

CVAR(novert, "1", "Disable vertical mouse movement", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(invertmouse, "0", "Invert vertical mouse movement", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(lookstrafe, "0", "Strafe with mouse", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(m_filter, "0", "Smooth mouse input", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(hud_mousegraph, "0", "Display mouse values", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(idmypos, "0", "Shows current player position on map", CVARTYPE_BOOL, CVAR_NULL)

// Heads up display
// ----------------
CVAR(hud_bigfont, "0", "Use BIGFONT for certain HUD items - intended as a stopgap feature for streamers", CVARTYPE_BOOL,
     CVAR_CLIENTARCHIVE)

CVAR(hud_crosshairdim, "0", "Crosshair transparency", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(hud_crosshairscale, "1", "Crosshair scaling", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(hud_crosshairhealth, "1", "Color of crosshair represents health level", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(hud_gamemsgtype, "2", "Game message type", CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 1.0f,
           2.0f)

CVAR_RANGE(hud_revealsecrets, "1", "Get a notification if you or another player finds a secret.", CVARTYPE_BYTE,
           CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 3.0f)

CVAR(hud_scale, "1", "HUD scaling", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(hud_scalescoreboard, "0", "Scoreboard scaling", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE(hud_scaletext, "2", "Scaling multiplier for chat and midprint", CVARTYPE_BYTE,
           CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 1.0f, 4.0f)

CVAR_RANGE(hud_targetcount, "2", "Number of players to reveal", CVARTYPE_BYTE,
           CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 32.0f)

CVAR(hud_targetnames, "1", "Show names of players you're aiming at", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(hud_targethealth_debug, "0",
     "Show health of friendly players you're aiming at - this feature has known "
     "shortcomings with inaccurate health values and will be fixed in a future version "
     "of Odamex, enable at your peril",
     CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(hud_timer, "1", "Show the HUD timer:\n// 0: No Timer\n// 1: Count-down Timer\n// 2: Count-up timer", CVARTYPE_INT,
     CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(hud_speedometer, "0", "Show the HUD speedometer", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(hud_transparency, "1.0", "HUD transparency", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f,
           1.0f)

CVAR_RANGE(hud_anchoring, "1.0", "HUD anchoring (0.0: Center, 1.0: Corners)", CVARTYPE_FLOAT,
           CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR_RANGE(hud_heldflag, "1", "Show the held flag border", CVARTYPE_INT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE,
           0.0f, 2.0f)

CVAR(hud_heldflag_flash, "1", "Enables the flashes around the flag border.", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(hud_show_scoreboard_ondeath, "1", "Show the scoreboard on death.", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(hud_demobar, "1", "Shows the netdemo bar and timer on the HUD.", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)
CVAR(hud_demoprotos, "0", "Debug protocol messages while demo is paused.", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)
CVAR_RANGE(hud_feedtime, "3.0", "How long entries show in the event feed, in seconds.", CVARTYPE_FLOAT,
           CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 1.0, 10.0)
CVAR(hud_feedobits, "1", "Show obituaries in the event feed.", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

// GhostlyDeath <November 2, 2008> -- someone had the order wrong (0-9!)
CVAR(chatmacro1, "I'm ready to kick butt!", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(chatmacro2, "I'm OK.", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(chatmacro3, "I'm not looking too good!", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(chatmacro4, "Help!", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(chatmacro5, "You suck!", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(chatmacro6, "Next time, scumbag...", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(chatmacro7, "Come here!", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(chatmacro8, "I'll take care of it.", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(chatmacro9, "Yes", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR(chatmacro0, "No", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

// Sound and music
// ---------------

CVAR_RANGE_FUNC_DECL(snd_sfxvolume, "0.5", "Sound effect volume", CVARTYPE_FLOAT,
                     CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR_RANGE_FUNC_DECL(snd_musicvolume, "0.5", "Music volume", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE,
                     0.0f, 1.0f)

CVAR_RANGE(snd_announcervolume, "1.0", "Announcer volume", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE,
           0.0f, 1.0f)

CVAR_RANGE(snd_voxtype, "2", "Voice announcer type", CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 1.0f,
           2.0f)

CVAR(snd_gamesfx, "1", "Game SFX", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(snd_crossover, "0", "Stereo switch", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE_FUNC_DECL(snd_samplerate, "44100", "Audio samplerate", CVARTYPE_INT,
                     CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 22050.0f, 192000.0f)

CVAR(snd_soundfont, "default.sf2", "Fluidlite soundfont", CVARTYPE_STRING, CVAR_NULL)

// [AM] If you bump the maximum, change the NUM_CHANNELS define to match,
//      otherwise many things will break.
CVAR_RANGE_FUNC_DECL(snd_channels, "32", "Number of channels for sound effects", CVARTYPE_BYTE,
                     CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 4.0f, 32.0f)

//
// C_GetDefaultMuiscSystem()
//
// Allows the default value for snd_musicsystem to change depending on
// compile-time factors (eg, OS)
//
static char *C_GetDefaultMusicSystem()
{
    static char str[4];

    MusicSystemType defaultmusicsystem = MS_FLUIDLITE;

    // don't overflow str
    if (int32_t(defaultmusicsystem) > 999 || int32_t(defaultmusicsystem) < 0)
        defaultmusicsystem = MS_NONE;

    sprintf(str, "%i", defaultmusicsystem);
    return str;
}

CVAR_FUNC_DECL(snd_musicsystem, C_GetDefaultMusicSystem(), "Music subsystem preference", CVARTYPE_BYTE,
               CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

// Video and Renderer
// ------------------

CVAR_FUNC_DECL(gammalevel, "1", "Gamma correction level", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE_FUNC_DECL(vid_gammatype, "1", "Select between ZDoom and DOS Doom gamma correction", CVARTYPE_BYTE,
                     CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(r_flashhom, "0", "Draws flashing colors where there is HOM", CVARTYPE_BOOL, CVAR_NULL)

CVAR(r_drawflat, "0", "Disables all texturing of walls, floors and ceilings", CVARTYPE_BOOL, CVAR_NULL)

#if 0
CVAR(			r_drawhitboxes, "0", "Draws a box outlining every actor's hitboxes",
				CVARTYPE_BOOL, CVAR_NULL)
#endif

CVAR(r_particles, "1", "Draw particles", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE_FUNC_DECL(r_stretchsky, "2", "Stretch sky textures. (0 - always off, 1 - always on, 2 - auto)",
                     CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 2.0f)

CVAR(r_skypalette, "0", "Invulnerability sphere changes the palette of the sky", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(r_forceenemycolor, "0", "Changes the color of all enemies to the color specified by r_enemycolor",
               CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(r_enemycolor, "40 cf 00", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(r_forceteamcolor, "0", "Changes the color of all teammates to the color specified by r_teamcolor",
               CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(r_teamcolor, "40 cf 00", "", CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE(r_painintensity, "0.5", "Intensity of red pain effect", CVARTYPE_FLOAT,
           CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR_FUNC_DECL(r_imgui, "0", "Test for imgui", CVARTYPE_BOOL, 0);

CVAR(r_viewsize, "0", "Set to the current video resolution", CVARTYPE_STRING, CVAR_NOSET | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(vid_defwidth, "1280", "", CVARTYPE_WORD, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(vid_defheight, "720", "", CVARTYPE_WORD, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(vid_widescreen, "1", "Widescreen mode (0: Off, 1: Auto, 2: 16:10, 3: 16:9, 4: 21:9, 5: 32:9)",
               CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(vid_autoadjust, "1", "Force fullscreen resolution to the closest available video mode.", CVARTYPE_BOOL,
     CVAR_CLIENTARCHIVE)

CVAR_RANGE(vid_displayfps, "0", "Display frames per second.\n1: Full Graph.\n2: Just FPS Counter.", CVARTYPE_BYTE,
           CVAR_NOENABLEDISABLE, 0.0f, 2.0f)

CVAR(vid_ticker, "0", "Vanilla Doom frames per second indicator", CVARTYPE_BOOL, CVAR_NULL)

CVAR_FUNC_DECL(vid_maxfps, "60", "Maximum framerate (0 indicates unlimited framerate)", CVARTYPE_FLOAT,
               CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(vid_vsync, "0", "Enable/Disable vertical refresh sync (vsync)", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(vid_fullscreen, "0", "Full screen video mode", CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(vid_filter, "", "Set render scale quality setting for SDL 2.0, one of \"nearest\",\"linear\",\"best\"",
               CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

// Optimize rendering functions based on CPU vectorization support
// Can be of "detect" or "none" or "mmx","sse2","altivec" depending on availability; case-insensitive.
CVAR_FUNC_DECL(r_optimize, "detect", "Rendering optimizations", CVARTYPE_STRING,
               CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE_FUNC_DECL(screenblocks, "10", "Selects the size of the visible window", CVARTYPE_BYTE,
                     CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 3.0f, 12.0f)

CVAR_RANGE_FUNC_DECL(vid_overscan, "1.0", "Overscan matting (as a percentage of the screen area)", CVARTYPE_FLOAT,
                     CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.5f, 1.0f)

VERSION_CONTROL(cl_cvarlist_cpp, "$Id: d7013c30215a6872874d95987b634b912ae90e7b $")
