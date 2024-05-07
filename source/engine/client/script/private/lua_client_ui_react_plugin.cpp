
#include "lua_client_ui_react_plugin.h"

#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Core/EventListener.h>
#include <i_system.h>

class MUDReactEventListener : public Rml::EventListener
{
  public:
    MUDReactEventListener(Rml::Element *element, luabridge::LuaRef &&function) : function(function)
    {
    }

    virtual ~MUDReactEventListener()
    {
    }

    // Deletes itself, which also unreferences the Lua function.
    void OnDetach(Rml::Element *element) override
    {
        // We consider this listener owned by its element, so we must delete ourselves when
        // we detach (probably because element was removed).
        delete this;
    }

    // Calls the associated Lua function.
    void ProcessEvent(Rml::Event &event) override
    {
        function.call();
    }

  private:
    luabridge::LuaRef     function;
    Rml::Element         *attached       = nullptr;
    Rml::ElementDocument *owner_document = nullptr;
};

static lua_State *g_L = nullptr;

MUDReactPlugin *MUDReactPlugin::mInstance = nullptr;

MUDReactPlugin::MUDReactPlugin(lua_State *lua_state) : mRenderDocument(nullptr), mRenderParent(nullptr)
{
    RMLUI_ASSERT(!g_L);
    g_L = lua_state;

    mInstance = this;

    std::vector<std::string> knownTypes{"div"};
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

                if (cache->key != deferred->key)
                {
                    ((Rml::ElementText *)current)->SetText(deferred->text);
                }
                
            }
            else if (currentTagName == deferred->type)
            {
                current = cache->element;
            }
        }
    }

    if (!current)
    {
        Rml::ElementPtr nelement = nullptr;

        if (deferred->type == "div")
        {
            nelement = mRenderDocument->CreateElement(deferred->type);
        }
        else if (deferred->type == "#text")
        {
            nelement = mRenderDocument->CreateTextNode(deferred->text);
        }

        deferred->element = nelement.get();
        deferred->element->SetAttribute<std::string>("key", deferred->key);

        parent->AppendChild(std::move(nelement));
    }
    else
    {
        deferred->element = current;

        if (current->GetParentNode() != parent)
        {
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

            if (value.isFunction() && (!cache || current != cache->element))
            {
                MUDReactEventListener *listener = new MUDReactEventListener(current, std::move(value));
                current->AddEventListener(key, listener, true);
                // curElement->AddEventListener(key, +[]{}, false);
                // curElement->SetAttribute<std::string>(key, value.cast<std::string>().value());
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

    if (cache && cache->element)
    {
        Rml::Element *child = cache->element->GetChild(deferred->children.size());
        while (child)
        {
            Rml::Element *next = child->GetNextSibling();
            child->GetParentNode()->RemoveChild(child);
            child = next;
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
        children.push_back(luabridge::get<luabridge::LuaRef>(g_L, i).value());
    }

    if (type.isFunction())
    {
        luabridge::LuaResult result = type.call();
        if (result.hasFailed())
        {
            I_FatalError("MUD React: %s", result.errorMessage().c_str());
        }
        if (result.size() != 1)
        {
            I_FatalError("MUD React: Functional component returned %u values", result.size());
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

        d->renderKey = mCurrentRenderKey++;

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
            defer.renderKey = mCurrentRenderKey++;
            return defer;
        }
        else
        {
            defer.type = stype;
        }
    }

    defer.renderKey = mCurrentRenderKey++;

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
        lua_pushinteger(g_L, defer.renderKey);
        luabridge::Result result = luabridge::push(g_L, props);
        if (!result)
        {
            I_FatalError("MUD React: Error pushing element properties %s", result.message().c_str());
        }
        lua_settable(g_L, -3);
        lua_pop(g_L, 1);
    }

    for (auto child : children)
    {
        if (child.isBool() && !child.cast<bool>().value())
        {
            // DeferredElement   cdefer;
            // cdefer.skip = true;
            // defer.children.push_back(cdefer);
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

        defer.children.push_back(*d);
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

    mRenderDocument   = *documentPtr;
    mRenderParent     = *parentPtr;
    mCurrentRenderKey = 0;

    // this is going to GC functions and stuff stored in props
    lua_newtable(g_L);
    lua_setfield(g_L, LUA_REGISTRYINDEX, "__mud_react_element_properties");

    if (lua_pcall(g_L, 0, 1, 0) != 0)
    {
        I_FatalError("MUD React Render: %s", lua_tostring(g_L, -1));
    }

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
    Rml::Element *yay = element;
}

void MUDReactPlugin::OnElementDestroy(Rml::Element *element)
{
}
