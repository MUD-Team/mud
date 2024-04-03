// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: aab1550810d4b5090d6c58828087eb2cff4b70b7 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	G_GAME
//
//-----------------------------------------------------------------------------

#include <math.h> // for pow()

#include "am_map.h"
#include "c_bind.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "cl_main.h"
#include "cl_replay.h"
#include "d_main.h"
#include "g_game.h"
#include "g_gametype.h"
#include "g_spawninv.h"
#include "gi.h"
#include "gstrings.h"
#include "i_input.h"
#include "i_system.h"
#include "i_video.h"
#include "m_alloc.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_random.h"
#include "minilzo.h"
#include "odamex.h"
#include "p_horde.h"
#include "p_local.h"
#include "p_saveg.h"
#include "p_tick.h"
#include "r_draw.h"
#include "r_sky.h"
#include "s_sndseq.h"
#include "s_sound.h"
#include "script/lua_client_public.h"
#include "v_video.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"


#define SAVESTRINGSIZE 24

#define TURN180_TICKS 9 // [RH] # of ticks to complete a turn180

void G_PlayerReborn(player_t &player);

void G_DoNewGame(void);
void G_DoLoadGame(void);
void G_DoCompleted(void);
void G_DoWorldDone(void);
void G_DoSaveGame();

bool C_DoSpectatorKey(event_t *ev);

void CL_QuitCommand();

EXTERN_CVAR(sv_skill)
EXTERN_CVAR(novert)
EXTERN_CVAR(sv_monstersrespawn)
EXTERN_CVAR(sv_itemsrespawn)
EXTERN_CVAR(sv_respawnsuper)
EXTERN_CVAR(sv_weaponstay)
EXTERN_CVAR(sv_keepkeys)
EXTERN_CVAR(sv_sharekeys)
EXTERN_CVAR(in_autosr50)

gameaction_t gameaction;
gamestate_t  gamestate = GS_STARTUP;

BOOL paused;
BOOL sendpause;      // send a pause event next tic
BOOL sendsave;       // send a save event next tic
BOOL usergame;       // ok to save / end game
BOOL sendcenterview; // send a center view event next tic

bool nodrawers;      // for comparative timing purposes
bool noblit;         // for comparative timing purposes

BOOL viewactive;

// Describes if a network game is being played
BOOL network_game;
// Describes if this is a multiplayer game or not
BOOL multiplayer;
// The player vector, contains all player information
Players players;

byte consoleplayer_id; // player taking events and displaying
byte displayplayer_id; // view being displayed
int  gametic;

EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_fastmonsters)
EXTERN_CVAR(cl_run)
EXTERN_CVAR(hud_mousegraph)
EXTERN_CVAR(cl_predictpickup)

EXTERN_CVAR(mouse_sensitivity)
EXTERN_CVAR(m_pitch)
EXTERN_CVAR(m_filter)
EXTERN_CVAR(invertmouse)
EXTERN_CVAR(lookstrafe)
EXTERN_CVAR(m_yaw)
EXTERN_CVAR(m_forward)
EXTERN_CVAR(m_side)

CVAR_FUNC_IMPL(mouse_type)
{
    // Convert vanilla Doom mouse sensitivity settings to ZDoom mouse sensitivity
    if (var.asInt() == MOUSE_DOOM)
    {
        mouse_sensitivity.Set((mouse_sensitivity + 5.0f) / 40.0f);
        m_pitch.Set(m_pitch * 4.0f);
    }
    if (var.asInt() != MOUSE_ZDOOM_DI)
        var.Set(MOUSE_ZDOOM_DI);
}

CVAR_FUNC_IMPL(cl_mouselook)
{
    // Nes - center the view
    AddCommandString("centerview");

    // Nes - update skies
    R_InitSkyMap();
}

extern bool simulated_connection;

int iffdemover;

BOOL precache = true;   // if true, load all graphics at start

wbstartstruct_t wminfo; // parms for world map / intermission

#define MAXPLMOVE (forwardmove[1])

#define TURBOTHRESHOLD 12800

fixed_t forwardmove[2] = {0x19, 0x32};
fixed_t sidemove[2]    = {0x18, 0x28};

fixed_t angleturn[3] = {640, 1280, 320}; // + slow turn
fixed_t flyspeed[2]  = {1 * 256, 3 * 256};
int     lookspeed[2] = {450, 512};

#define SLOWTURNTICS 6

int turnheld; // for accelerative turning

// mouse values are used once
int mousex;
int mousey;

// [Toke - Mouse] new mouse stuff
int mousexleft;
int mouseydown;

// Joystick values are repeated
// Store a value for each of the analog axis controls -- Hyper_Eye
int joyforward;
int joystrafe;
int joyturn;
int joylook;

