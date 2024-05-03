#pragma once

void LUA_OpenClientState();
void LUA_CloseClientState();
void LUA_ClientGameTicker();

void LUA_CallGlobalClientFunction(const char* function_name);
