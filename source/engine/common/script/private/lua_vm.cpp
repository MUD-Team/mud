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
//  LuaVM module.
//
//-----------------------------------------------------------------------------

#include <string>

#include "../lua_public.h"
#include "Poco/Buffer.h"
#include "cmdlib.h"
#include "i_system.h"
#include "lua_debugger.h"
#include "m_fileio.h"

void LUA_OpenCommonState(lua_State *L);

static void    LUA_Sandbox(lua_State *L);
static int32_t LUA_DbgNOP(lua_State *L);
static int32_t LUA_MsgHandler(lua_State *L);
static int32_t LUA_LuaRequire(lua_State *L);
static int32_t LUA_DoFile(lua_State *L, const std::string &filepath, const char *source, size_t source_length);

// TODO: Fixme
static std::vector<std::string> requirePaths;

lua_State *LUA_OpenState()
{
    // we could specify a lua allocator, which would be a good idea to hook up
    // to a debug allocator library for tracing l = lua_newstate(lua_Alloc
    // alloc, nullptr);
    lua_State *L = luaL_newstate();

    /*
    ** these libs are loaded by lua.c and are readily available to any Lua
    ** program
    */
    const luaL_Reg loadedlibs[] = {
        {LUA_GNAME, luaopen_base},          {LUA_LOADLIBNAME, luaopen_package}, {LUA_OSLIBNAME, luaopen_os},
        {LUA_COLIBNAME, luaopen_coroutine}, {LUA_TABLIBNAME, luaopen_table},    {LUA_STRLIBNAME, luaopen_string},
        {LUA_MATHLIBNAME, luaopen_math},    {LUA_UTF8LIBNAME, luaopen_utf8},    {nullptr, nullptr}};

    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++)
    {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1); /* remove lib */
    }

    lua_newtable(L);
    lua_setglobal(L, "__mud_modules");

    lua_pushcfunction(L, LUA_LuaRequire);
    lua_setglobal(L, "require");

    LUA_Sandbox(L);

    LUA_OpenCommonState(L);

    if (true) // lua_debug.d_)
    {
        lua_newtable(L);
        lua_setglobal(L, "__lua_debugger_source");
        dbg_setup_default(L);
    }
    else
    {
        lua_pushcfunction(L, LUA_DbgNOP);
        lua_setglobal(L, "dbg");
    }

    assert(!lua_gettop(L));

    return L;
}

void LUA_CloseState(lua_State *L)
{
    lua_close(L);
}

int32_t LUA_DoFile(lua_State *L, const std::string &filepath)
{
    std::string path;
    M_ExtractFilePath(filepath, path);
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path.back() != '/')
    {
        path.append("/");
    }
    requirePaths.push_back(path);

    if (!filepath.size())
    {
        I_Error("LuaVM: Missing filepath");
    }

    PHYSFS_file *fp = PHYSFS_openRead(filepath.c_str());
    if (!fp)
    {
        I_Error("LuaVM: Unable to open %s", filepath.c_str());
    }

    PHYSFS_sint64 length = PHYSFS_fileLength(fp);

    if (!length)
    {
        I_Error("LuaVM: Zero length file %s", filepath.c_str());
    }

    Poco::Buffer<char> buffer(length + 1);

    if (length != PHYSFS_readBytes(fp, buffer.begin(), length))
    {
        I_Error("LuaVM: Incorrect bytes read for file %s", filepath.c_str());
    }

    PHYSFS_close(fp);

    buffer[length] = 0;
    int32_t result = LUA_DoFile(L, filepath, buffer.begin(), length);

    requirePaths.pop_back();
    return result;
}

int32_t LUA_DoFile(lua_State *L, const std::string &filepath, const char *source, size_t source_length)
{
    if (true) // lua_debug.d_)
    {
        lua_getglobal(L, "__lua_debugger_source");
        lua_getfield(L, -1, filepath.c_str());
        if (lua_isstring(L, -1))
        {
            I_Error("LUA: Redundant execution of %s", filepath.c_str());
            lua_pop(L, 2);
            return 0;
        }
        lua_pop(L, 1);
        lua_pushstring(L, source);
        lua_setfield(L, -2, filepath.c_str());
        lua_pop(L, 1);
    }
    int32_t top    = lua_gettop(L);
    int32_t status = luaL_loadbuffer(L, source, source_length, (std::string("@") + filepath).c_str());

    if (status != LUA_OK)
    {
        I_Error(StrFormat("LUA: Error compiling %s : %s\n", filepath.c_str() ? filepath.c_str() : "???",
                          lua_tostring(L, -1))
                    .c_str());
    }

    if (true) // lua_debug.d_)
    {
        status = dbg_pcall(L, 0, LUA_MULTRET, 0);
    }
    else
    {
        int32_t base = lua_gettop(L);         // function index
        lua_pushcfunction(L, LUA_MsgHandler); // push message handler */
        lua_insert(L, base);                  // put it under function and args */
        status = lua_pcall(L, 0, LUA_MULTRET, base);
        lua_remove(L, base);
    }

    if (status != LUA_OK)
    {
        I_Error(StrFormat("LUA: Error in %s\n", filepath.c_str() ? filepath.c_str() : "???").c_str(),
                lua_tostring(L, -1));
    }

    return lua_gettop(L) - top;
}

