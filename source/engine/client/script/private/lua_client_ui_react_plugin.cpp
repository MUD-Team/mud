
#include "lua_client_ui_react_plugin.h"

#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <i_system.h>

static lua_State *g_L                       = nullptr;
MUDReactPlugin   *MUDReactPlugin::mInstance = nullptr;

// Calls the associated Lua function.
void MUDReactEventListener::ProcessEvent(Rml::Event &event)
{
    int top = lua_gettop(g_L);
    lua_getfield(g_L, LUA_REGISTRYINDEX, "__mud_react_element_functions");
    lua_pushnumber(g_L, mFunctionIndex);
    lua_gettable(g_L, -2);
    lua_pcall(g_L, 0, 0, 0);
    lua_settop(g_L, top);
}

MUDReactPlugin::MUDReactPlugin(lua_State *lua_state) : mRenderDocument(nullptr), mRenderParent(nullptr)
{
    RMLUI_ASSERT(!g_L);
    g_L = lua_state;

    mInstance = this;

    std::vector<std::string> knownTypes{"div", "tabset", "tab", "panel", "table", "tr", "td", "select", "option"};
    mKnownTypes.insert(knownTypes.begin(), knownTypes.end());
}

MUDReactPlugin::~MUDReactPlugin()
{
    g_L = nullptr;

    mInstance = nullptr;
}

int MUDReactPlugin::GetEventClasses()
{
    return EVT_ELEMENT;
}

void MUDReactPlugin::EnumerateElements(CacheElement *cache, Rml::Element *element)
{
    RMLUI_ASSERT(element);

    cache->parent  = element->GetParentNode();
    cache->element = element;

    Rml::Element *child = element->GetFirstChild();
    while (child)
    {
        CacheElement ccache;
        EnumerateElements(&ccache, child);
        cache->children.emplace_back(ccache);
        child = child->GetNextSibling();
    }
}

void MUDReactPlugin::ProcessElement(DeferredElement *deferred, Rml::Element *parent, CacheElement *cache)
{

    std::string currentKey;
    std::string currentTagName;

    Rml::Element *current = nullptr;

    if (cache && cache->element)
    {
        currentKey     = cache->element->GetAttribute<std::string>("key", "");
        currentTagName = cache->element->GetTagName();

        if (deferred->type == currentTagName)
        {
            if (currentTagName == "#text")
            {
                current = cache->element;
                ((Rml::ElementText *)current)->SetText(deferred->text);
            }
            else if (currentTagName == deferred->type)
            {
                current = cache->element;
            }
        }
        else
        {
            cache->element->GetParentNode()->RemoveChild(cache->element);
            cache->element = nullptr;
            cache->parent  = nullptr;
        }
    }

    if (!current)
    {
        Rml::ElementPtr nelement = nullptr;

        if (deferred->type == "#text")
        {
            nelement = mRenderDocument->CreateTextNode(deferred->text);
        }
        else
        {
            nelement = mRenderDocument->CreateElement(deferred->type);
        }
        
        deferred->element = nelement.get();
        deferred->element->SetAttribute<std::string>("key", deferred->key);

        parent->AppendChild(std::move(nelement));
    }
    else
    {
        deferred->element = current;

        // todo, clear element to defaults
        current->SetAttribute<std::string>("class", "");

        if (current->GetParentNode() != parent)
        {
            cache->parent       = nullptr;
            Rml::ElementPtr ptr = current->GetParentNode()->RemoveChild(current);
            parent->AppendChild(std::move(ptr));
        }
    }

    // We need to parent before goes out of scope, other ElementPtr will destruct

    current = deferred->element;

    if (!current)
    {
        I_FatalError("MUD React: No current element");
    }

    // current properties
    lua_getfield(g_L, LUA_REGISTRYINDEX, "__mud_react_element_properties");
    lua_pushinteger(g_L, deferred->renderKey);
    lua_gettable(g_L, -2);
    if (lua_istable(g_L, -1))
    {
        lua_pushnil(g_L);
        while (lua_next(g_L, -2) != 0)
        {
            std::string       key   = luabridge::get<std::string>(g_L, -2).valueOr("");
            luabridge::LuaRef value = luabridge::get<luabridge::LuaRef>(g_L, -1).value();

            if (value.isBool())
            {
                current->SetAttribute<bool>(key, value.cast<bool>().value());
            }

            if (value.isString())
            {
                current->SetAttribute<std::string>(key, value.cast<std::string>().value());
            }

            if (value.isFunction())
            {
                const std::string &event = key;

                lua_getfield(g_L, LUA_REGISTRYINDEX, "__mud_react_element_functions");
                lua_pushnumber(g_L, mCurrentFunctionIndex);
                auto result = luabridge::push<luabridge::LuaRef>(g_L, value);
                lua_settable(g_L, -3);
                lua_pop(g_L, 1);

                bool capturePhase = true;

                MUDReactEventListener *listener =
                    new MUDReactEventListener(current, event, capturePhase, mCurrentFunctionIndex++);

                auto itr = mReactListeners.find(current);
                if (itr == mReactListeners.end())
                {
                    mReactListeners.insert((std::pair<Rml::Element *, std::vector<MUDReactEventListener *>>(
                        current, std::vector<MUDReactEventListener *>{{listener}})));
                }
                else
                {
                    itr->second.push_back(listener);
                }

                current->AddEventListener(event, listener, capturePhase);
            }

            lua_pop(g_L, 1);
        }
    }

    // result and __mud_react_element_properties
    lua_pop(g_L, 2);

    for (size_t i = 0; i < deferred->children.size(); i++)
    {
        DeferredElement *child = &deferred->children[i];
        ProcessElement(child, deferred->element,
                       (cache && (i < cache->children.size())) ? &cache->children[i] : nullptr);
    }

    if (cache)
    {
        for (size_t i = deferred->children.size(); i < cache->children.size(); i++)
        {
            if (cache->parent)
            {
                Rml::Element *element = cache->children[i].element;
                Rml::Element *parent  = element->GetParentNode();
                RMLUI_ASSERT(parent);
                parent->RemoveChild(element);
            }
        }
    }
}