EXTERN_CVAR(joy_forwardaxis)
EXTERN_CVAR(joy_strafeaxis)
EXTERN_CVAR(joy_turnaxis)
EXTERN_CVAR(joy_lookaxis)
EXTERN_CVAR(joy_sensitivity)
EXTERN_CVAR(joy_fastsensitivity)
EXTERN_CVAR(joy_invert)
EXTERN_CVAR(joy_freelook)

int  savegameslot;
char savedescription[32];

player_t &consoleplayer()
{
    return idplayer(consoleplayer_id);
}

player_t &displayplayer()
{
    return idplayer(displayplayer_id);
}

player_t &listenplayer()
{
    return displayplayer();
}

// [RH] Name of screenshot file to generate (usually NULL)
std::string shotfile;

/* [RH] Impulses: Temporary hack to get weapon changing
 * working with keybindings until I can get the
 * inventory system working.
 *
 *	So this turned out to not be so temporary. It *will*
 * change, though.
 */
int Impulse;

BEGIN_COMMAND(impulse)
{
    if (argc > 1)
        Impulse = atoi(argv[1]);
}
END_COMMAND(impulse)

BEGIN_COMMAND(centerview)
{
    sendcenterview = true;
}
END_COMMAND(centerview)

BEGIN_COMMAND(pause)
{
    sendpause = true;
}
END_COMMAND(pause)

BEGIN_COMMAND(turnspeeds)
{
    if (argc == 1)
    {
        Printf(PRINT_HIGH, "Current turn speeds: %ld %ld %ld\n", angleturn[0], angleturn[1], angleturn[2]);
    }
    else
    {
        size_t i;
        for (i = 1; i <= 3 && i < argc; i++)
            angleturn[i - 1] = atoi(argv[i]);

        if (i <= 2)
            angleturn[1] = angleturn[0] * 2;
        if (i <= 3)
            angleturn[2] = angleturn[0] / 2;
    }
}
END_COMMAND(turnspeeds)

static int turntick;
BEGIN_COMMAND(turn180)
{
    turntick = TURN180_TICKS;
}
END_COMMAND(turn180)

weapontype_t P_GetNextWeapon(player_t *player, bool forward);
BEGIN_COMMAND(weapnext)
{
    weapontype_t newweapon = P_GetNextWeapon(&consoleplayer(), true);
    if (newweapon != wp_nochange)
        Impulse = int(newweapon) + 50;
}
END_COMMAND(weapnext)

