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
//  ClientMain module.
//
//-----------------------------------------------------------------------------

#include <LuaBridge.h>

#include "c_console.h"
#include "cl_download.h"
#include "cl_main.h"
#include "d_main.h"
#include "doomstat.h"
#include "g_level.h"
#include "i_system.h"
#include "lua_client_private.h"
#include "script/lua_public.h"


static lua_State *clientState = nullptr;

static void RunTics()
{
    try
    {
        D_RunTics(CL_RunTics, CL_DisplayTics);
    }
    catch (CRecoverableError &error)
    {
        Printf(PRINT_ERROR, "\nERROR: %s\n", error.GetMsg().c_str());

        // [AM] In case an error is caused by a console command.
        // C_ClearCommand();

        CL_QuitNetGame(NQ_SILENT);

        G_ClearSnapshots();

        DThinker::DestroyAllThinkers();

        ::players.clear();

        ::gameaction = ga_nothing;
    }
}

static void MUD_AllocConsole()
{
    static bool console_allocated = false;

#ifdef WIN32
    if (console_allocated)
    {
        return;
    }

    console_allocated = true;
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif
}

class LuaClientMain
{
  public:
    static void open(lua_State *L)
    {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("mud")
            .beginNamespace("client")
            .addFunction("allocate_console", MUD_AllocConsole)
            .addFunction("run_tics", RunTics)
            .addFunction("download_tick", CL_DownloadTick)
            .addProperty("headless", I_IsHeadless)
            .addProperty("nodrawers", &nodrawers)            
            .endNamespace()
            .endNamespace();
    }
};

void LUA_ClientGameTicker()
{
}

void LUA_OpenClientState()
{
    if (clientState)
    {
        I_Error("Lua: Client state already open");
    }

    clientState = LUA_OpenState();

    LuaClientMain::open(clientState);

    LUA_OpenClientOptions(clientState);
    LUA_OpenClientVideo(clientState);
    LUA_OpenClientUI(clientState);
    LUA_OpenClientGame(clientState);

    LUA_DoFile(clientState, "client/scripts/main.lua");
}

void LUA_CloseClientState()
{
    if (!nullptr)
    {
        return;
    }
    LUA_CloseState(clientState);
    clientState = nullptr;
}

void LUA_MainLoop()
{
    LUA_CallGlobalFunction(clientState, "MainLoop");
}

void LUA_Display()
{
    LUA_CallGlobalFunction(clientState, "Display");
}


void LUA_CallGlobalClientFunction(const char* function_name)
{
    LUA_CallGlobalFunction(clientState, function_name);
}