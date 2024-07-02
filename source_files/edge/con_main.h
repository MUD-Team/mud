//----------------------------------------------------------------------------
//  EDGE Console Main
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>

#include "epi_color.h"
#include "sokol_color.h"

extern std::string working_directory;

enum ConsoleVisibility
{
    kConsoleVisibilityNotVisible, // invisible
    kConsoleVisibilityMaximal,    // fullscreen + a command line
    kConsoleVisibilityToggle
};

class ConsoleLine
{
  public:
    std::string line_;

    RGBAColor color_;

  public:
    ConsoleLine(const std::string &text, RGBAColor col = SG_LIGHT_GRAY_RGBA32) : line_(text), color_(col)
    {
    }

    ConsoleLine(const char *text, RGBAColor col = SG_LIGHT_GRAY_RGBA32) : line_(text), color_(col)
    {
    }

    ~ConsoleLine()
    {
    }

    void Append(const char *text)
    {
        line_.append(text);
    }

    void Clear()
    {
        line_.clear();
    }
};

void TryConsoleCommand(const char *cmd);

#ifdef __GNUC__
void ConsolePrint(const char *message, ...) __attribute__((format(printf, 1, 2)));
void ConsoleMessage(const char *message, ...) __attribute__((format(printf, 1, 2)));
void ConsolePlayerMessage(int plyr, const char *message, ...) __attribute__((format(printf, 2, 3)));
#else
void ConsolePrint(const char *message, ...);
void ConsoleMessage(const char *message, ...);
void ConsolePlayerMessage(int plyr, const char *message, ...);
#endif

void ClearConsoleLines();

// Looks up the string in LDF, appends an extra '\n', and then writes it to
// the console. Should be used for most player messages.
void ConsoleMessageLDF(const char *lookup, ...);

void ImportantConsoleMessageLDF(const char *lookup, ...);

// Looks up in LDF.
void PlayerConsoleMessageLDF(int plyr, const char *message, ...);

// this color will apply to the next ConsoleMessage or ConsolePrint call.
void ConsoleMessageColor(RGBAColor col);

// Displays/Hides the console.
void SetConsoleVisible(ConsoleVisibility v);

int MatchConsoleCommands(std::vector<const char *> &list, const char *pattern);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
