#pragma once

#include <string>

#include "lua.hpp"

lua_State *LUA_OpenState();
void       LUA_CloseState(lua_State *L);

int LUA_RequireFile(lua_State *L, const std::string &filepath);

void LUA_CallGlobalFunction(lua_State *L, const char *function_name);