MUDReactPlugin::DeferredElement MUDReactPlugin::DeferCreateElement()
{
    DeferredElement defer;

    int top = lua_gettop(g_L);
    RMLUI_ASSERT(top > 0);

    // type
    auto typeResult = luabridge::get<luabridge::LuaRef>(g_L, 1);
    RMLUI_ASSERT(typeResult);
    const luabridge::LuaRef &type = typeResult.value();

    // children
    std::vector<luabridge::LuaRef> children;
    for (int i = 3; i <= top; i++)
    {
        luabridge::LuaRef child = luabridge::get<luabridge::LuaRef>(g_L, i).value();
        if (child.isTable())
        {
            int length = child.length();
            for (int i = 0; i < length; i++)
            {
                children.push_back(child[i + 1]);    
            }
        }
        else
        {
            children.push_back(child);
        }

        
    }

    int renderKey = mCurrentRenderKey++;

    // props
    luabridge::LuaRef props(g_L);
    if (top > 1)
    {
        auto propsResult = luabridge::get<luabridge::LuaRef>(g_L, 2);
        RMLUI_ASSERT(propsResult);
        props = propsResult.value();
    }

    if (props.isTable())
    {
        defer.key = props["key"].cast<std::string>().valueOr("");
        lua_getfield(g_L, LUA_REGISTRYINDEX, "__mud_react_element_properties");
        lua_pushinteger(g_L, renderKey);
        luabridge::Result result = luabridge::push(g_L, props);
        if (!result)
        {
            I_FatalError("MUD React: Error pushing element properties %s", result.message().c_str());
        }
        lua_settable(g_L, -3);
        lua_pop(g_L, 1);
    }

    if (type.isFunction())
    {
        if (defer.key.size())
        {
            lua_pushstring(g_L, defer.key.c_str());
            lua_setglobal(g_L, "__mud_react_component_key");
            lua_pushnumber(g_L, defer.renderKey);
            lua_setglobal(g_L, "__mud_react_component_render_key");
        }
        
        luabridge::LuaResult result = type.call(props);

        if (defer.key.size())
        {
            lua_pushnil(g_L);
            lua_setglobal(g_L, "__mud_react_component_key");
            lua_pushnil(g_L);
            lua_setglobal(g_L, "__mud_react_component_render_key");
        }

        if (result.hasFailed())
        {
            I_FatalError("MUD React: %s", result.errorMessage().c_str());
        }
        if (result.size() != 1)
        {
            I_FatalError("MUD React: Functional component returned %u values", result.size());
        }

        if (result[0].isNil())
        {
            DeferredElement d;
            d.skip = true;
            return d;
        }

        if (!result[0].isUserdata())
        {
            I_FatalError("MUD React: Functional component returned non-user data: %s", result[0].tostring());
        }

        DeferredElement *d = result[0].cast<DeferredElement *>().valueOr(nullptr);

        if (!d)
        {
            I_FatalError("MUD React: Functional component could't get DeferredElement");
        }

        d->renderKey = renderKey;

        return *d;
    }

    if (type.isString())
    {
        std::string stype = type.cast<std::string>().value();

        if (mKnownTypes.find(stype) == mKnownTypes.end())
        {
            defer.type      = "#text";
            defer.text      = stype;
            defer.key       = stype;
            defer.renderKey = renderKey;
            return defer;
        }
        else
        {
            defer.type = stype;
        }
    }

    defer.renderKey = renderKey;

    for (auto child : children)
    {
        if (child.isBool() && !child.cast<bool>().value())
        {
            continue;
        }
        if (child.isString())
        {
            const std::string value = child.cast<std::string>().valueOr("");
            DeferredElement   cdefer;
            cdefer.type = "#text";
            cdefer.text = value;
            cdefer.key  = value;
            defer.children.push_back(cdefer);
            continue;
        }

        DeferredElement *d = child.cast<DeferredElement *>().valueOr(nullptr);
        if (!d)
        {
            I_FatalError("Unknown MUD React child type");
        }

        if (!d->skip) {
            defer.children.push_back(*d);
        }
    }

    return defer;
}

