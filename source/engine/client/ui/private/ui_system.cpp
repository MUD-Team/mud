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
//  UISystem module.
//
//-----------------------------------------------------------------------------

#include "ui_system.h"

#include <RmlUi/Core.h>
#include <SDL.h>

UISystemInterface::UISystemInterface()
{
    Rml::SetSystemInterface(this);
}

UISystemInterface::~UISystemInterface()
{
}

double UISystemInterface::GetElapsedTime()
{
    static const Uint64 start     = SDL_GetPerformanceCounter();
    static const double frequency = double(SDL_GetPerformanceFrequency());
    return double(SDL_GetPerformanceCounter() - start) / frequency;
}

void UISystemInterface::SetMouseCursor(const Rml::String &cursor_name)
{
}

void UISystemInterface::SetClipboardText(const Rml::String &text_utf8)
{
    SDL_SetClipboardText(text_utf8.c_str());
}

void UISystemInterface::GetClipboardText(Rml::String &text)
{
    char *raw_text = SDL_GetClipboardText();
    text           = Rml::String(raw_text);
    SDL_free(raw_text);
}
