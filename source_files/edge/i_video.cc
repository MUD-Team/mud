//----------------------------------------------------------------------------
//  EDGE SDL Video Code
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

#include "i_video.h"

#include "edge_profiling.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "i_defs_gl.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"
#include "r_modes.h"
#include "version.h"

int graphics_shutdown = 0;

EDGE_DEFINE_CONSOLE_VARIABLE(vsync, "0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE_CLAMPED(gamma_correction, "0", kConsoleVariableFlagArchive, -1.0, 1.0)

// this is the Monitor Size setting, really an aspect ratio.
// it defaults to 16:9, as that is the most common monitor size nowadays.
EDGE_DEFINE_CONSOLE_VARIABLE(monitor_aspect_ratio, "1.77777", kConsoleVariableFlagArchive)

// these are zero until StartupGraphics is called.
// after that they never change (we assume the desktop won't become other
// resolutions while EC is running).
EDGE_DEFINE_CONSOLE_VARIABLE(desktop_resolution_width, "0", kConsoleVariableFlagReadOnly)
EDGE_DEFINE_CONSOLE_VARIABLE(desktop_resolution_height, "0", kConsoleVariableFlagReadOnly)

EDGE_DEFINE_CONSOLE_VARIABLE(pixel_aspect_ratio, "1.0", kConsoleVariableFlagReadOnly);

// when > 0, this will force the pixel_aspect to a particular value, for
// cases where a normal logic fails.  however, it will apply to *all* modes,
// including windowed mode.
EDGE_DEFINE_CONSOLE_VARIABLE(forced_pixel_aspect_ratio, "0", kConsoleVariableFlagArchive)

extern ConsoleVariable renderer_far_clip;

void GrabCursor(bool enable)
{
    if (graphics_shutdown)
        return;    

    if (sapp_mouse_locked() == enable)
    {
        return;
    }

    sapp_lock_mouse(enable);
}

void DeterminePixelAspect()
{
    // the pixel aspect is the shape of pixels on the monitor for the current
    // video mode.  on modern LCDs (etc) it is usuall 1.0 (1:1).  knowing this
    // is critical to get things drawn correctly.  for example, Doom assets
    // assumed a 320x200 resolution on a 4:3 monitor, a pixel aspect of 5:6 or
    // 0.833333, and we must adjust image drawing to get "correct" results.

    // allow user to override
    if (forced_pixel_aspect_ratio.f_ > 0.1)
    {
        pixel_aspect_ratio = forced_pixel_aspect_ratio.f_;
        return;
    }

    // if not a fullscreen mode, check for a modern LCD (etc) monitor -- they
    // will have square pixels (1:1 aspect).
    bool is_crt = (desktop_resolution_width.d_ < desktop_resolution_height.d_ * 7 / 5);

    bool is_fullscreen = (current_window_mode > 0);
    if (is_fullscreen && current_screen_width == desktop_resolution_width.d_ &&
        current_screen_height == desktop_resolution_height.d_ && graphics_shutdown)
        is_fullscreen = false;

    if (!is_fullscreen && !is_crt)
    {
        pixel_aspect_ratio = 1.0f;
        return;
    }

    // in fullscreen modes, or a CRT monitor, compute the pixel aspect from
    // the current resolution and Monitor Size setting.  this assumes that the
    // video mode is filling the whole monitor (i.e. the monitor is not doing
    // any letter-boxing or pillar-boxing).  DPI setting does not matter here.

    pixel_aspect_ratio = monitor_aspect_ratio.f_ * (float)current_screen_height / (float)current_screen_width;
}

void StartupGraphics(void)
{
    // -DS- 2005/06/27 Detect SDL Resolutions
    // SDL_DisplayMode info;
    // SDL_GetDesktopDisplayMode(0, &info);

    // SOKOL_FIX
    desktop_resolution_width  = 1360;
    desktop_resolution_height = 768;

    if (current_screen_width > desktop_resolution_width.d_)
        current_screen_width = desktop_resolution_width.d_;
    if (current_screen_height > desktop_resolution_height.d_)
        current_screen_height = desktop_resolution_height.d_;

    LogPrint("Desktop resolution: %dx%d\n", desktop_resolution_width.d_, desktop_resolution_height.d_);

    // sokol fix
    /*
    int num_modes = SDL_GetNumDisplayModes(0);

    for (int i = 0; i < num_modes; i++)
    {
        SDL_DisplayMode possible_mode;
        SDL_GetDisplayMode(0, i, &possible_mode);

        if (possible_mode.w > desktop_resolution_width.d_ || possible_mode.h > desktop_resolution_height.d_)
            continue;

        DisplayMode test_mode;

        test_mode.width       = possible_mode.w;
        test_mode.height      = possible_mode.h;
        test_mode.depth       = SDL_BITSPERPIXEL(possible_mode.format);
        test_mode.window_mode = kWindowModeFullscreen;

        if ((test_mode.width & 15) != 0)
            continue;

        if (test_mode.depth == 15 || test_mode.depth == 16 || test_mode.depth == 24 || test_mode.depth == 32)
        {
            AddDisplayResolution(&test_mode);

            if (test_mode.width < desktop_resolution_width.d_ && test_mode.height < desktop_resolution_height.d_)
            {
                DisplayMode win_mode = test_mode;
                win_mode.window_mode = kWindowModeWindowed;
                AddDisplayResolution(&win_mode);
            }
        }
    }
    */

    DisplayMode win_mode;
    win_mode.depth       = 32;
    win_mode.height      = 768;
    win_mode.width       = 1360;
    win_mode.window_mode = kWindowModeWindowed;
    AddDisplayResolution(&win_mode);

    // If needed, set the default window toggle mode to the largest non-native
    // res
    if (toggle_windowed_window_mode.d_ == kWindowModeInvalid)
    {
        for (size_t i = 0; i < screen_modes.size(); i++)
        {
            DisplayMode *check = screen_modes[i];
            if (check->window_mode == kWindowModeWindowed)
            {
                toggle_windowed_window_mode = kWindowModeWindowed;
                toggle_windowed_height      = check->height;
                toggle_windowed_width       = check->width;
                toggle_windowed_depth       = check->depth;
                break;
            }
        }
    }

    // Fill in borderless mode scrmode with the native display info
    borderless_mode.window_mode = kWindowModeBorderless;
    borderless_mode.width       = 1360;
    borderless_mode.height      = 768;
    borderless_mode.depth       = 24;

    // If needed, also make the default fullscreen toggle mode borderless
    if (toggle_fullscreen_window_mode.d_ == kWindowModeInvalid)
    {
        toggle_fullscreen_window_mode = kWindowModeBorderless;
        toggle_fullscreen_width       = 1360;
        toggle_fullscreen_height      = 768;
        toggle_fullscreen_depth       = 24;
    }

    LogPrint("StartupGraphics: initialisation OK\n");
}

static bool window_initialized = false;

static bool InitializeWindow(DisplayMode *mode)
{
    window_initialized     = true;
    std::string temp_title = game_name.s_;
    temp_title.append(" ").append(edge_version.s_);

    /*
    int resizeable = 0;
    program_window =
        SDL_CreateWindow(temp_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mode->width, mode->height,
                         SDL_WINDOW_OPENGL |
                             (mode->window_mode == kWindowModeBorderless
                                  ? (SDL_WINDOW_FULLSCREEN_DESKTOP)
                                  : (mode->window_mode == kWindowModeFullscreen ? SDL_WINDOW_FULLSCREEN : 0)) |
                             resizeable);

    if (program_window == nullptr)
    {
        LogPrint("Failed to create window: %s\n", SDL_GetError());
        return false;
    }

    if (mode->window_mode == kWindowModeBorderless)
        SDL_GetWindowSize(program_window, &borderless_mode.width, &borderless_mode.height);
    */

    if (mode->window_mode == kWindowModeWindowed)
    {
        toggle_windowed_depth       = mode->depth;
        toggle_windowed_height      = mode->height;
        toggle_windowed_width       = mode->width;
        toggle_windowed_window_mode = kWindowModeWindowed;
    }
    else if (mode->window_mode == kWindowModeFullscreen)
    {
        toggle_fullscreen_depth       = mode->depth;
        toggle_fullscreen_height      = mode->height;
        toggle_fullscreen_width       = mode->width;
        toggle_fullscreen_window_mode = kWindowModeFullscreen;
    }
    else
    {
        toggle_fullscreen_depth       = borderless_mode.depth;
        toggle_fullscreen_height      = borderless_mode.height;
        toggle_fullscreen_width       = borderless_mode.width;
        toggle_fullscreen_window_mode = kWindowModeBorderless;
    }

    /*
    if (SDL_GL_CreateContext(program_window) == nullptr)
        FatalError("Failed to create OpenGL context.\n");

    if (vsync.d_ == 2)
    {
        // Fallback to normal VSync if Adaptive doesn't work
        if (SDL_GL_SetSwapInterval(-1) == -1)
        {
            vsync = 1;
            SDL_GL_SetSwapInterval(vsync.d_);
        }
    }
    else
        SDL_GL_SetSwapInterval(vsync.d_);
    */

    gladLoaderLoadGL();

    return true;
}

bool SetScreenSize(DisplayMode *mode)
{
    LogPrint("SetScreenSize: trying %dx%d %dbpp (%s)\n", mode->width, mode->height, mode->depth,
             mode->window_mode == kWindowModeBorderless
                 ? "borderless"
                 : (mode->window_mode == kWindowModeFullscreen ? "fullscreen" : "windowed"));

    if (!window_initialized)
    {
        if (!InitializeWindow(mode))
        {
            return false;
        }
    }
#ifdef SOKOL_DISABLED
    else if (mode->window_mode == kWindowModeBorderless)
    {
        SDL_SetWindowFullscreen(program_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_GetWindowSize(program_window, &borderless_mode.width, &borderless_mode.height);

        LogPrint("SetScreenSize: mode now %dx%d %dbpp\n", mode->width, mode->height, mode->depth);
    }
    else if (mode->window_mode == kWindowModeFullscreen)
    {
        SDL_SetWindowFullscreen(program_window, SDL_WINDOW_FULLSCREEN);
        SDL_DisplayMode *new_mode = new SDL_DisplayMode;
        new_mode->h               = mode->height;
        new_mode->w               = mode->width;
        SDL_SetWindowDisplayMode(program_window, new_mode);
        SDL_SetWindowSize(program_window, mode->width, mode->height);
        delete new_mode;
        new_mode = nullptr;

        LogPrint("SetScreenSize: mode now %dx%d %dbpp\n", mode->width, mode->height, mode->depth);
    }
    else /* kWindowModeWindowed */
    {
        SDL_SetWindowFullscreen(program_window, 0);
        SDL_SetWindowSize(program_window, mode->width, mode->height);
        SDL_SetWindowPosition(program_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        LogPrint("SetScreenSize: mode now %dx%d %dbpp\n", mode->width, mode->height, mode->depth);
    }
#endif

    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // SDL_GL_SwapWindow(program_window);

    return true;
}

void StartFrame(void)
{
    ec_frame_stats.Clear();
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderer_far_clip.f_ = 64000.0;
}

void FinishFrame(void)
{
    // SDL_GL_SwapWindow(program_window);

    EDGE_TracyPlot("draw_render_units", (int64_t)ec_frame_stats.draw_render_units);
    EDGE_TracyPlot("draw_wall_parts", (int64_t)ec_frame_stats.draw_wall_parts);
    EDGE_TracyPlot("draw_planes", (int64_t)ec_frame_stats.draw_planes);
    EDGE_TracyPlot("draw_things", (int64_t)ec_frame_stats.draw_things);
    EDGE_TracyPlot("draw_light_iterator", (int64_t)ec_frame_stats.draw_light_iterator);
    EDGE_TracyPlot("draw_sector_glow_iterator", (int64_t)ec_frame_stats.draw_sector_glow_iterator);

    EDGE_FrameMark;

    if (vsync.CheckModified())
    {
        if (vsync.d_ == 2)
        {
            // Fallback to normal VSync if Adaptive doesn't work
            // if (SDL_GL_SetSwapInterval(-1) == -1)
            //{
            //    vsync = 1;
            //    SDL_GL_SetSwapInterval(vsync.d_);
            // }
        }
        // else
        //     SDL_GL_SetSwapInterval(vsync.d_);
    }

    if (monitor_aspect_ratio.CheckModified() || forced_pixel_aspect_ratio.CheckModified())
        DeterminePixelAspect();
}

void ShutdownGraphics(void)
{
    if (graphics_shutdown)
        return;

    graphics_shutdown = 1;

    gladLoaderUnloadGL();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
