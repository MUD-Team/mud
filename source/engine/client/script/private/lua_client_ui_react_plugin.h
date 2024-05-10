#pragma once
#include <LuaBridge.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Plugin.h>

class Rml::ElementDocument;
class Rml::Element;

class MUDReactEventListener : public Rml::EventListener
{
  public:
    MUDReactEventListener(Rml::Element *element, const std::string &event, bool capturePhase, int functionIndex)
        : mEvent(event), mCapturePhase(capturePhase), mElement(element), mFunctionIndex(functionIndex)
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
    void ProcessEvent(Rml::Event &event) override;

    std::string   mEvent;
    bool          mCapturePhase;
    Rml::Element *mElement;
    int           mFunctionIndex;
};

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
            children.clear();
            element = nullptr;
            parent  = nullptr;
        }

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
            skip = false;
            type.clear();
            key.clear();
            text.clear();
            children.clear();
            element   = nullptr;
            renderKey = -1;
        }

        bool skip;
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

    // Elements

    void OnElementCreate(Rml::Element *element) override;

    void OnElementDestroy(Rml::Element *element) override;

    void EnumerateElements(CacheElement *cache, Rml::Element *element);

    void ProcessElement(DeferredElement *deferred, Rml::Element *parent, CacheElement *cache);

    DeferredElement DeferCreateElement();

    // Hooks
    void HookUseState();

    // Render

    void Render();

    void FinishRender();

    Rml::ElementDocument *mRenderDocument;
    Rml::Element         *mRenderParent;

    DeferredElement mDeferred;
    int             mCurrentRenderKey;
    int             mCurrentFunctionIndex;

    std::set<std::string> mKnownTypes;

    std::map<Rml::Element *, std::vector<MUDReactEventListener *>> mReactListeners;

    static MUDReactPlugin *mInstance;
};
