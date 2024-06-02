// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: e7b26ff55265c64f835d2dceb76bab9b7044396a $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2020 by The Odamex Team.
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
//	C_CONSOLE
//
//-----------------------------------------------------------------------------

#include "c_console.h"

#include <stdarg.h>

#include <algorithm>
#include <list>

#include "c_bind.h"
#include "c_dispatch.h"
#include "cl_download.h"
#include "cl_responderkeys.h"
#include "g_game.h"
#include "g_gametype.h"
#include "i_system.h"
#include "i_video.h"
#include "m_alloc.h"
#include "m_fileio.h"
#include "mud_includes.h"
#include "r_main.h"
#include "s_sound.h"
#include "v_palette.h"
#include "v_textcolors.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

// These functions are standardized in C++11, POSIX standard otherwise
#if defined(_WIN32) && (__cplusplus <= 199711L)
#define vsnprintf _vsnprintf
#define snprintf  _snprintf
#endif

static const int32_t MAX_LINE_LENGTH = 8192;

static bool   ShouldTabCycle = false;
static size_t NextCycleIndex = 0;
static size_t PrevCycleIndex = 0;


EXTERN_CVAR(con_coloredmessages)
EXTERN_CVAR(con_buffersize)
EXTERN_CVAR(show_messages)
EXTERN_CVAR(print_stdout)
EXTERN_CVAR(con_notifytime)

EXTERN_CVAR(message_showpickups)
EXTERN_CVAR(message_showobituaries)

class ConsoleCompletions
{
    std::vector<std::string> _completions;
    size_t                   _maxlen;

  public:
    void add(const std::string &completion)
    {
        _completions.push_back(completion);
        if (_maxlen < completion.length())
            _maxlen = completion.length();
    }

    const std::string &at(size_t index) const
    {
        return _completions.at(index);
    }

    void clear()
    {
        _completions.clear();
        _maxlen = 0;
    }

    bool empty() const
    {
        return _completions.empty();
    }

    //
    // Get longest common substring of completions.
    //
    std::string getCommon() const
    {
        bool        diff = false;
        std::string common;

        for (size_t index = 0;; index++)
        {
            char compare = '\xFF';

            std::vector<std::string>::const_iterator it = _completions.begin();
            for (; it != _completions.end(); ++it)
            {
                if (index >= it->length())
                {
                    // End of string, this is an implicit failed match.
                    diff = true;
                    break;
                }

                if (compare == '\xFF')
                {
                    // Set character to compare against.
                    compare = it->at(index);
                }
                else if (compare != it->at(index))
                {
                    // Found a different character.
                    diff = true;
                    break;
                }
            }

            // If we found a different character, break, otherwise add it
            // to the string we're going to return and try the next index.
            if (diff == true)
                break;
            else
                common += compare;
        }

        return common;
    }

    size_t getMaxLen() const
    {
        return _maxlen;
    }

    size_t size() const
    {
        return _completions.size();
    }
};

// ============================================================================
//
// Console object definitions
//
// ============================================================================

static ConsoleCompletions CmdCompletions;

/****** Tab completion code ******/

typedef std::map<std::string, size_t> tabcommand_map_t; // name, use count
tabcommand_map_t                     &TabCommands()
{
    static tabcommand_map_t _TabCommands;
    return _TabCommands;
}

void C_AddTabCommand(const char *name)
{
    tabcommand_map_t::iterator it = TabCommands().find(StdStringToLower(name));

    if (it != TabCommands().end())
        TabCommands()[name]++;
    else
        TabCommands()[name] = 1;
}

void C_RemoveTabCommand(const char *name)
{
    tabcommand_map_t::iterator it = TabCommands().find(StdStringToLower(name));

    if (it != TabCommands().end())
        if (!--it->second)
            TabCommands().erase(it);
}

