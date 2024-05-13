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
//  UIMain header.
//
//-----------------------------------------------------------------------------

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