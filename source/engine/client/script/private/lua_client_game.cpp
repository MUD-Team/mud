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
//  ClientGame module.
//
//-----------------------------------------------------------------------------

#include <LuaBridge.h>
#include "lua_client_private.h"

class LuaClientGame
{
  public:
    static void open(lua_State *L)
    {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("mud")
            .beginClass<LuaClientGame>("ClientGame")
            .addConstructor<void()>()
            .endClass()
            .endNamespace();
    }
};

void LUA_OpenClientGame(lua_State *L)
{
    LuaClientGame::open(L);
}