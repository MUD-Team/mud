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
//  UIInput header.
//
//-----------------------------------------------------------------------------

#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>

#include "../../sdl/i_sdl.h"

class UIInput
{
  public:
    UIInput();

  private:
    friend class UI;

    void postEvent(SDL_Event &ev);
    void processEvents();

    std::vector<SDL_Event> mSDLEvents;
};