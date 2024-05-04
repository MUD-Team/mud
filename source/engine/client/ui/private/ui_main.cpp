

#include "ui_main.h"

#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <i_video.h>

#include "c_dispatch.h"
#include "ui_mud_plugin.h"

Rml::UniquePtr<UI> UI::mInstance;

UI::UI()
{
}

UI::~UI()
{
    if (!mInitialized)
    {
        return;
    }

    Rml::Shutdown();

    mRenderInterface.reset();
    mFileInterface.reset();
    mSystemInterface.reset();
    mInput.reset();
}

bool UI::initialize()
{
    RMLUI_ASSERT(!mInstance.get());
    mInstance = Rml::UniquePtr<UI>(new UI());
    return mInstance->initInstance();
}

void UI::shutdown()
{
    mInstance.reset();
}

bool UI::initInstance()
{
    mSystemInterface = Rml::MakeUnique<UISystemInterface>();
    mFileInterface   = Rml::MakeUnique<UIFileInterface>();
    mRenderInterface = Rml::MakeUnique<UIRenderInterface>();
    mInput           = Rml::MakeUnique<UIInput>();

    // RmlUi initialisation.
    Rml::Initialise();

    Rml::RegisterPlugin(new MUDPlugin());

    loadCoreFonts();

    mInitialized = true;

    return true;
}

void UI::loadCoreFonts()
{
    const Rml::String directory = "fonts/";

    struct FontFace
    {
        const char *filename;
        bool        fallback_face;
    };
    FontFace font_faces[] = {
        {"MUD-RussoOne.ttf", true},
    };

    for (const FontFace &face : font_faces)
        Rml::LoadFontFace(directory + face.filename, face.fallback_face);
}

void UI::postEvent(SDL_Event &ev)
{
    if (!mInstance || !mInstance->mInput)
    {
        return;
    }
    return mInstance->mInput->postEvent(ev);
}

void UI::processEvents()
{
    return mInstance->mInput->processEvents();
}

void UI::loadCore()
{
}

bool UI_Initialize()
{
    UI::initialize();
    return true;
}

void UI_Shutdown()
{
    UI::shutdown();
}

void UI_PostEvent(SDL_Event &ev)
{
    UI::postEvent(ev);
}

void UI_OnResize()
{
    Rml::Context *context = Rml::GetContext("play");
    if (context)
    {
        int width  = I_GetVideoWidth();
        int height = I_GetVideoHeight();

        context->SetDimensions(Rml::Vector2i(width, height));
    }
}

void UI_ToggleDebug()
{
    Rml::Context *context = Rml::GetContext("play");
    if (!context)
    {
        return;
    }

    Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
}

void UI_LoadCore()
{

    UI::loadCore();
}

bool UI_RenderInitialized()
{
    UIRenderInterface *interface = (UIRenderInterface *)Rml::GetRenderInterface();
    if (!interface || !interface->getRenderer())
    {
        return false;
    }

    return true;
}

BEGIN_COMMAND(ui_debug)
{
    UI_ToggleDebug();
}
END_COMMAND(ui_debug)
