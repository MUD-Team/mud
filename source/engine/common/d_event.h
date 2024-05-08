// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 025bcc1ea3ff368ec655b68b790be1e333f82eb9 $
//
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
//	Input event handling (?)
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdint.h>

//
// Event handling.
//

// Input event types.
typedef enum
{
    ev_keydown,
    ev_keyup,
    ev_mouse,
    ev_joystick
} evtype_t;

// Event structure.
struct event_t
{
    event_t(evtype_t t = ev_keydown, int32_t d1 = 0, int32_t d2 = 0, int32_t d3 = 0, int32_t mod = 0)
        : type(t), data1(d1), data2(d2), data3(d3), mod(mod)
    {
    }

    event_t(const event_t &other)
    {
        this->operator=(other);
    }

    event_t &operator=(const event_t &other)
    {
        type  = other.type;
        data1 = other.data1;
        data2 = other.data2;
        data3 = other.data3;
        mod   = other.mod;
        return *this;
    }
    
    void clear()
    {
        type = ev_keydown;
        data1 = 0;
        data2 = 0;
        data3 = 0;
        mod = 0;
    }

    evtype_t type;
    int32_t      data1; // keys / mouse/joystick buttons
    int32_t      data2; // mouse/joystick x move
    int32_t      data3; // mouse/joystick y move
    int32_t      mod;   // input mods
};

typedef enum
{
    ga_nothing,
    ga_loadlevel,
    ga_newgame,
    ga_loadgame,
    ga_savegame,
    ga_completed,
    ga_worlddone,
    ga_screenshot,
    ga_fullresetlevel,
    ga_resetlevel
} gameaction_t;

//
// Button/action code definitions.
//
typedef enum
{
    // Press "Fire".
    BT_ATTACK = 1,
    // Use button, to open doors, activate switches.
    BT_USE = 2,

    // Flag: game events, not really buttons.
    BT_SPECIAL     = 128,
    BT_SPECIALMASK = 3,

    // Flag, weapon change pending.
    // If true, the next 3 bits hold weapon num.
    BT_CHANGE = 4,
    // The 3bit weapon mask and shift, convenience.
    BT_WEAPONMASK  = (8 + 16 + 32),
    BT_WEAPONSHIFT = 3,

    BT_JUMP = 64,

    // Pause the game.
    BTS_PAUSE = 1,
    // Save the game at each console.
    BTS_SAVEGAME = 2,

    // Savegame slot numbers
    //	occupy the second byte of buttons.
    BTS_SAVEMASK  = (4 + 8 + 16),
    BTS_SAVESHIFT = 2

} buttoncode_t;

//
// GLOBAL VARIABLES
//
#define MAXEVENTS 128

extern event_t events[MAXEVENTS];
extern int32_t     eventhead;
extern int32_t     eventtail;

extern gameaction_t gameaction;