static int32_t LUA_LuaRequire(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);

    if (!requirePaths.size())
    {
        I_Error("Lua Loader has no require paths");
    }

    std::string path = M_CleanPath(requirePaths.back() + name);
    std::string ext;
    M_ExtractFileExtension(path, ext);
    if (ext != "lua")
    {
        path.append(".lua");
    }
    std::replace(path.begin(), path.end(), '\\', '/');
    if (!PHYSFS_exists(path.c_str()))
    {
        path = std::string(name) + ".lua";
        std::replace(path.begin(), path.end(), '\\', '/');

        if (!PHYSFS_exists(path.c_str()))
        {
            I_Error("Unable to resolve require %s", name);
        }
    }

    lua_getglobal(L, "__mud_modules");
    lua_getfield(L, -1, path.c_str());

    if (!lua_isnil(L, -1))
    {
        lua_remove(L, -2);
        assert(lua_gettop(L) == 2);
        return 1;
    }

    lua_pop(L, 1);

    int32_t result = LUA_DoFile(L, path.c_str());

    if (result == 0)
    {
        lua_newtable(L);
        result = 1;
    }

    lua_pushvalue(L, -1);
    lua_setfield(L, -3, path.c_str());
    lua_remove(L, -2);

    // result and the name
    assert(lua_gettop(L) == 2);
    return result;
}

int32_t LUA_MsgHandler(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);
    if (msg == nullptr)
    {                                            /* is error object not a string? */
        if (luaL_callmeta(L, 1, "__tostring") && /* does it have a metamethod */
            lua_type(L, -1) == LUA_TSTRING)      /* that produces a string? */
            return 1;                            /* that is the message */
        else
            msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1); /* append a standard traceback */
    return 1;                     /* return the traceback */
}

static int32_t LUA_SandboxWarning(lua_State *L)
{
    const char *function_name = luaL_checkstring(L, lua_upvalueindex(1));

#ifdef CLIENT_APP
    I_Warning("LUA: Called sandbox disabled function %s\n", function_name);
#else
    Printf("WARNING: LUA: Called sandbox disabled function %s\n", function_name);
#endif

    return 0;
}

static void LUA_SandboxModule(lua_State *L, const char *module_name, const char **functions)
{
    int32_t i = 0;
    lua_getglobal(L, module_name);
    while (const char *function_name = functions[i++])
    {
        lua_pushfstring(L, "%s.%s", module_name, function_name);
        lua_pushcclosure(L, LUA_SandboxWarning, 1);
        lua_setfield(L, -2, function_name);
    }
    lua_pop(L, 1);
}

void LUA_Sandbox(lua_State *L)
{
    // clear out search path and loadlib
    lua_getglobal(L, "package");
    lua_pushnil(L);
    lua_setfield(L, -2, "loadlib");
    lua_pushnil(L);
    lua_setfield(L, -2, "searchpath");
    // pop package off stack
    lua_pop(L, 1);

    // os module
    const char *os_functions[] = {"execute", "exit", "getenv", "remove", "rename", "setlocale", "tmpname", nullptr};
    LUA_SandboxModule(L, "os", os_functions);

    // base/global functions
    const char *base_functions[] = {"LUA_dofile", "loadfile", nullptr};
    LUA_SandboxModule(L, "_G", base_functions);

    // if debugging is enabled, load debug/io libs and sandbox
    if (true) // lua_debug.d_)
    {
        // open the debug library and io libraries
        luaL_requiref(L, LUA_DBLIBNAME, luaopen_debug, 1);
        luaL_requiref(L, LUA_IOLIBNAME, luaopen_io, 1);
        lua_pop(L, 2);

        const char *io_functions[] = {"close", "input", "lines", "open", "output", "popen", "tmpfile", "type", nullptr};
        LUA_SandboxModule(L, "io", io_functions);
    }
}

// NOP dbg() for when debugger is disabled and someone has left some breakpoints in code
static bool dbg_nop_warn = false;
int32_t     LUA_DbgNOP(lua_State *L)
{
    if (!dbg_nop_warn)
    {
        dbg_nop_warn = true;
#ifdef CLIENT_APP
        I_Warning("LUA: dbg() called without lua_debug being set.  Please check that "
                  "a stray dbg call didn't get left "
                  "in source.");
#else
        Printf("WARNING: LUA: dbg() called without lua_debug being set.  Please check that "
               "a stray dbg call didn't get left "
               "in source.");
#endif
    }
    return 0;
}

static void LuaError(const char *msg, const char *luaerror)
{
    std::string error(luaerror);
    std::replace(error.begin(), error.end(), '\t', '>');
    I_Error((msg + error).c_str());
}

void LUA_CallGlobalFunction(lua_State *L, const char *function_name)
{

    int32_t top = lua_gettop(L);
    lua_getglobal(L, function_name);
    int32_t status = 0;
    if (true) // lua_debug.d_)
    {
        status = dbg_pcall(L, 0, 0, 0);
    }
    else
    {
        int32_t base = lua_gettop(L);         // function index
        lua_pushcfunction(L, LUA_MsgHandler); // push message handler */
        lua_insert(L, base);                  // put it under function and args */

        status = lua_pcall(L, 0, 0, base);
    }

    if (status != LUA_OK)
    {
        LuaError(StrFormat("Error calling global function %s\n", function_name).c_str(), lua_tostring(L, -1));
    }

    lua_settop(L, top);
}
