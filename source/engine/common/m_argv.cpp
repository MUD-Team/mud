// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 94efe5c9d92907e3d2c665e6e02f15eab90654dd $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//  Command-line arguments
//
//-----------------------------------------------------------------------------

#include "m_argv.h"

#include <algorithm>

#include "Poco/Buffer.h"
#include "cmdlib.h"
#include "i_system.h"
#include "m_fileio.h"
#include "mud_includes.h"
#include "win32inc.h"
#ifdef _WIN32
// Need for wide string arg stuff - Dasho
// Must go after win32inc.h...should we just throw it in there?
#include <shellapi.h>
#include "Poco/UnicodeConverter.h"
#endif

IMPLEMENT_CLASS(DArgs, DObject)

DArgs::DArgs()
{
}

DArgs::DArgs(uint32_t argc, char **argv)
{
    if (argv)
        CopyArgs(argc, argv);
}

DArgs::DArgs(const DArgs &other) : args(other.args)
{
}

DArgs::~DArgs()
{
    FlushArgs();
}

DArgs &DArgs::operator=(const DArgs &other)
{
    args = other.args;
    return *this;
}

const char *DArgs::operator[](size_t n)
{
    return GetArg(n);
}

void DArgs::SetArgs(uint32_t argc, char **argv)
{
    CopyArgs(argc, argv);
}

// For Windows, we need to get the wide string version of the arguments
// to properly convert paths passed via -file, etc to UTF-8 - Dasho
#ifdef _WIN32
void DArgs::CopyArgs(uint32_t argc, char **argv)
{
    if (!argv || !argc)
        return;

    int32_t       win_argc = 0;
    uint32_t    i;
    wchar_t **win_argv = CommandLineToArgvW(GetCommandLineW(), &win_argc);

    if (!win_argv)
        I_Error("Could not retrieve command line arguments!\n");

    args.resize(win_argc);

    for (i = 0; i < win_argc; i++)
    {
        if (win_argv[i] == NULL)
            I_Error("Error parsing command line arguments!\n");
        Poco::UnicodeConverter::convert(win_argv[i], args[i]);
    }

    LocalFree(win_argv);
}
#else
void DArgs::CopyArgs(uint32_t argc, char **argv)
{
    args.clear();

    if (!argv || !argc)
        return;

    args.resize(argc);

    for (uint32_t i = 0; i < argc; i++)
        if (argv[i])
            args[i] = argv[i];
}
#endif

void DArgs::FlushArgs()
{
    args.clear();
}

//
// CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1) or 0 if not present
//
size_t DArgs::CheckParm(const char *check) const
{
    if (!check)
        return 0;

    for (size_t i = 1, n = args.size(); i < n; i++)
        if (!stricmp(check, args[i].c_str()))
            return i;

    return 0;
}

const char *DArgs::CheckValue(const char *check) const
{
    if (!check)
        return 0;

    size_t i = CheckParm(check);

    if (i > 0 && i < args.size() - 1)
        return args[i + 1].c_str();
    else
        return NULL;
}

const char *DArgs::GetArg(size_t arg) const
{
    if (arg < args.size())
        return args[arg].c_str();
    else
        return NULL;
}

const std::vector<std::string> DArgs::GetArgList(size_t start) const
{
    std::vector<std::string> out;

    if (start < args.size())
    {
        out.resize(args.size() - start);
        std::copy(args.begin() + start, args.end(), out.begin());
    }

    return out;
}

size_t DArgs::NumArgs() const
{
    return args.size();
}

void DArgs::AppendArg(const char *arg)
{
    if (arg)
        args.push_back(arg);
}

//
// IsParam
//
// Helper function to return if the given argument number i is a parameter
//
static bool IsParam(const std::vector<std::string> &args, size_t i)
{
    return i < args.size() && (args[i][0] == '-' || args[i][0] == '+');
}

//
// FindNextParamArg
//
// Returns the next argument number for a command line parameter starting
// from argument number i.
//
static size_t FindNextParamArg(const char *param, const std::vector<std::string> &args, size_t i)
{
    while (i < args.size())
    {
        if (!IsParam(args, i))
            return i;

        // matches param, return first argument for this param
        if (stricmp(param, args[i].c_str()) == 0)
        {
            i++;
            continue;
        }

        // skip over any params that don't match and their arguments
        for (i++; i < args.size() && !IsParam(args, i); i++)
            ;
    }

    return args.size();
}

//
// DArgs::GatherFiles
//
// Collects all of the arguments entered after param.
//
// [AM] This used to be smarter but since we now properly deal with file
//      types higher up the stack we don't need to filter by extension
//      anymore.
//
DArgs DArgs::GatherFiles(const char *param) const
{
    DArgs out;

    if (param[0] != '-' && param[0] != '+')
        return out;

    for (size_t i = 1; i < args.size(); i++)
    {
        i = FindNextParamArg(param, args, i);
        if (i < args.size())
        {
            out.AppendArg(args[i].c_str());
        }
    }

    return out;
}

