// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 327d88d404773e453fa0498df8318c01d7a0717f $
//
// Copyright (C) 2006-2022 by The Odamex Team.
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
//   Common HUD functionality that can be called by the server as well.
//
//-----------------------------------------------------------------------------

#include "com_misc.h"

#include "c_dispatch.h"
#include "m_random.h"
#include "mud_includes.h"
#include "p_local.h"
#if defined(SERVER_APP)
#include "svc_message.h"
#else
#endif
#include "v_textcolors.h"

void COM_PushToast(const toast_t &toast)
{
#if defined(SERVER_APP)
    for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
    {
        MSG_WriteSVC(&it->client.reliablebuf, SVC_Toast(toast));
    }
#else
    //hud::PushToast(toast);
#endif
}

BEGIN_COMMAND(toast)
{
    toast_t toast;

    toast.flags = toast_t::LEFT | toast_t::ICON | toast_t::RIGHT;

    if (M_Random() % 2)
    {
        toast.left  = std::string(TEXTCOLOR_LIGHTBLUE) + "[BLU]Ralphis";
        toast.right = std::string(TEXTCOLOR_BRICK) + "[RED]KBlair";
    }
    else
    {
        toast.left  = std::string(TEXTCOLOR_BRICK) + "[RED]KBlair";
        toast.right = std::string(TEXTCOLOR_LIGHTBLUE) + "[BLU]Ralphis";
    }

    toast.icon = M_Random() % NUMMODS;

    COM_PushToast(toast);
}
END_COMMAND(toast)
