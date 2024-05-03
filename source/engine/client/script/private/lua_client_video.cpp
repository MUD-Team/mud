
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