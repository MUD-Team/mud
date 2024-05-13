// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 267b62faf29499aa138ecca58d833e7613365323 $
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
//
// DESCRIPTION:
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

// denis - todo - remove
#include "win32inc.h"
#ifdef _WIN32
#undef GetMessage
typedef BOOL(WINAPI *SetAffinityFunc)(HANDLE hProcess, DWORD mask);
#else
#include <sched.h>
#endif // WIN32
#ifdef UNIX
// for getuid and geteuid
#include <sys/types.h>
#include <unistd.h>
#endif

#include <iostream>
#include <new>
#include <stack>

#include "c_console.h"
#include "d_main.h"
#include "i_crash.h"
#include "i_sdl.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "mud_includes.h"
#include "z_zone.h"

// [Russell] - Don't need SDLmain library
#ifdef _WIN32
#undef main
#endif // WIN32

// Use main() on windows for msvc
#if defined(_MSC_VER)
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

EXTERN_CVAR(r_centerwindow)

DArgs Args;

// functions to be called at shutdown are stored in this stack
typedef void(STACK_ARGS *term_func_t)(void);
std::stack<std::pair<term_func_t, std::string>> TermFuncs;

void addterm(void(STACK_ARGS *func)(), const char *name)
{
    TermFuncs.push(std::pair<term_func_t, std::string>(func, name));
}

void STACK_ARGS call_terms(void)
{
    while (!TermFuncs.empty())
        TermFuncs.top().first(), TermFuncs.pop();
}

int32_t main(int32_t argc, char *argv[])
{
    // [AM] Set crash callbacks, so we get something useful from crashes.
#ifdef NDEBUG
    I_SetCrashCallbacks();
#endif

    try
    {

#if defined(UNIX)
        if (!getuid() || !geteuid())
            I_FatalError("root user detected, quitting odamex immediately");
#endif

        // [ML] 2007/9/3: From Eternity (originally chocolate Doom) Thanks SoM & fraggle!
        ::Args.SetArgs(argc, argv);

        if (PHYSFS_init(::Args.GetArg(0)) == 0)
            I_FatalError("Could not initialize PHYSFS:\n%d\n", PHYSFS_getLastErrorCode());

        PHYSFS_setWriteDir(M_GetWriteDir().c_str());
        // Ensure certain directories exist in the write folder
        // These should be no-ops if already present - Dasho
        PHYSFS_mkdir("assets");
        // Ensure downloads folder exists (I don't think we need to do this for core in the writeable folder)
        PHYSFS_mkdir("assets/downloads");
        PHYSFS_mkdir("saves");
        PHYSFS_mkdir("screenshots");
        PHYSFS_mkdir("soundfonts");

        PHYSFS_mount(M_GetBinaryDir().c_str(), NULL, 0);
        PHYSFS_mount(M_GetWriteDir().c_str(), NULL, 0);

        PHYSFS_mount(M_GetBinaryDir().append("assets").append(PATHSEP).append("core").c_str(), NULL, 0);
        PHYSFS_mount(M_GetBinaryDir().append("assets").append(PATHSEP).append("core").append(PATHSEP).append("common").c_str(), NULL, 0);
        PHYSFS_mount(M_GetBinaryDir().append("assets").append(PATHSEP).append("core").append(PATHSEP).append("client").c_str(), NULL, 0);
        PHYSFS_mount(M_GetWriteDir().append("assets").append(PATHSEP).append("downloads").c_str(), NULL, 0);

        // TODO: configurable with -game and root config json
        PHYSFS_mount(M_GetBinaryDir().append("assets").append(PATHSEP).append("example").c_str(), NULL, 0);

        if (::Args.CheckParm("--version"))
        {
#ifdef _WIN32
            PHYSFS_File *fh = PHYSFS_openWrite("mud-version.txt");
            if (!fh)
                exit(EXIT_FAILURE);

            std::string str;
            StrFormat(str, "MUD %s\n", NiceVersion());
            const int32_t ok = PHYSFS_writeBytes(fh, str.data(), str.size());
            if (ok != str.size())
            {
                PHYSFS_deinit();
                exit(EXIT_FAILURE);
            }

            PHYSFS_close(fh);
            PHYSFS_deinit();
#else
            printf("MUD Client %s\n", NiceVersion());
#endif
            exit(EXIT_SUCCESS);
        }

        const char *crashdir = ::Args.CheckValue("-crashdir");
        if (crashdir)
        {
            I_SetCrashDir(crashdir);
        }
        else
        {
            std::string writedir = M_GetWriteDir();
            I_SetCrashDir(writedir.c_str());
        }

        const char *CON_FILE = ::Args.CheckValue("-confile");
        if (CON_FILE)
        {
            CON.open(CON_FILE, std::ios::in);
        }

        // denis - if argv[1] starts with "mud://"
        if (argc == 2 && argv && argv[1])
        {
            const char *protocol = "mud://";
            const char *uri      = argv[1];

            if (strncmp(uri, protocol, strlen(protocol)) == 0)
            {
                std::string location = uri + strlen(protocol);
                size_t      term     = location.find_first_of('/');

                if (term == std::string::npos)
                    term = location.length();

                Args.AppendArg("-connect");
                Args.AppendArg(location.substr(0, term).c_str());
            }
        }

        uint32_t sdl_flags = SDL_INIT_TIMER;

#ifdef _MSC_VER
        // [SL] per the SDL documentation, SDL's parachute, used to cleanup
        // after a crash, causes the MSVC debugger to be unusable
        sdl_flags |= SDL_INIT_NOPARACHUTE;
#endif

        if (SDL_Init(sdl_flags) == -1)
            I_FatalError("Could not initialize SDL:\n%s\n", SDL_GetError());

        atterm(SDL_Quit);

        /*
        killough 1/98:

          This fixes some problems with exit handling
          during abnormal situations.

            The old code called I_Quit() to end program,
            while now I_Quit() is installed as an exit
            handler and exit() is called to exit, either
            normally or abnormally.
        */

        // But avoid calling this on windows!
        // Good on some platforms, useless on others
        //		#ifndef _WIN32
        //		atexit (call_terms);
        //		#endif

        atterm(I_Quit);
        atterm(DObject::StaticShutdown);

        D_DoomMain(); // Usually does not return

        // If D_DoomMain does return (as is the case with the +demotest parameter)
        // proper termination needs to occur -- Hyper_Eye
        call_terms();
        PHYSFS_deinit();
    }
    catch (CDoomError &error)
    {
        if (LOG.is_open())
        {
            LOG << "=== ERROR: " << error.GetMsg() << " ===\n\n";
        }

        I_ErrorMessageBox(error.GetMsg().c_str());

        call_terms();
        PHYSFS_deinit();
        exit(EXIT_FAILURE);
    }
#ifndef _DEBUG
    catch (...)
    {
        // If an exception is thrown, be sure to do a proper shutdown.
        // This is especially important if we are in fullscreen mode,
        // because the OS will only show the alert box if we are in
        // windowed mode. Graphics gets shut down first in case something
        // goes wrong calling the cleanup functions.
        call_terms();
        // Now let somebody who understands the exception deal with it.
        throw;
    }
#endif
    return 0;
}

VERSION_CONTROL(i_main_cpp, "$Id: 267b62faf29499aa138ecca58d833e7613365323 $")
