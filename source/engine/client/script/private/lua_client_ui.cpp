
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