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
//  UIMUDPlugin module.
//
//-----------------------------------------------------------------------------

#include "ui_mud_plugin.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Debugger.h>
#include <assert.h>

#include "i_system.h"
#include "ui_console.h"

MUDPlugin::MUDPlugin()
{
}

int32_t MUDPlugin::GetEventClasses()
{
    return EVT_BASIC | EVT_DOCUMENT;
}

void MUDPlugin::OnInitialise()
{
    mPlayerViewInstancer = Rml::MakeUnique<Rml::ElementInstancerGeneric<ElementPlayerView>>();
    Rml::Factory::RegisterElementInstancer("playerview", mPlayerViewInstancer.get());

    mConsoleInstancer = Rml::MakeUnique<Rml::ElementInstancerGeneric<ElementConsole>>();
    Rml::Factory::RegisterElementInstancer("console", mConsoleInstancer.get());
}

void MUDPlugin::OnShutdown()
{
    mConsoleInstancer.reset();
    mPlayerViewInstancer.reset();
}

void MUDPlugin::OnContextCreate(Rml::Context *context)
{
    if (context->GetName() == "play")
    {
        assert(!mPlayContext.get());
        mPlayContext = Rml::MakeUnique<UIContextPlay>(context);

        // todo: move this to the ui debug console command, we're currently using fonts from here
        Rml::Debugger::Initialise(context);
        Rml::Debugger::SetVisible(false);
    }
}