BEGIN_COMMAND(weapprev)
{
    weapontype_t newweapon = P_GetNextWeapon(&consoleplayer(), false);
    if (newweapon != wp_nochange)
        Impulse = int(newweapon) + 50;
}
END_COMMAND(weapprev)

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
void G_BuildTiccmd(ticcmd_t *cmd)
{
    ::localview.skipangle = false;
    ::localview.skippitch = false;

    ticcmd_t *base = I_BaseTiccmd(); // empty or external driver
    memcpy(cmd, base, sizeof(*cmd));

    int strafe = Actions[ACTION_STRAFE];
    int speed  = Actions[ACTION_SPEED];
    if (cl_run)
        speed ^= 1;

    int forward = 0, side = 0, look = 0, fly = 0;

    if ((&consoleplayer())->spectator && Actions[ACTION_USE] && connected)
        AddCommandString("join");

    // [RH] only use two stage accelerative turning on the keyboard
    //		and not the joystick, since we treat the joystick as
    //		the analog device it is.
    if ((Actions[ACTION_LEFT]) || (Actions[ACTION_RIGHT]))
        turnheld += 1;
    else
        turnheld = 0;

    int tspeed = speed;
    if (turnheld < SLOWTURNTICS)
        tspeed = 2; // slow turn

    // let movement keys cancel each other out
    if (strafe)
    {
        if (in_autosr50)
        {
            if (Actions[ACTION_MOVERIGHT])
                side += sidemove[speed];
            if (Actions[ACTION_MOVELEFT])
                side -= sidemove[speed];
        }
        else
        {
            if (Actions[ACTION_RIGHT])
                side += sidemove[speed];
            if (Actions[ACTION_LEFT])
                side -= sidemove[speed];
        }
    }
    else
    {
        if (Actions[ACTION_RIGHT])
        {
            if (::angleturn[tspeed] != 0)
            {
                cmd->yaw -= ::angleturn[tspeed];
                ::localview.skipangle = true;
            }
        }
        if (Actions[ACTION_LEFT])
        {
            if (::angleturn[tspeed] != 0)
            {
                cmd->yaw += ::angleturn[tspeed];
                ::localview.skipangle = true;
            }
        }
    }

    // Joystick analog strafing -- Hyper_Eye
    side += (int)(((float)joystrafe / (float)SHRT_MAX) * sidemove[speed]);

    if (Actions[ACTION_LOOKUP])
    {
        look += lookspeed[speed];
        ::localview.skippitch = true;
    }
    if (Actions[ACTION_LOOKDOWN])
    {
        look -= lookspeed[speed];
        ::localview.skippitch = true;
    }

    if (Actions[ACTION_MOVEUP])
        fly += flyspeed[speed];
    if (Actions[ACTION_MOVEDOWN])
        fly -= flyspeed[speed];

    if (Actions[ACTION_KLOOK])
    {
        if (Actions[ACTION_FORWARD])
        {
            look += lookspeed[speed];
            ::localview.skippitch = true;
        }
        if (Actions[ACTION_BACK])
        {
            look -= lookspeed[speed];
            ::localview.skippitch = true;
        }
    }
    else
    {
        if (Actions[ACTION_FORWARD])
            forward += forwardmove[speed];
        if (Actions[ACTION_BACK])
            forward -= forwardmove[speed];
    }

    // Joystick analog look -- Hyper_Eye
    if (joy_freelook || consoleplayer().spectator)
    {
        if (joy_invert)
            look += (int)(((float)joylook / (float)SHRT_MAX) * lookspeed[speed]);
        else
            look -= (int)(((float)joylook / (float)SHRT_MAX) * lookspeed[speed]);

        ::localview.skippitch = true;
    }

    if (Actions[ACTION_MOVERIGHT])
    {
        side += sidemove[speed];
    }
    if (Actions[ACTION_MOVELEFT])
    {
        side -= sidemove[speed];
    }

    // buttons
    if (Actions[ACTION_ATTACK])
        cmd->buttons |= BT_ATTACK;

    if (Actions[ACTION_USE])
        cmd->buttons |= BT_USE;

    // Ch0wW : Forbid writing ACTION_JUMP to the demofile if recording a vanilla-compatible demo.
    if (Actions[ACTION_JUMP])
        cmd->buttons |= BT_JUMP;

    // [RH] Handle impulses. If they are between 1 and 7,
    //		they get sent as weapon change events.
    // FIXME : "weapnext/weapprev" doesn't handle this properly, desyncing the demos.
    if (Impulse >= 1 && Impulse <= 8)
    {
        cmd->buttons |= BT_CHANGE;
        cmd->buttons |= (Impulse - 1) << BT_WEAPONSHIFT;
    }
    else
    {
        cmd->impulse = Impulse;
    }
    Impulse = 0;

    // [SL] 2012-03-31 - Let the server know when the client is predicting a
    // weapon change due to a weapon pickup
    if (!serverside && cl_predictpickup)
    {
        if (!cmd->impulse && !(cmd->buttons & BT_CHANGE) && consoleplayer().pendingweapon != wp_nochange)
            cmd->impulse = 50 + static_cast<int>(consoleplayer().pendingweapon);
    }

    if (::joyturn)
    {
        if (strafe || lookstrafe)
        {
            side += (int)(((float)::joyturn / (float)SHRT_MAX) * ::sidemove[speed]);
        }
        else
        {
            if (Actions[ACTION_FASTTURN])
                cmd->yaw -= (short)((((float)joyturn / (float)SHRT_MAX) * angleturn[1]) * (joy_fastsensitivity / 10));
            else
                cmd->yaw -= (short)((((float)joyturn / (float)SHRT_MAX) * angleturn[1]) * (joy_sensitivity / 10));
        }
        ::localview.skipangle = true;
    }

    if (Actions[ACTION_MLOOK])
    {
        if (joy_invert)
            look += (int)(((float)joyforward / (float)SHRT_MAX) * lookspeed[speed]);
        else
            look -= (int)(((float)joyforward / (float)SHRT_MAX) * lookspeed[speed]);
        ::localview.skippitch = true;
    }
    else
    {
        forward -= (int)(((float)joyforward / (float)SHRT_MAX) * forwardmove[speed]);
    }

    if (!consoleplayer().spectator && !Actions[ACTION_MLOOK] && !cl_mouselook &&
        novert == 0) // [Toke - Mouse] acts like novert.exe
    {
        forward += (int)(float(mousey) * m_forward);
    }

    if (strafe || lookstrafe)
        side += (int)(float(mousex) * m_side);

    mousex = mousey = 0;

    if (forward > MAXPLMOVE)
        forward = MAXPLMOVE;
    else if (forward < -MAXPLMOVE)
        forward = -MAXPLMOVE;
    if (side > MAXPLMOVE)
        side = MAXPLMOVE;
    else if (side < -MAXPLMOVE)
        side = -MAXPLMOVE;

    cmd->forwardmove += forward;
    cmd->sidemove += side;
    cmd->upmove = fly;

    // special buttons
    if (sendpause)
    {
        sendpause    = false;
        cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

    if (sendsave)
    {
        sendsave     = false;
        cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot << BTS_SAVESHIFT);
    }

    cmd->forwardmove <<= 8;
    cmd->sidemove <<= 8;

    //// [RH] 180-degree turn overrides all other yaws
    if (turntick)
    {
        turntick--;
        cmd->yaw              = (ANG180 / TURN180_TICKS) >> 16;
        ::localview.skipangle = true;
    }

    if (sendcenterview)
    {
        sendcenterview = false;
        cmd->pitch     = CENTERVIEW;
    }
    else
    {
        // [AM] LocalViewPitch is an offset on look.
        cmd->pitch = look + (::localview.pitch >> 16);
    }

    if (::localview.setangle)
    {
        // [AM] LocalViewAngle is a global angle, only pave over the existing
        //      yaw if we have local yaw.
        cmd->yaw = ::localview.angle >> 16;
    }

    ::localview.angle    = 0;
    ::localview.setangle = false;
    ::localview.pitch    = 0;
    ::localview.setpitch = false;
}

