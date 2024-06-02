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
//  UIMUDPlugin header.
//
//-----------------------------------------------------------------------------

#pragma once

#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/Plugin.h>

#include "ui_context_play.h"
#include "ui_playerview.h"

class ElementConsole;

class MUDPlugin : public Rml::Plugin
{
  public:
    MUDPlugin();

  private:
    int32_t GetEventClasses() override;

    void OnInitialise() override;

    void OnShutdown() override;

    void OnContextCreate(Rml::Context *context) override;

    // Main Context
    // void InitializeMainContext(Rml::Context* context);
    // bool LoadConsoleElement();

    Rml::UniquePtr<Rml::Context> mMainContext;

    Rml::UniquePtr<UIContextPlay> mPlayContext;

    // Player View
    Rml::UniquePtr<Rml::ElementInstancerGeneric<ElementPlayerView>> mPlayerViewInstancer;

    // Player View
    Rml::UniquePtr<Rml::ElementInstancerGeneric<ElementConsole>> mConsoleInstancer;
};
