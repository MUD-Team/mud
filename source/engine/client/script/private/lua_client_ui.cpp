
#include <LuaBridge.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Lua.h>

#include "../../ui/private/ui_render.h"
#include "lua_client_private.h"

class LuaUI
{
  public:
    static void open(lua_State *L)
    {
        Rml::Lua::Initialise(L);

        luabridge::getGlobalNamespace(L)
            .beginNamespace("mud")
            .beginClass<LuaUI>("UI")
            .addConstructor<void()>()
            .addFunction(
                "begin_frame", +[](LuaUI *) { ((UIRenderInterface *)Rml::GetRenderInterface())->BeginFrame(); })
            .addFunction(
                "end_frame", +[](LuaUI *) { ((UIRenderInterface *)Rml::GetRenderInterface())->EndFrame(); })
            .endClass()
            .endNamespace();
    }
};

void LUA_OpenClientUI(lua_State *L)
{
    LuaUI::open(L);
}