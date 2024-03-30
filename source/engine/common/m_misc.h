// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 2b5d49d2d250ddced8674ceba9f3fc263ea6ecee $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		Default Config File.
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>

#include "doomtype.h"

void M_LoadDefaults(void);

void STACK_ARGS M_SaveDefaults(std::string filename = "");

std::string M_GetConfigPath(void);
std::string M_ExpandTokens(const std::string &str);