// [RH] Spy mode has been separated into two console commands.
//		One goes forward; the other goes backward.
// denis - todo - should this be used somewhere?
/*static void ChangeSpy (void)
{
    consoleplayer().camera = players[displayplayer].mo;
    S_UpdateSounds(consoleplayer().camera);
    ST_Start();		// killough 3/7/98: switch status bar views too
}*/

float G_ZDoomDIMouseScaleX(float x)
{
    return (x * 4.0 * mouse_sensitivity);
}

float G_ZDoomDIMouseScaleY(float y)
{
    return (y * mouse_sensitivity);
}

void G_ProcessMouseMovementEvent(const event_t *ev)
{
    static float fprevx = 0.0f, fprevy = 0.0f;
    float        fmousex = (float)ev->data2;
    float        fmousey = (float)ev->data3;

    if (m_filter)
    {
        // smooth out the mouse input
        fmousex = (fmousex + fprevx) / 2.0f;
        fmousey = (fmousey + fprevy) / 2.0f;
    }

    fprevx = fmousex;
    fprevy = fmousey;

    fmousex = G_ZDoomDIMouseScaleX(fmousex);
    fmousey = G_ZDoomDIMouseScaleY(fmousey);

    mousex = (int)fmousex;
    mousey = (int)fmousey;

    G_AddViewAngle(fmousex * 8.0f * m_yaw);
    G_AddViewPitch(fmousey * 16.0f * m_pitch);
}

void G_AddViewAngle(int yaw)
{
    if (G_ShouldIgnoreMouseInput())
        return;

    if (!Actions[ACTION_STRAFE] && !lookstrafe)
    {
        localview.angle -= yaw << 16;
        localview.setangle = true;
    }
}

void G_AddViewPitch(int pitch)
{
    if (G_ShouldIgnoreMouseInput())
        return;

    if (invertmouse)
        pitch = -pitch;

    if ((Actions[ACTION_MLOOK]) || (cl_mouselook) || consoleplayer().spectator)
    {
        localview.pitch += pitch << 16;
        localview.setpitch = true;
    }
}

bool G_ShouldIgnoreMouseInput()
{
    if (consoleplayer().id != displayplayer().id || consoleplayer().playerstate == PST_DEAD)
        return true;

    return false;
}

