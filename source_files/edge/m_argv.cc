//------------------------------------------------------------------------
//  EDGE Arguments/Parameters Code
//------------------------------------------------------------------------
//
//  Copyright (C) 2023-2024 The EDGE Team
//  Copyright (C) 2021-2022 The OBSIDIAN Team
//  Copyright (C) 2006-2017 Andrew Apted
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
//------------------------------------------------------------------------

#include "m_argv.h"

#include <string.h>

#include "epi.h"
#include "epi_filesystem.h"
#include "epi_lexer.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "epi_windows.h"
#include "i_system.h"
#ifdef _WIN32
#include <shellapi.h>
#endif

std::vector<std::string> program_argument_list;

//
// ArgvInit
//
// Initialise argument program_argument_list. The strings (and array) are
// copied.
//
// NOTE: doesn't merge multiple uses of an option, hence
//       using ArgvFind() will only return the first usage.
//
#ifdef _WIN32
void ParseArguments(const int argc, const char *const *argv)
{
    (void)argc;
    (void)argv;

    int       win_argc = 0;
    size_t    i;
    wchar_t **win_argv = CommandLineToArgvW(GetCommandLineW(), &win_argc);

    if (!win_argv)
        FatalError("ParseArguments: Could not retrieve command line arguments!\n");

    program_argument_list.reserve(win_argc);

    std::vector<std::string> argv_block;

    for (i = 0; i < win_argc; i++)
    {
        EPI_ASSERT(win_argv[i] != nullptr);
        argv_block.push_back(epi::WStringToUTF8(win_argv[i]));
    }

    LocalFree(win_argv);

    for (i = 0; i < argv_block.size(); i++)
    {
        // Just place argv[0] as is
        if (i == 0)
        {
            program_argument_list.emplace_back(argv_block[i]);
            continue;
        }

        if (argv_block[i][0] == '@')
        { // ignore response files
            continue;
        }

        program_argument_list.emplace_back(argv_block[i]);
    }
}
#else
void ParseArguments(const int argc, const char *const *argv)
{
    EPI_ASSERT(argc >= 0);
    program_argument_list.reserve(argc);

    for (int i = 0; i < argc; i++)
    {
        EPI_ASSERT(argv[i] != nullptr);

#ifdef __APPLE__
        // ignore MacOS X rubbish
        if (argv[i] == "-psn")
            continue;
#endif
        // Just place argv[0] as is
        if (i == 0)
        {
            program_argument_list.emplace_back(argv[i]);
            continue;
        }

        if (argv[i][0] == '@')
        { // ignore response files
            continue;
        }

        program_argument_list.emplace_back(argv[i]);
    }
}
#endif

int FindArgument(std::string_view long_name, int *total_parameters)
{
    EPI_ASSERT(!long_name.empty());

    if (total_parameters)
        *total_parameters = 0;

    size_t p = 0;

    for (; p < program_argument_list.size(); ++p)
    {
        if (!ArgumentIsOption(p))
            continue;

        if (epi::StringCaseCompareASCII(long_name, program_argument_list[p].substr(1)) == 0)
            break;
    }

    if (p == program_argument_list.size())
        return -1;

    if (total_parameters)
    {
        size_t q = p + 1;

        while (q < program_argument_list.size() && !ArgumentIsOption(q))
            ++q;

        *total_parameters = q - p - 1;
    }

    return p;
}

std::string ArgumentValue(std::string_view long_name, int *total_parameters)
{
    EPI_ASSERT(!long_name.empty());

    int pos = FindArgument(long_name, total_parameters);

    if (pos <= 0)
        return "";

    if (pos + 1 < int(program_argument_list.size()) && !ArgumentIsOption(pos + 1))
        return program_argument_list[pos + 1];
    else
        return "";
}

// Sets boolean variable to true if parm (prefixed with `-') is
// present, sets it to false if parm prefixed with `-no' is present,
// otherwise leaves it unchanged.
//
void CheckBooleanParameter(const std::string &parameter, bool *boolean_value, bool reverse)
{
    if (FindArgument(parameter) > 0)
    {
        *boolean_value = !reverse;
        return;
    }

    if (FindArgument(epi::StringFormat("no%s", parameter.c_str())) > 0)
    {
        *boolean_value = reverse;
        return;
    }
}

void CheckBooleanConsoleVariable(const std::string &parameter, ConsoleVariable *variable, bool reverse)
{
    if (FindArgument(parameter) > 0)
    {
        *variable = (reverse ? 0 : 1);
        return;
    }

    if (FindArgument(epi::StringFormat("no%s", parameter.c_str())) > 0)
    {
        *variable = (reverse ? 1 : 0);
        return;
    }
}

void DumpArguments(void)
{
    LogPrint("Command-line Options:\n");

    int i = 0;

    while (i < int(program_argument_list.size()))
    {
        bool pair_it_up = false;

        if (i > 0 && i + 1 < int(program_argument_list.size()) && !ArgumentIsOption(i + 1))
            pair_it_up = true;

        LogPrint("  %s %s\n", program_argument_list[i].c_str(), pair_it_up ? program_argument_list[i + 1].c_str() : "");

        i += pair_it_up ? 2 : 1;
    }
}

bool ArgumentIsOption(const int index)
{
    return program_argument_list.at(index)[0] == '-';
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
