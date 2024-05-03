
#pragma once

#include "../ui_public.h"
#include "ui_file.h"
#include "ui_input.h"
#include "ui_render.h"
#include "ui_system.h"


class UI
{
  public:
    ~UI();
    static bool initialize();
    static void shutdown();
    static void postEvent(SDL_Event &ev);
    static void processEvents();
    static void loadCore();

    static UI *get()
    {
        UI *instance = mInstance.get();
        RMLUI_ASSERT(instance);
        return instance;
    }

  private:
    UI();

    void loadCoreFonts();

    bool initInstance();

    bool mInitialized = false;

    Rml::UniquePtr<UISystemInterface> mSystemInterface;
    Rml::UniquePtr<UIFileInterface>   mFileInterface;
    Rml::UniquePtr<UIInput>           mInput;
    Rml::UniquePtr<UIRenderInterface> mRenderInterface;

    static Rml::UniquePtr<UI> mInstance;
};