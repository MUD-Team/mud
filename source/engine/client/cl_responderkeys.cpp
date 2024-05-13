// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 424f82d8f9b41912a718538027bc60e7b644ead5 $
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
//	Common descriptors for commands on the menu.
//  This was done to make console ports key handlers easier on the menu/console.
//
//-----------------------------------------------------------------------------

#include "cl_responderkeys.h"

#include "doomkeys.h"
#include "i_input.h"
#include "mud_includes.h"

//
// Key_IsUpKey
//
bool Key_IsUpKey(int32_t key, bool numlock)
{
    return (key == OKEY_HAT1 || key == OKEY_UPARROW || (key == OKEYP_8 && !numlock) || key == OKEY_JOY12);
}

//
// Key_IsDownKey
//
bool Key_IsDownKey(int32_t key, bool numlock)
{
    return (key == OKEY_HAT3 || key == OKEY_DOWNARROW || (key == OKEYP_2 && !numlock) || key == OKEY_JOY13);
}

//
// Key_IsLeftKey
//
bool Key_IsLeftKey(int32_t key, bool numlock)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_LEFTARROW || (key == OKEYP_4 && !numlock));

    return (key == OKEY_HAT4 || keyboard || key == OKEY_JOY14);
}

//
// Key_IsRightKey
//
bool Key_IsRightKey(int32_t key, bool numlock)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_RIGHTARROW || (key == OKEYP_6 && !numlock));

    return (key == OKEY_HAT2 || keyboard || key == OKEY_JOY15);
}

bool Key_IsPageUpKey(int32_t key, bool numlock)
{
    bool keyboard = (key == OKEY_PGUP || (key == OKEYP_9 && !numlock));

    return (keyboard || key == OKEY_JOY10);
}

bool Key_IsPageDownKey(int32_t key, bool numlock)
{
    bool keyboard = (key == OKEY_PGDN || (key == OKEYP_3 && !numlock));

    return (keyboard || key == OKEY_JOY11);
}

bool Key_IsHomeKey(int32_t key, bool numlock)
{
    return (key == OKEY_HOME || (key == OKEYP_7 && !numlock));
}

bool Key_IsEndKey(int32_t key, bool numlock)
{
    return (key == OKEY_END || (key == OKEYP_1 && !numlock));
}

bool Key_IsInsKey(int32_t key, bool numlock)
{
    return (key == OKEY_INS || (key == OKEYP_0 && !numlock));
}

bool Key_IsDelKey(int32_t key, bool numlock)
{
    return (key == OKEY_DEL || (key == OKEYP_PERIOD && !numlock));
}

//
// Key_IsAcceptKey
//
bool Key_IsAcceptKey(int32_t key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_ENTER || key == OKEYP_ENTER);

    return key == OKEY_JOY1 || keyboard;
}

//
// Key_IsCancelKey
//
bool Key_IsCancelKey(int32_t key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_ESCAPE);

    return (key == OKEY_JOY2 || keyboard);
}

//
// Key_IsYesKey
//
bool Key_IsYesKey(int32_t key)
{
    return (key == OKEY_JOY1);
}

//
// Key_IsNoKey
//
bool Key_IsNoKey(int32_t key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_ESCAPE);

    return (key == OKEY_JOY2 || keyboard);
}

//
// Key_IsMenuKey
//
bool Key_IsMenuKey(int32_t key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_ESCAPE);

    return (keyboard || key == OKEY_JOY7);
}

bool Key_IsUnbindKey(int32_t key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_BACKSPACE);

    return (keyboard || key == OKEY_JOY4);
}

bool Key_IsSpyNextKey(int32_t key)
{
    // Default Keyboard press
    bool mouse = (key == OKEY_MWHEELDOWN);

    return (mouse || key == OKEY_JOY15);
}

bool Key_IsSpyPrevKey(int32_t key)
{
    // Default Keyboard press
    bool mouse = (key == OKEY_MWHEELUP);

    return (mouse || key == OKEY_JOY14);
}

bool Key_IsTabulationKey(int32_t key)
{
    bool keyboard = (key == OKEY_TAB);

    return (keyboard);
}