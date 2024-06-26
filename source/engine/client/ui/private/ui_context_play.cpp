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
//  UIContextPlay module.
//
//-----------------------------------------------------------------------------

#include "ui_context_play.h"

#include <assert.h>

#include "i_system.h"
#include "ui_playerview.h"

UIContextPlay::UIContextPlay(Rml::Context *context)
{
    ElementPlayerView::InitializeContext(context);
}

UIContextPlay::~UIContextPlay()
{
}