//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
BOOL G_Responder(event_t *ev)
{
    // any other key pops up menu if in demos
    // [RH] But only if the key isn't bound to a "special" command
    if (gameaction == ga_nothing && (gamestate == GS_NONE))
    {
        const char *cmd = Bindings.GetBind(ev->data1).c_str();

        if (ev->type == ev_keydown)
        {

            if (!cmd || (strnicmp(cmd, "menu_", 5) && stricmp(cmd, "toggleconsole") && stricmp(cmd, "sizeup") &&
                         stricmp(cmd, "sizedown") && stricmp(cmd, "togglemap") && stricmp(cmd, "spynext") &&
                         stricmp(cmd, "chase") && stricmp(cmd, "+showscores") && stricmp(cmd, "bumpgamma") &&
                         stricmp(cmd, "screenshot") && stricmp(cmd, "stepmode") && stricmp(cmd, "step")))
            {
                S_Sound(CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
                return true;
            }
            else
            {
                return C_DoKey(ev, &Bindings, &DoubleBindings);
            }
        }
        if (cmd && cmd[0] == '+')
            return C_DoKey(ev, &Bindings, &DoubleBindings);

        return false;
    }

    if (gamestate == GS_LEVEL)
    {
        if (C_DoSpectatorKey(ev))
            return true;

        if (!viewactive)
            if (AM_Responder(ev))
                return true; // automap ate it
    }

    switch (ev->type)
    {
    case ev_keydown:
        if (C_DoKey(ev, &Bindings, &DoubleBindings))
            return true;
        break;

    case ev_keyup:
        C_DoKey(ev, &Bindings, &DoubleBindings);
        break;

    // [Toke - Mouse] New mouse code
    case ev_mouse:
        G_ProcessMouseMovementEvent(ev);

        // if (hud_mousegraph)
        //     mousegraph.append(mousex, mousey);

        break;

    case ev_joystick:
        if (ev->data1 == 0)                        // Axis Movement
        {
            if (ev->data2 == joy_strafeaxis)       // Strafe
                joystrafe = ev->data3;
            else if (ev->data2 == joy_forwardaxis) // Move
                joyforward = ev->data3;
            else if (ev->data2 == joy_turnaxis)    // Turn
                joyturn = ev->data3;
            else if (ev->data2 == joy_lookaxis)    // Look
                joylook = ev->data3;
            else
                break; // The default case will be to treat the analog control as a button -- Hyper_Eye
        }

        break;
    }

    // [RH] If the view is active, give the automap a chance at
    // the events *last* so that any bound keys get precedence.

    if (gamestate == GS_LEVEL && viewactive)
        return AM_Responder(ev);

    if (ev->type == ev_keydown || ev->type == ev_mouse || ev->type == ev_joystick)
        return true;
    else
        return false;
}

int netin;
int netout;
int outrate;

BEGIN_COMMAND(netstat)
{
    Printf(PRINT_HIGH, "in = %d  out = %d \n", netin, netout);
}
END_COMMAND(netstat)

void P_MovePlayer(player_t *player);
void P_CalcHeight(player_t *player);
void P_DeathThink(player_t *player);
void CL_SimulateWorld();
//
// G_Ticker
// Make ticcmd_ts for the players.
//
extern DCanvas *page;
extern int      connecttimeout;

void G_Ticker(void)
{
    int buf;

    // Turn off no-z-snapping for all players.
    // [AM] Eventually, it would be nice to do this for all mobjs, but iterating
    //      through every mobj every tic would be incredibly time-consuming.
    if (!serverside)
    {
        for (Players::iterator it = players.begin(); it != players.end(); ++it)
        {
            if (it->mo)
                it->mo->oflags &= ~MFO_NOSNAPZ;
        }
    }

    // do player reborns if needed
    if (serverside)
        for (Players::iterator it = players.begin(); it != players.end(); ++it)
        {
            if (it->ingame() && (it->playerstate == PST_REBORN || it->playerstate == PST_ENTER))
            {
                if (it->playerstate == PST_REBORN)
                    it->doreborn = true; // State only our will to lose the whole inventory in case of a reborn.
                G_DoReborn(*it);
            }
        }

    // do things to change the game state
    while (gameaction != ga_nothing)
    {
        switch (gameaction)
        {
        case ga_loadlevel:
            G_DoLoadLevel(-1);
            break;
        case ga_fullresetlevel:
            gameaction = ga_nothing;
            break;
        case ga_resetlevel:
            gameaction = ga_nothing;
            break;
        case ga_newgame:
            G_DoNewGame();
            break;
        case ga_loadgame:
            G_DoLoadGame();
            break;
        case ga_savegame:
            G_DoSaveGame();
            break;
        case ga_completed:
            G_DoCompleted();
            break;
        case ga_worlddone:
            G_DoWorldDone();
            break;
        case ga_screenshot:
            // V_ScreenShot(shotfile);
            gameaction = ga_nothing;
            break;
        case ga_nothing:
            break;
        }        
    }

    buf = gametic % BACKUPTICS;

    // get commands
    memcpy(&consoleplayer().cmd, &consoleplayer().netcmds[buf], sizeof(ticcmd_t));

    static int realrate = 0;
    int        packet_size;

    if (connected && !simulated_connection)
    {
        while ((packet_size = NET_GetPacket()))
        {
            // denis - don't accept candy from strangers
            if (!NET_CompareAdr(serveraddr, net_from))
                break;

            realrate += packet_size;
            last_received = gametic;
            noservermsgs  = false;

            if (!CL_ReadPacketHeader())
                continue;

            CL_ParseCommands();

            if (gameaction == ga_nothing) 
                return;
        }

        if (!(gametic % TICRATE))
        {
            netin    = realrate;
            realrate = 0;
        }

        CL_SaveCmd();     // save console commands
        if (!noservermsgs)
            CL_SendCmd(); // send console commands to the server

        if (!(gametic % TICRATE))
        {
            netout  = outrate;
            outrate = 0;
        }

        if (gametic - last_received > 65)
            noservermsgs = true;
    }
    else if (NET_GetPacket() && !simulated_connection)
    {
        // denis - don't accept candy from strangers
        if (gamestate == GS_CONNECTING && NET_CompareAdr(serveraddr, net_from))
        {
            int type = MSG_ReadLong();

            if (type == MSG_CHALLENGE)
            {
                CL_PrepareConnect();
            }
            else if (type == 0)
            {
                if (!CL_Connect())
                    memset(&serveraddr, 0, sizeof(serveraddr));

                connecttimeout = 0;
            }
            else
            {
                // we are already connected to this server, quit first
                MSG_WriteMarker(&net_buffer, clc_disconnect);
                NET_SendPacket(net_buffer, serveraddr);

                Printf(PRINT_WARNING, "Got unknown challenge %d while connecting, disconnecting.\n", type);
            }
        }
    }

    // check for special buttons
    if (serverside && consoleplayer().ingame())
    {
        player_t &player = consoleplayer();

        if (player.cmd.buttons & BT_SPECIAL)
        {
            switch (player.cmd.buttons & BT_SPECIALMASK)
            {
            case BTS_PAUSE:
                paused ^= 1;
                if (paused)
                    S_PauseSound();
                else
                    S_ResumeSound();
                break;

            case BTS_SAVEGAME:
                if (!savedescription[0])
                    strcpy(savedescription, "NET GAME");
                savegameslot = (player.cmd.buttons & BTS_SAVEMASK) >> BTS_SAVESHIFT;
                gameaction   = ga_savegame;
                break;
            }
        }
    }

    // do main actions
    switch (gamestate)
    {
    case GS_LEVEL:
        if (clientside && !serverside)
        {
            if (!consoleplayer().mo)
            {
                // [SL] 2011-12-14 - Spawn message from server has not arrived
                // yet.  Fake it and hope it arrives soon.
                AActor *mobj = new AActor(0, 0, 0, MT_PLAYER);
                mobj->flags &= ~MF_SOLID;
                mobj->flags2 |= MF2_DONTDRAW;
                consoleplayer().mo = consoleplayer().camera = mobj->ptr();
                consoleplayer().mo->player                  = &consoleplayer();
                G_PlayerReborn(consoleplayer());
                DPrintf("Did not receive spawn for consoleplayer.\n");
            }

            CL_SimulateWorld();
            CL_PredictWorld();

            // Replay item pickups if the items arrived now.
            ClientReplay::getInstance().itemReplay();
        }
        P_Ticker();
        AM_Ticker();
        break;

    default:
        break;
    }

    LUA_ClientGameTicker();
}

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Mobj
//

//
// G_PlayerFinishLevel
// Call when a player completes a level.
//
void G_PlayerFinishLevel(player_t &player)
{
    player_t *p;

    p = &player;

    memset(p->powers, 0, sizeof(p->powers));
    memset(p->cards, 0, sizeof(p->cards));

    if (p->mo)
        p->mo->flags &= ~MF_SHADOW; // cancel invisibility

    p->extralight    = 0;           // cancel gun flashes
    p->fixedcolormap = 0;           // cancel ir goggles
    p->damagecount   = 0;           // no palette changes
    p->bonuscount    = 0;
}

//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
void G_PlayerReborn(player_t &p) // [Toke - todo] clean this function
{
    size_t i;
    for (i = 0; i < NUMAMMO; i++)
    {
        p.maxammo[i] = maxammo[i];
        p.ammo[i]    = 0;
    }
    for (i = 0; i < NUMWEAPONS; i++)
        p.weaponowned[i] = false;

    if (!sv_keepkeys && !sv_sharekeys)
        P_ClearPlayerCards(p);

    P_ClearPlayerPowerups(p);

    for (i = 0; i < NUMTEAMS; i++)
        p.flags[i] = false;
    p.backpack = false;

    G_GiveSpawnInventory(p);

    p.usedown = p.attackdown  = true; // don't do anything immediately
    p.playerstate             = PST_LIVE;
    p.weaponowned[NUMWEAPONS] = true;

    if (!p.spectator)
        p.cheats = 0; // Reset cheat flags

    p.death_time = 0;
    p.tic        = 0;
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing2_t spot
// because something is occupying it
//
void P_SpawnPlayer(player_t &player, mapthing2_t *mthing);

bool G_CheckSpot(player_t &player, mapthing2_t *mthing)
{
    unsigned an;
    AActor  *mo;
    fixed_t  xa, ya;

    fixed_t x = mthing->x << FRACBITS;
    fixed_t y = mthing->y << FRACBITS;
    fixed_t z = P_FloorHeight(x, y);

    if (level.flags & LEVEL_USEPLAYERSTARTZ)
        z = mthing->z << FRACBITS;

    if (!player.mo)
    {
        // first spawn of level, before corpses
        for (Players::iterator it = players.begin(); it != players.end(); ++it)
        {
            if (&player == &*it)
                continue;

            if (it->mo && it->mo->x == x && it->mo->y == y)
                return false;
        }
        return true;
    }

    fixed_t oldz = player.mo->z; // [RH] Need to save corpse's z-height
    player.mo->z = z;            // [RH] Checks are now full 3-D

    // killough 4/2/98: fix bug where P_CheckPosition() uses a non-solid
    // corpse to detect collisions with other players in DM starts
    //
    // Old code:
    // if (!P_CheckPosition (players[playernum].mo, x, y))
    //    return false;

    player.mo->flags |= MF_SOLID;
    bool valid_position = P_CheckPosition(player.mo, x, y);
    player.mo->flags &= ~MF_SOLID;
    player.mo->z = oldz; // [RH] Restore corpse's height
    if (!valid_position)
        return false;

    // spawn a teleport fog
    //	if (!player.spectator && !player.deadspectator)	// ONLY IF THEY ARE NOT A SPECTATOR
    if (!player.spectator) // ONLY IF THEY ARE NOT A SPECTATOR
    {
        an = (ANG45 * ((unsigned int)mthing->angle / 45)) >> ANGLETOFINESHIFT;
        xa = finecosine[an];
        ya = finesine[an];

        mo = new AActor(x + 20 * xa, y + 20 * ya, z, MT_TFOG);

        if (level.time)
            S_Sound(mo, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM); // don't start sound on first frame
    }

    return true;
}

//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//

// [RH] Returns the distance of the closest player to the given mapthing2_t.
// denis - todo - should this be used somewhere?
/*static fixed_t PlayersRangeFromSpot (mapthing2_t *spot)
{
    fixed_t closest = MAX_INT;
    fixed_t distance;

    for (size_t i = 0; i < players.size(); i++)
    {
        if (!players[i].ingame() || !players[i].mo || players[i].health <= 0)
            continue;

        distance = P_AproxDistance (players[i].mo->x - spot->x * FRACUNIT,
                                    players[i].mo->y - spot->y * FRACUNIT);

        if (distance < closest)
            closest = distance;
    }

    return closest;
}

// [RH] Select the deathmatch spawn spot farthest from everyone.
static mapthing2_t *SelectFarthestDeathmatchSpot (int selections)
{
    fixed_t bestdistance = 0;
    mapthing2_t *bestspot = NULL;
    int i;

    for (i = 0; i < selections; i++)
    {
        fixed_t distance = PlayersRangeFromSpot (&deathmatchstarts[i]);

        if (distance > bestdistance)
        {
            bestdistance = distance;
            bestspot = &deathmatchstarts[i];
        }
    }

    return bestspot;
}

*/

// [RH] Select a deathmatch spawn spot at random (original mechanism)
static mapthing2_t *SelectRandomDeathmatchSpot(player_t &player, int selections)
{
    int i = 0, j;

    for (j = 0; j < 20; j++)
    {
        i = P_Random() % selections;
        if (G_CheckSpot(player, &DeathMatchStarts[i]))
            return &DeathMatchStarts[i];
    }

    // [RH] return a spot anyway, since we allow telefragging when a player spawns
    return &DeathMatchStarts[i];
}

void G_DeathMatchSpawnPlayer(player_t &player)
{
    int          selections;
    mapthing2_t *spot;

    if (!serverside || G_UsesCoopSpawns())
        return;

    selections = DeathMatchStarts.size();
    // [RH] We can get by with just 1 deathmatch start
    if (selections < 1)
        I_Error("No deathmatch starts");

    // [Toke - dmflags] Old location of DF_SPAWN_FARTHEST
    spot = SelectRandomDeathmatchSpot(player, selections);

    if (!spot && !playerstarts.empty())
    {
        // no good spot, so the player will probably get stuck
        spot = &playerstarts[player.id % playerstarts.size()];
    }
    else
    {
        if (player.id < 4)
            spot->type = player.id + 1;
        else
            spot->type = player.id + 4001 - 4; // [RH] > 4 players
    }

    P_SpawnPlayer(player, spot);
}

//
// G_DoReborn
//
void G_DoReborn(player_t &player)
{
    if (!serverside)
        return;

    if (!multiplayer)
    {
        // reload the level from scratch
        gameaction = ga_loadlevel;
        return;
    }

    // respawn at the start
    // first disassociate the corpse
    if (player.mo)
        player.mo->player = NULL;

    // spawn at random spot if in death match
    if (!G_UsesCoopSpawns())
    {
        G_DeathMatchSpawnPlayer(player);
        return;
    }

    if (playerstarts.empty())
        I_Error("No player starts");

    unsigned int playernum = player.id - 1;

    if (G_CheckSpot(player, &playerstarts[playernum % playerstarts.size()]))
    {
        P_SpawnPlayer(player, &playerstarts[playernum % playerstarts.size()]);
        return;
    }

    // try to spawn at one of the other players' spots
    for (size_t i = 0; i < playerstarts.size(); i++)
    {
        if (G_CheckSpot(player, &playerstarts[i]))
        {
            P_SpawnPlayer(player, &playerstarts[i]);
            return;
        }
    }

    // he's going to be inside something.  Too bad.
    P_SpawnPlayer(player, &playerstarts[playernum % playerstarts.size()]);
}

void G_ScreenShot(const char *filename)
{
    // SoM: THIS CRASHES A LOT
    if (filename && *filename)
        shotfile = filename;
    else
        shotfile = "";

    gameaction = ga_screenshot;
}

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
char savename[256];

void G_LoadGame(char *name)
{
    strcpy(savename, name);
    gameaction = ga_loadgame;
}

void G_DoLoadGame(void)
{
    unsigned int i;
    char         text[16];

    gameaction = ga_nothing;

    PHYSFS_File *stdfile = PHYSFS_openRead(savename);
    if (stdfile == NULL)
    {
        Printf(PRINT_HIGH, "Could not read savegame '%s'\n", savename);
        return;
    }

    PHYSFS_seek(stdfile, SAVESTRINGSIZE); // skip the description field
    size_t readlen = PHYSFS_readBytes(stdfile, text, 16);
    if (readlen < 16)
    {
        Printf(PRINT_HIGH, "Failed to read savegame '%s'\n", savename);
        PHYSFS_close(stdfile);
        return;
    }
    if (strncmp(text, SAVESIG, 16))
    {
        Printf(PRINT_HIGH, "Savegame '%s' is from a different version\n", savename);

        PHYSFS_close(stdfile);

        return;
    }
    readlen = PHYSFS_readBytes(stdfile, text, 8);
    if (readlen < 8)
    {
        Printf(PRINT_HIGH, "Failed to read savegame '%s'\n", savename);
        PHYSFS_close(stdfile);
        return;
    }
    text[8] = 0;

    /*bglobal.RemoveAllBots (true);*/

    FLZOFile savefile(stdfile, FFile::EReading);

    if (!savefile.IsOpen())
        I_Error("Savegame '%s' is corrupt\n", savename);

    Printf(PRINT_HIGH, "Loading savegame '%s'...\n", savename);

    CL_QuitNetGame(NQ_SILENT);

    FArchive arc(savefile);

    {
        byte         vars[4096], *vars_p;
        unsigned int len;
        vars_p = vars;
        len    = arc.ReadCount();
        arc.Read(vars, len);
        cvar_t::C_ReadCVars(&vars_p);
    }

    // dearchive all the modifications
    G_SerializeSnapshots(arc);
    P_SerializeRNGState(arc);
    P_SerializeACSDefereds(arc);
    P_SerializeHorde(arc);

    multiplayer = false;

    // load a base level
    savegamerestore = true; // Use the player actors in the savegame
    serverside      = true;
    G_InitNew(text);
    displayplayer_id = consoleplayer_id = 1;
    savegamerestore                     = false;

    arc >> level.time;

    for (i = 0; i < NUM_WORLDVARS; i++)
        arc >> ACS_WorldVars[i];

    for (i = 0; i < NUM_GLOBALVARS; i++)
        arc >> ACS_GlobalVars[i];

    arc >> text[9];

    arc.Close();

    if (text[9] != 0x1d)
        I_Error("Bad savegame");

    P_HordePostLoad();
}

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
void G_SaveGame(int slot, char *description)
{
    savegameslot = slot;
    strcpy(savedescription, description);
    sendsave = true;
}

/**
 * @brief Create a filename for a savegame.
 *
 * @param name Output string.
 * @param slot Slot number.
 */
void G_BuildSaveName(std::string &name, int slot)
{
    StrFormat(name, "saves/odasv%d.ods", slot);
}

void G_DoSaveGame()
{
    std::string name;
    char       *description;
    int         i;

    G_SnapshotLevel();

    G_BuildSaveName(name, savegameslot);
    description = savedescription;

    PHYSFS_File *stdfile = PHYSFS_openWrite(name.c_str());

    if (stdfile == NULL)
    {
        return;
    }

    Printf(PRINT_HIGH, "Saving game to '%s'...\n", name.c_str());

    PHYSFS_writeBytes(stdfile, description, SAVESTRINGSIZE);
    PHYSFS_writeBytes(stdfile, SAVESIG, 16);
    PHYSFS_writeBytes(stdfile, level.mapname.c_str(), 8);

    FLZOFile savefile(stdfile, FFile::EWriting, true);
    FArchive arc(savefile);

    {
        byte vars[4096], *vars_p;
        vars_p = vars;

        cvar_t::C_WriteCVars(&vars_p, CVAR_SERVERINFO);
        arc.WriteCount(vars_p - vars);
        arc.Write(vars, vars_p - vars);
    }

    G_SerializeSnapshots(arc);
    P_SerializeRNGState(arc);
    P_SerializeACSDefereds(arc);
    P_SerializeHorde(arc);

    arc << level.time;

    for (i = 0; i < NUM_WORLDVARS; i++)
        arc << ACS_WorldVars[i];

    for (i = 0; i < NUM_GLOBALVARS; i++)
        arc << ACS_GlobalVars[i];

    arc << (BYTE)0x1d; // consistancy marker

    gameaction         = ga_nothing;
    savedescription[0] = 0;

    Printf(PRINT_HIGH, "%s\n", GStrings(GGSAVED));
    arc.Close();

    if (level.info->snapshot != NULL)
    {
        delete level.info->snapshot;
        level.info->snapshot = NULL;
    }
}

VERSION_CONTROL(g_game_cpp, "$Id: aab1550810d4b5090d6c58828087eb2cff4b70b7 $")
