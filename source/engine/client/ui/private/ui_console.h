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
//  UIConsole header.
//
//-----------------------------------------------------------------------------

#pragma once

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Types.h>

#include <string>
#include <vector>

enum ConsoleLevel
{
    CONSOLE_INFO,
    CONSOLE_WARNING,
    CONSOLE_ERROR,
    CONSOLE_MAX
};

class ElementConsole : public Rml::Element, public Rml::EventListener
{
  public:
    ElementConsole(const Rml::String &tag);
    ~ElementConsole();

    static void AddMessage(ConsoleLevel level, const std::string& message);

protected:
	void OnUpdate() override;
  void OnChildAdd(Rml::Element* child) override;
  void OnChildRemove(Rml::Element* child) override;
	void ProcessEvent(Rml::Event& event) override;
	void OnLayout() override;


  private:
    struct ConsoleLine
    {
        ConsoleLevel mLevel;
        std::string  mLineSource;
    };

    struct ConsoleType
    {
        ConsoleLevel mLevel;
        std::string  mClassName;
        std::string  mAlertContents;
        std::string  mButtonName;
    };    

    Rml::Element *mConsoleContent = nullptr;
    Rml::Element *mConsoleInput = nullptr;

    ConsoleType mConsoleTypes[CONSOLE_MAX];

    static bool mDirty;
    static std::vector<ConsoleLine> mConsoleLines;

};