//
// Start tab cycling.
//
// Note that this initial state points to the front and back of the completions
// index, which is a unique state that is not possible to get into after you
// start hitting tab.
//
static void TabCycleStart()
{
    ::ShouldTabCycle = true;
    ::NextCycleIndex = 0;
    ::PrevCycleIndex = CmdCompletions.size() - 1;
}

//
// Given an specific completion index, determine the next and previous index.
//
static void TabCycleSet(size_t index)
{
    // Find the next index.
    if (index >= CmdCompletions.size() - 1)
        ::NextCycleIndex = 0;
    else
        ::NextCycleIndex = index + 1;

    // Find the previous index.
    if (index == 0)
        ::PrevCycleIndex = CmdCompletions.size() - 1;
    else
        ::PrevCycleIndex = index - 1;
}

//
// Get out of the tab cycle state.
//
static void TabCycleClear()
{
    ::ShouldTabCycle = false;
    ::NextCycleIndex = 0;
    ::PrevCycleIndex = 0;
}

enum TabCompleteDirection
{
    TAB_COMPLETE_FORWARD,
    TAB_COMPLETE_BACKWARD
};

//
// Handle tab-completion and cycling.
//
static void TabComplete(TabCompleteDirection dir)
{
    /*
    // Pressing tab twice in a row starts cycling the completion.
    if (::ShouldTabCycle && ::CmdCompletions.size() > 0)
    {
        if (dir == TAB_COMPLETE_FORWARD)
        {
            size_t index = ::NextCycleIndex;
            ::CmdLine.replaceString(::CmdCompletions.at(index));
            TabCycleSet(index);
        }
        else if (dir == TAB_COMPLETE_BACKWARD)
        {
            size_t index = ::PrevCycleIndex;
            ::CmdLine.replaceString(::CmdCompletions.at(index));
            TabCycleSet(index);
        }
        return;
    }

    // Clear the completions.
    ::CmdCompletions.clear();

    // Figure out what we need to autocomplete.
    size_t tabStart = ::CmdLine.text.find_first_not_of(' ', 0);
    if (tabStart == std::string::npos)
        tabStart = 0;
    size_t tabEnd = ::CmdLine.text.find(' ', 0);
    if (tabEnd == std::string::npos)
        tabEnd = ::CmdLine.text.length();
    size_t tabLen = tabEnd - tabStart;

    // Don't complete if the cursor is past the command.
    if (::CmdLine.cursor_position >= tabEnd + 1)
        return;

    // Find all substrings.
    std::string                sTabPos = StdStringToLower(::CmdLine.text.substr(tabStart, tabLen));
    const char                *cTabPos = sTabPos.c_str();
    tabcommand_map_t::iterator it      = TabCommands().lower_bound(sTabPos);
    for (; it != TabCommands().end(); ++it)
    {
        if (strncmp(cTabPos, it->first.c_str(), sTabPos.length()) == 0)
            ::CmdCompletions.add(it->first.c_str());
    }

    if (::CmdCompletions.size() > 1)
    {
        // Get common substring of all completions.
        std::string common = ::CmdCompletions.getCommon();
        ::CmdLine.replaceString(common);
    }
    else if (::CmdCompletions.size() == 1)
    {
        // We found a single completion, use it.
        ::CmdLine.replaceString(::CmdCompletions.at(0));
        ::CmdCompletions.clear();
    }

    // If we press tab twice, cycle the command.
    TabCycleStart();
    */
}

//
// C_AddNotifyString
//
// Prioritise messages on top of screen
// Break up the lines so that they wrap around the screen boundary
//
void C_AddNotifyString(int32_t printlevel, const char *color_code, const char *source)
{
}

//
// C_PrintStringStdOut
//
// Prints the given string to stdout, stripping away any color markup
// escape codes.
//
static int32_t C_PrintStringStdOut(const char *str)
{
    std::string sanitized_str(str);
    StripColorCodes(sanitized_str);

    printf("%s", sanitized_str.c_str());
    fflush(stdout);

    return sanitized_str.length();
}

