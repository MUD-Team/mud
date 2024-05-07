#pragma once
#include <LuaBridge.h>
#include <RmlUi/Core/Plugin.h>

class Rml::ElementDocument;
class Rml::Element;

class MUDReactPlugin : public Rml::Plugin
{
  public:
    MUDReactPlugin(lua_State *lua_state);
    ~MUDReactPlugin();

  private:
    struct CacheElement
    {
        CacheElement()
        {
            Clear();
        }

        void Clear()
        {
            type.clear();
            key.clear();
            children.clear();
            element   = nullptr;
            parent = nullptr;
        }

        std::string               type;
        std::string               key;
        Rml::Element             *parent;
        Rml::Element             *element;
        std::vector<CacheElement> children;
    };

    struct DeferredElement
    {
        DeferredElement()
        {
            Clear();
        }

        void Clear()
        {
            type.clear();
            key.clear();
            text.clear();
            children.clear();
            element   = nullptr;
            renderKey = -1;
        }

        std::string                  type;
        std::string                  key;
        std::string                  text;
        Rml::Element                *element;
        std::vector<DeferredElement> children;
        int                          renderKey;
    };

    int GetEventClasses() override;

    void OnInitialise() override;

    void OnShutdown() override;

    void OnElementCreate(Rml::Element *element) override;

    void OnElementDestroy(Rml::Element *element) override;

    void EnumerateElements(CacheElement *cache, Rml::Element *element);

    void ProcessElement(DeferredElement *deferred, Rml::Element *parent, CacheElement *cache);

    DeferredElement DeferCreateElement();

    void Render();

    void FinishRender();

    Rml::ElementDocument *mRenderDocument;
    Rml::Element         *mRenderParent;

    DeferredElement mDeferred;
    int             mCurrentRenderKey;

    std::set<std::string> mKnownTypes;

    static MUDReactPlugin *mInstance;
};
