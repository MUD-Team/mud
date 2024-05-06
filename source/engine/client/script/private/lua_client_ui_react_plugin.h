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
    struct DeferredElement
    {
        DeferredElement()
        {
            Clear();
        }

        void Clear()
        {
            type = "Invalid";
            key.clear();
            text.clear();
            children.clear();
            element = nullptr;
        }

        std::string                  type;
        std::string                  key;
        std::string                  text;
        void                        *element;
        std::vector<DeferredElement> children;
    };

    int GetEventClasses() override;

    void OnInitialise() override;

    void OnShutdown() override;

    void OnElementCreate(Rml::Element *element) override;

    void OnElementDestroy(Rml::Element *element) override;

    void ProcessElement(Rml::Element* parent, DeferredElement* deferred, DeferredElement* cached);

    DeferredElement DeferCreateElement();

    void Render();

    void FinishRender();

    Rml::ElementDocument *mRenderDocument;
    Rml::Element         *mRenderParent;

    DeferredElement mDeferred;
    DeferredElement mCache;

    std::set<std::string> mKnownTypes;

    static MUDReactPlugin *mInstance;
};
