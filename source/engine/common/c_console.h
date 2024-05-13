// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 67aa34514c86ae501d091a86b247df83984523ef $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	C_CONSOLE
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdarg.h>

#include "cmdlib.h"
#include "d_event.h"
#include "d_player.h"

#define MAX_CHATSTR_LEN 128

int32_t            PrintString(int32_t printlevel, const char *string);
int32_t STACK_ARGS Printf_Bold(const char *format, ...);

void C_AddTabCommand(const char *name);
void C_RemoveTabCommand(const char *name);

