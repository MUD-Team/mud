// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 33285cd42e59a225f01242cd7b082b64cdf7fc75 $
//
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
//	Voting-specific stuff.
//
//-----------------------------------------------------------------------------

#include "mud_includes.h"

/**
 * A string array used to associate vote types with command names.
 */
const char *vote_type_cmd[] = {"???",        "kick",       "forcespec", "forcestart", "randcaps",
                               "randpickup", "map",        "nextmap",   "randmap",    "restart",
                               "fraglimit",  "scorelimit", "timelimit", "coinflip",   "???"};