//
// bond - PROC M_FindResponseFile
//

static long ParseCommandLine(const char *args, int32_t *argc, char **argv);

void M_FindResponseFile(void)
{
    for (size_t i = 1; i < Args.NumArgs(); i++)
    {
        if (Args.GetArg(i)[0] == '@')
        {
            char **argv;
            int32_t    argc;
            int32_t    argcinresp;
            PHYSFS_File  *handle;
            int32_t    size;
            long   argsize;
            size_t index;

            std::string responsepath = Args.GetArg(i) + 1;
            std::string mountpath;
            M_ExtractFilePath(responsepath, mountpath);
            if (!mountpath.empty())
            {
                PHYSFS_mount(mountpath.c_str(), NULL, 0);
            }

            // READ THE RESPONSE FILE INTO MEMORY
            handle = PHYSFS_openRead(M_ExtractFileName(responsepath).c_str());
            if (!handle)
            { // [RH] Make this a warning, not an error.
                Printf(PRINT_WARNING, "No such response file (%s)!", Args.GetArg(i) + 1);
                continue;
            }

            Printf(PRINT_HIGH, "Found response file %s!\n", Args.GetArg(i) + 1);
            size = PHYSFS_fileLength(handle);
            Poco::Buffer<char> file(size + 1);
            size_t readlen = PHYSFS_readBytes(handle, file.begin(), size);
            if (readlen < size)
            {
                Printf(PRINT_HIGH, "Failed to read response file %s.\n", Args.GetArg(i) + 1);
            }
            file[size] = 0;
            PHYSFS_close(handle);

            argsize = ParseCommandLine(file.begin(), &argcinresp, NULL);
            argc    = argcinresp + Args.NumArgs() - 1;

            if (argc != 0)
            {
                Poco::Buffer<char *> argv(argc * sizeof(char *) + argsize);
                argv[i] = (char *)argv.begin() + argc * sizeof(char *);
                ParseCommandLine(file.begin(), NULL, argv.begin() + i);

                for (index = 0; index < i; ++index)
                    argv[index] = (char *)Args.GetArg(index);

                for (index = i + 1, i += argcinresp; index < Args.NumArgs(); ++index)
                    argv[i++] = (char *)Args.GetArg(index);

                DArgs newargs(i, argv.begin());
                Args = newargs;
            }

            // DISPLAY ARGS
            Printf("%zu command-line args:\n", Args.NumArgs());
            for (size_t k = 1; k < Args.NumArgs(); k++)
                Printf(PRINT_HIGH, "%s\n", Args.GetArg(k));

            break;
        }
    }
}

// ParseCommandLine
//
// bond - This is just like the version in c_dispatch.cpp, except it does not
// do cvar expansion.

static long ParseCommandLine(const char *args, int32_t *argc, char **argv)
{
    int32_t   count;
    char *buffplace;

    count     = 0;
    buffplace = NULL;
    if (argv != NULL)
    {
        buffplace = argv[0];
    }

    for (;;)
    {
        while (*args <= ' ' && *args)
        { // skip white space
            args++;
        }
        if (*args == 0)
        {
            break;
        }
        else if (*args == '\"')
        { // read quoted string
            char stuff;
            if (argv != NULL)
            {
                argv[count] = buffplace;
            }
            count++;
            args++;
            do
            {
                stuff = *args++;
                if (stuff == '\\' && *args == '\"')
                {
                    stuff = '\"', args++;
                }
                else if (stuff == '\"')
                {
                    stuff = 0;
                }
                else if (stuff == 0)
                {
                    args--;
                }
                if (argv != NULL)
                {
                    *buffplace = stuff;
                }
                buffplace++;
            } while (stuff);
        }
        else
        { // read unquoted string
            const char *start = args++, *end;

            while (*args && *args > ' ' && *args != '\"')
                args++;
            end = args;
            if (argv != NULL)
            {
                argv[count] = buffplace;
                while (start < end)
                    *buffplace++ = *start++;
                *buffplace++ = 0;
            }
            else
            {
                buffplace += end - start + 1;
            }
            count++;
        }
    }
    if (argc != NULL)
    {
        *argc = count;
    }
    return (long)(buffplace - (char *)0);
}

//
// M_GetParmValue
//
// Easy way of retrieving an integer parameter value.
//
int32_t M_GetParmValue(const char *name)
{
    const char *valuestr = Args.CheckValue(name);
    if (valuestr)
        return atoi(valuestr);
    return 0;
}

VERSION_CONTROL(m_argv_cpp, "$Id: 94efe5c9d92907e3d2c665e6e02f15eab90654dd $")
