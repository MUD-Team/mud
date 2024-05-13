// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
//
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
//  LuaPublic header.
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>

#include "lua.hpp"

lua_State *LUA_OpenState();
void       LUA_CloseState(lua_State *L);

int32_t LUA_DoFile(lua_State *L, const std::string &filepath);

void LUA_CallGlobalFunction(lua_State *L, const char *function_name);