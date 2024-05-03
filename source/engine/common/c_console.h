// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 67aa34514c86ae501d091a86b247df83984523ef $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	C_CONSOLE
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdarg.h>

#include "cmdlib.h"
#include "d_event.h"
#include "d_player.h"

#define MAX_CHATSTR_LEN 128

int            PrintString(int printlevel, const char *string);
int STACK_ARGS Printf_Bold(const char *format, ...);

void C_AddTabCommand(const char *name);
void C_RemoveTabCommand(const char *name);

