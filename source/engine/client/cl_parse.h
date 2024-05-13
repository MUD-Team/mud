// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 6fbc7168d33ce64f2ea701a3333c08adccaa53da $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Handlers for messages sent from the server.
//
//-----------------------------------------------------------------------------

#pragma once

#include <vector>

#include "doomtype.h"

enum parseError_e
{
    PERR_OK,
    PERR_UNKNOWN_HEADER,
    PERR_UNKNOWN_MESSAGE,
    PERR_BAD_DECODE
};

struct Proto
{
    uint8_t        header;
    std::string name;
    size_t      size;
    std::string data;
};

typedef std::vector<Proto> Protos;

const Protos &CL_GetTicProtos();
parseError_e  CL_ParseCommand();
