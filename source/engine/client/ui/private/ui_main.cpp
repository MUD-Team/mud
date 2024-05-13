// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
//
// Copyright (C) 2024 by The MUD Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  UIMain module.
//
//-----------------------------------------------------------------------------

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
        int32_t width  = I_GetVideoWidth();
        int32_t height = I_GetVideoHeight();

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