static int32_t VPrintf(int32_t printlevel, const char *color_code, const char *format, va_list parms)
{
    char outline[MAX_LINE_LENGTH], outlinelog[MAX_LINE_LENGTH];

    extern bool gameisdead;
    if (gameisdead)
        return 0;

    vsnprintf(outline, ARRAY_LENGTH(outline), format, parms);

    // denis - 0x07 is a system beep, which can DoS the console (lol)
    // ToDo: there may be more characters not allowed on a consoleprint,
    // maybe restrict a few ASCII stuff later on ?
    int32_t len = strlen(outline);
    for (int32_t i = 0; i < len; i++)
    {
        if (outline[i] == 0x07)
            outline[i] = '.';
    }

    // Prevents writing a whole lot of new lines to the log file
    if (gamestate)
    {
        strcpy(outlinelog, outline);

        // [Nes] - Horizontal line won't show up as-is in the logfile.
        for (int32_t i = 0; i < len; i++)
        {
            if (outlinelog[i] == '\35' || outlinelog[i] == '\36' || outlinelog[i] == '\37')
                outlinelog[i] = '=';
        }

        // Up the row buffer for the console.
        // This is incremented here so that any time we
        // print something we know about it.  This feels pretty hacky!

        // We need to know if there were any new lines being printed
        // in our string.

        int32_t newLineCount = std::count(outline, outline + strlen(outline), '\n');
    }

    if (print_stdout)
        C_PrintStringStdOut(outline);

    std::string sanitized_str(outline);

    //if (!con_coloredmessages)
    StripColorCodes(sanitized_str);    

    void UI_PrintString(printlevel_t print_level, const std::string &);
    UI_PrintString((printlevel_t)printlevel, sanitized_str);

    // Once done, log
    if (LOG.is_open())
    {
        // Strip if not already done
        if (con_coloredmessages)
            StripColorCodes(sanitized_str);

        LOG << sanitized_str;
        LOG.flush();
    }

#if defined(_WIN32) && defined(_DEBUG)
    // [AM] Since we don't have stdout/stderr in a non-console Win32 app,
    //      this outputs the string to the "Output" window.
    OutputDebugStringA(sanitized_str.c_str());
#endif

    return len;
}

FORMAT_PRINTF(1, 2) int32_t STACK_ARGS Printf(const char *format, ...)
{
    va_list argptr;
    int32_t     count;

    va_start(argptr, format);
    count = VPrintf(PRINT_HIGH, TEXTCOLOR_NORMAL, format, argptr);
    va_end(argptr);

    return count;
}

FORMAT_PRINTF(2, 3) int32_t STACK_ARGS Printf(int32_t printlevel, const char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    int32_t count = VPrintf(printlevel, TEXTCOLOR_NORMAL, format, argptr);
    va_end(argptr);

    return count;
}

FORMAT_PRINTF(1, 2) int32_t STACK_ARGS Printf_Bold(const char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    int32_t count = VPrintf(PRINT_HIGH, TEXTCOLOR_BOLD, format, argptr);
    va_end(argptr);

    return count;
}

FORMAT_PRINTF(1, 2) int32_t STACK_ARGS DPrintf(const char *format, ...)
{
    if (developer || devparm)
    {
        va_list argptr;

        va_start(argptr, format);
        int32_t count = VPrintf(PRINT_WARNING, TEXTCOLOR_NORMAL, format, argptr);
        va_end(argptr);
        return count;
    }

    return 0;
}

BEGIN_COMMAND(history)
{
}
END_COMMAND(history)

BEGIN_COMMAND(clear)
{
}
END_COMMAND(clear)

BEGIN_COMMAND(echo)
{
    if (argc > 1)
    {
        std::string str = C_ArgCombine(argc - 1, (const char **)(argv + 1));
        Printf(PRINT_HIGH, "%s\n", str.c_str());
    }
}
END_COMMAND(echo)

BEGIN_COMMAND(toggleconsole)
{
}
END_COMMAND(toggleconsole)
