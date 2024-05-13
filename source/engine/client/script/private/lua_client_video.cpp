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
//  ClientVideo module.
//
//-----------------------------------------------------------------------------

#include <LuaBridge.h>

#include "i_video.h"
#include "lua_client_private.h"
#include "v_video.h"

class LuaVideo
{
  public:
    static void open(lua_State *L)
    {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("mud")
            .beginNamespace("video")
            .addProperty("width", I_GetVideoWidth)
            .addProperty("height", I_GetVideoHeight)
            .addProperty("initialized", I_VideoInitialized)
            .addFunction(
                "start_refresh",
                +[] {
                    if (!I_VideoInitialized())
                    {
                        return;
                    }
                    I_GetWindow()->startRefresh();
                })
            .addFunction(
                "finish_refresh",
                +[] {
                    if (!I_VideoInitialized())
                    {
                        return;
                    }
                    I_GetWindow()->finishRefresh();
                })

            .addFunction(
                "adjust_video_mode", +[]() { V_AdjustVideoMode(); })
            .endNamespace()
            .endNamespace();
    }
};

void LUA_OpenClientVideo(lua_State *L)
{
    LuaVideo::open(L);
}