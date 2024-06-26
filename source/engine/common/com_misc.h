// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 55e6255de3b40cd38d21ab8543443f3f79fb352a $
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

#pragma once

#include <stdint.h>

#include <string>

#include "doomtype.h"

/**
 * @brief Toast for infofeed - these can be constructed from anywhere.
 */
struct toast_t
{
    enum flags
    {
        LEFT      = BIT(0),
        LEFT_PID  = BIT(1),
        RIGHT     = BIT(2),
        RIGHT_PID = BIT(3),
        ICON      = BIT(4),
    };

    uint32_t    flags;
    std::string left;
    int32_t     left_pid;
    std::string right;
    int32_t     right_pid;
    int32_t     icon;

    toast_t() : flags(0), left(""), left_pid(-1), right(""), right_pid(-1), icon(-1)
    {
    }

    toast_t(const toast_t &other)
    {
        flags     = other.flags;
        left      = other.left;
        left_pid  = other.left_pid;
        right     = other.right;
        right_pid = other.right_pid;
        icon      = other.icon;
    }
};

void COM_PushToast(const toast_t &toast);
