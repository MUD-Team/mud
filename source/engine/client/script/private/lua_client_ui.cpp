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
//  ClientUI module.
//
//-----------------------------------------------------------------------------

#include <LuaBridge.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Lua.h>

#include "../../ui/private/ui_render.h"
#include "lua_client_private.h"
#include "../../sdl/i_input.h"

class LuaUI
{
  public:
    static void open(lua_State *L)
    {
        Rml::Lua::Initialise(L);

        luabridge::getGlobalNamespace(L)
            .beginNamespace("mud")
            .beginNamespace("ui")
            .addFunction(
                "set_relative_mouse", +[](bool relative) { I_SetRelativeMouseMode(relative); })

            .addFunction(
                "begin_frame", +[]() { ((UIRenderInterface *)Rml::GetRenderInterface())->BeginFrame(); })
            .addFunction(
                "end_frame", +[]() { ((UIRenderInterface *)Rml::GetRenderInterface())->EndFrame(); })
            .endNamespace()
            .endNamespace();
    }
};

void LUA_OpenClientUI(lua_State *L)
{
    LuaUI::open(L);
}