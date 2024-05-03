
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