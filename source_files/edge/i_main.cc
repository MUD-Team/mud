//----------------------------------------------------------------------------
//  EDGE Main
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

#include "dm_defs.h"
#include "e_main.h"
#include "epi_filesystem.h"
#include "epi_str_util.h"
#include "i_system.h"
#include "m_argv.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_time.h"
#include "version.h"
#include "physfs.h"

std::string executable_path = ".";

// SOKOL_FIXME
static int          s_argc = 0;
static const char **s_argv;

static void InitCallback()
{
    void EdgeInit(int argc, const char **argv);
    EdgeInit(s_argc, s_argv);
}

static void CleanupCallback()
{
    EdgeShutdown();
    SystemShutdown();
}

static void FrameCallback()
{
    void EdgeTick();
    EdgeTick();
}

static void EventCallback(const sapp_event *event)
{
    ControlPostEvent(*event);
}

sapp_desc sokol_main(int argc, char *argv[])
{
    stm_setup();

    if (!argc || !argv[0] || PHYSFS_init(argv[0]) == 0)
    {
        FatalError("Could not initialize PHYSFS:\n%d\n", PHYSFS_getLastErrorCode());
    }

    s_argc = argc;
    s_argv = (const char **) argv;

    executable_path = PHYSFS_getBaseDir();

#ifdef _WIN32
    // -AJA- change current dir to match executable
    if (!epi::CurrentDirectorySet(executable_path))
        FatalError("Couldn't set program directory to %s!!\n", executable_path.c_str());
#endif

    sapp_desc desc = {0};

    desc.init_cb              = InitCallback;
    desc.frame_cb             = FrameCallback;
    desc.cleanup_cb           = CleanupCallback;
    desc.event_cb             = EventCallback;
    desc.width                = 1360;
    desc.height               = 768;
    desc.high_dpi             = false;
    desc.logger.func          = slog_func;
    desc.window_title         = "MUD";    

    desc.win32_console_utf8   = true;
    desc.win32_console_create = true;

#ifdef __APPLE__
    // temporary hack while on GL 1.x
    desc.gl_major_version = 1;
    desc.gl_minor_version = 0;
#endif

    return desc;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
