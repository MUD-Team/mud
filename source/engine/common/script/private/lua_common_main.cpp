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
//  LuaCommonMain module.
//
//-----------------------------------------------------------------------------

#include <LuaBridge.h>

#include "c_dispatch.h"
#include "doomstat.h"
#include "p_tick.h"

class LuaCommonMain
{
  public:
    static void open(lua_State *L)
    {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("mud")
            .addFunction("p_ticker_pause", P_Ticker_Pause)
            .addFunction(
                "add_command", +[](const std::string &command) { AddCommandString(command); })
            .addProperty("paused", &paused)
            .addProperty("gamestate", &gamestate)
            .endNamespace();
    }
};

void LUA_OpenCommonState(lua_State *commonState)
{
    LuaCommonMain::open(commonState);
}