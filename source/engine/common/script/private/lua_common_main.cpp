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
            .endNamespace();
    }
};

void LUA_OpenCommonState(lua_State *commonState)
{
    LuaCommonMain::open(commonState);
}