void MUDReactPlugin::Render()
{
    RMLUI_ASSERT(!mRenderDocument);
    RMLUI_ASSERT(!mRenderParent);

    const int top = lua_gettop(g_L);
    RMLUI_ASSERT(top == 3);

    Rml::ElementDocument **documentPtr = static_cast<Rml::ElementDocument **>(lua_touserdata(g_L, 1));
    Rml::Element         **parentPtr   = static_cast<Rml::Element **>(lua_touserdata(g_L, 2));
    RMLUI_ASSERT(documentPtr && *documentPtr);
    RMLUI_ASSERT(parentPtr && *parentPtr);
    RMLUI_ASSERT(lua_isfunction(g_L, 3));

    mRenderDocument       = *documentPtr;
    mRenderParent         = *parentPtr;
    mCurrentRenderKey     = 0;
    mCurrentFunctionIndex = 0;

    lua_newtable(g_L);
    lua_setfield(g_L, LUA_REGISTRYINDEX, "__mud_react_element_properties");

    lua_newtable(g_L);
    lua_setfield(g_L, LUA_REGISTRYINDEX, "__mud_react_element_functions");

    lua_newtable(g_L);
    lua_setfield(g_L, LUA_REGISTRYINDEX, "__mud_react_dirty_render_keys");


    // remove current event listeners, todo, optimize

    for (auto listeners : mReactListeners)
    {
        for (auto listener : listeners.second)
        {
            listener->mElement->RemoveEventListener(listener->mEvent, listener, listener->mCapturePhase);
        }
    }

    mReactListeners.clear();

    lua_call(g_L, 0, 1);
    //{
    //    I_FatalError("MUD React Render: %s", lua_tostring(g_L, -1));
    //}

    // result
    DeferredElement *deferred = luabridge::get<DeferredElement *>(g_L, -1).valueOr(nullptr);
    if (!deferred)
    {
        I_FatalError("MUD React render didn't return a deferred");
    }

    CacheElement cache;

    if (mRenderParent->GetFirstChild())
    {
        EnumerateElements(&cache, mRenderParent->GetFirstChild());
    }

    mDeferred = *deferred;

    ProcessElement(&mDeferred, mRenderParent, &cache);

    FinishRender();
}

void MUDReactPlugin::FinishRender()
{

    mRenderDocument = nullptr;
    mRenderParent   = nullptr;

    mDeferred.Clear();
}

void MUDReactPlugin::HookUseState()
{
    __debugbreak();
}

void MUDReactPlugin::OnInitialise()
{
    RMLUI_ASSERT(g_L);

    luabridge::getGlobalNamespace(g_L)
        .beginNamespace("mud")
        .beginNamespace("ui")
        .beginNamespace("react")
        .beginClass<DeferredElement>("DeferredElement")
        .addProperty("type", &DeferredElement::type)
        .addProperty("key", &DeferredElement::key)
        .addProperty("text", &DeferredElement::text)
        .addProperty("element", (&DeferredElement::element))
        .addProperty("children", (&DeferredElement::children))
        .endClass()
        .addFunction(
            "createElement",
            +[] {
                RMLUI_ASSERT(mInstance);
                return mInstance->DeferCreateElement();
            })
        .addFunction(
            "createFragment", +[] { I_Warning("Hi!"); })
        .addFunction(
            "render",
            +[]() {
                RMLUI_ASSERT(mInstance);
                mInstance->Render();
            })
        .addFunction(
            "nativeHookUseState",
            +[]() {
                RMLUI_ASSERT(mInstance);
                mInstance->HookUseState();
            })

        .endNamespace()
        .endNamespace()
        .endNamespace();
}

void MUDReactPlugin::OnShutdown()
{
    g_L = nullptr;
}

void MUDReactPlugin::OnElementCreate(Rml::Element *element)
{
}

void MUDReactPlugin::OnElementDestroy(Rml::Element *element)
{
}
