
#include "lua_client_ui_react_plugin.h"

#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <i_system.h>


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

void MUDReactPlugin::ProcessElement(Rml::Element *parent, DeferredElement *deferred, DeferredElement *cached)
{
    Rml::Element *curElement    = nullptr;
    Rml::Element *cachedElement = cached ? (Rml::Element *)cached->element : nullptr;

    if (cached && cached->element && cached->type == deferred->type && cached->key == deferred->key)
    {
        curElement = (Rml::Element *)cached->element;
    }
    else if (cached && cached->element && cached->type == "#text" && deferred->type == "#text")
    {
        curElement = (Rml::Element *)cached->element;
        ((Rml::ElementText *)cachedElement)->SetText(deferred->text);
    }

    if (!curElement)
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

        curElement = nelement.get();

        deferred->element = curElement;

        if (!curElement)
        {
            I_Error("MUD React: Unable to create deferred element");
        }

        if (cachedElement)
        {
            Rml::Element *child = cachedElement->GetFirstChild();
            while (child)
            {
                Rml::ElementPtr c = cachedElement->RemoveChild(child);
                curElement->AppendChild(std::move(c));
                child = cachedElement->GetFirstChild();
            }

            parent->InsertBefore(std::move(nelement), cachedElement);
            Rml::ElementPtr el = parent->RemoveChild(cachedElement);
            RMLUI_ASSERT(el.get() == cachedElement);
        }
        else
        {
            parent->AppendChild(std::move(nelement));
        }
    }
    else
    {
        deferred->element = cached->element;
    }

    if (!curElement)
    {
        I_Error("MUD React: Unable to instantiate element");
    }

    for (size_t i = 0; i < deferred->children.size(); i++)
    {
        ProcessElement(curElement, &deferred->children[i],
                       (cached && i < cached->children.size()) ? &cached->children[i] : nullptr);
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

    // props
    luabridge::LuaRef props(g_L);
    if (top > 1)
    {
        auto propsResult = luabridge::get<luabridge::LuaRef>(g_L, 2);
        RMLUI_ASSERT(propsResult);
        props = propsResult.value();
    }

    // children
    std::vector<luabridge::LuaRef> children;
    for (int i = 3; i <= top; i++)
    {
        children.push_back(luabridge::get<luabridge::LuaRef>(g_L, i).value());
    }

    if (type.isFunction())
    {
        luabridge::LuaResult result = type.call();
        return defer;
    }

    if (type.isString())
    {
        std::string stype = type.cast<std::string>().value();

        if (mKnownTypes.find(stype) == mKnownTypes.end())
        {
            defer.type = "#text";
            defer.text = stype;
            defer.key  = stype;
            return defer;
        }
        else
        {
            defer.type = stype;
        }
    }

    if (props.isTable())
    {
        defer.key = props["key"].cast<std::string>().valueOr("");
    }

    for (auto child : children)
    {
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
            I_Error("Unknown MUD React child type");
        }

        defer.children.push_back(*d);
    }

    return defer;
}

void MUDReactPlugin::Render()
{
    RMLUI_ASSERT(!mRenderDocument);
    RMLUI_ASSERT(!mRenderParent);

    int top = lua_gettop(g_L);
    RMLUI_ASSERT(top == 3);

    Rml::ElementDocument **documentPtr = static_cast<Rml::ElementDocument **>(lua_touserdata(g_L, 1));
    Rml::Element         **parentPtr   = static_cast<Rml::Element **>(lua_touserdata(g_L, 2));
    RMLUI_ASSERT(documentPtr && *documentPtr);
    RMLUI_ASSERT(parentPtr && *parentPtr);
    RMLUI_ASSERT(lua_isfunction(g_L, 3));

    mRenderDocument = *documentPtr;
    mRenderParent   = *parentPtr;

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

    mDeferred = *deferred;

    ProcessElement(mRenderParent, &mDeferred, mCache.type == "Invalid" ? nullptr : &mCache);

    FinishRender();
}

void MUDReactPlugin::FinishRender()
{

    mRenderDocument = nullptr;
    mRenderParent   = nullptr;

    mCache = mDeferred;

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
