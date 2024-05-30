// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 403580d5cf355aba57962141deccd21f0c74ecce $
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
//		Functions to draw patches (by post) directly to screen->
//		Functions to blit a block to the screen->
//
//-----------------------------------------------------------------------------

#include "v_video.h"

#include <assert.h>

#include "c_console.h"
#include "c_dispatch.h"
#include "cmdlib.h"
#include "d_main.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "mud_includes.h"
#include "r_draw.h"
#include "r_local.h"
#include "r_state.h"
#include "w_wad.h"
#include "z_zone.h"

argb_t     Col2RGB8[65][256];
palindex_t RGB32k[32][32][32];

// [RH] The framebuffer is no longer a mere byte array.
// There's also only one, not four.
DCanvas *screen;

static DBoundingBox dirtybox;

static bool V_DoSetResolution();
static void BuildTransTable(const argb_t *palette_colors);

// flag to indicate that V_AdjustVideoMode should try to change the video mode
static bool setmodeneeded = false;

//
//	V_ForceVideoModeAdjustment
//
// Tells the video subsystem to change the video mode and recalculate any
// lookup tables dependent on the video mode prior to drawing the next frame.
//
void V_ForceVideoModeAdjustment()
{
    setmodeneeded = true;
}

//
// V_AdjustVideoMode
//
// Checks if the video mode needs to be changed and calls several video mode
// dependent initialization routines if it does. This should be called at the
// start of drawing a video frame.
//
void V_AdjustVideoMode()
{
    if (setmodeneeded)
    {
        setmodeneeded = false;

        // Change screen mode.
        if (!V_DoSetResolution())
            I_Error("Could not change screen mode");

        // Recalculate various view parameters.
        R_ForceViewWindowResize();

        void UI_OnResize();
        UI_OnResize();
    }
}

EXTERN_CVAR(vid_defwidth)
EXTERN_CVAR(vid_defheight)
EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_filter)
EXTERN_CVAR(vid_widescreen)
EXTERN_CVAR(sv_allowwidescreen)
EXTERN_CVAR(vid_vsync)
EXTERN_CVAR(vid_displayfps)

static int32_t vid_widescreen_old = -1;

static IVideoMode V_GetRequestedVideoMode()
{
    int32_t               surface_bpp = 32;
    EWindowMode       window_mode = (EWindowMode)vid_fullscreen.asInt();
    bool              vsync       = (vid_vsync != 0.0f);
    const std::string stretch_mode(vid_filter);

    return IVideoMode(vid_defwidth.asInt(), vid_defheight.asInt(), surface_bpp, window_mode, vsync, stretch_mode);
}

bool V_CheckModeAdjustment()
{
    const IWindow *window = I_GetWindow();
    if (!window)
        return false;

    if (V_GetRequestedVideoMode() != window->getVideoMode())
        return true;

    bool using_widescreen = I_IsWideResolution();
    if (vid_widescreen.asInt() > 0 && sv_allowwidescreen != using_widescreen)
        return true;

    if (vid_widescreen.asInt() != vid_widescreen_old)
    {
        vid_widescreen_old = vid_widescreen.asInt();
        return true;
    }

    return false;
}

CVAR_FUNC_IMPL(vid_defwidth)
{
    if (var < 320 || var > MAXWIDTH)
        var.RestoreDefault();

    if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
        V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL(vid_defheight)
{
    if (var < 200 || var > MAXHEIGHT)
        var.RestoreDefault();

    if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
        V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL(vid_fullscreen)
{
    if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
        V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL(vid_filter)
{
    if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
        V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL(vid_vsync)
{
    if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
        V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL(vid_overscan)
{
    if (gamestate != GS_STARTUP)
        V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL(vid_widescreen)
{
    if (var < 0 || var > 5)
        var.RestoreDefault();

    if (gamestate != GS_STARTUP && V_CheckModeAdjustment())
        V_ForceVideoModeAdjustment();
}

//
// Only checks to see if the widescreen mode is proper compared to sv_allowwidescreen.
//
// Doing a full check on Windows results in strange flashing behavior in fullscreen
// because there's an 32-bit surface being rendered to as 8-bit.
//
static bool CheckWideModeAdjustment()
{
    bool using_widescreen = I_IsWideResolution();
    if (vid_widescreen.asInt() > 0 && sv_allowwidescreen != using_widescreen)
        return true;

    if (vid_widescreen.asInt() > 0 != using_widescreen)
        return true;

    return false;
}

CVAR_FUNC_IMPL(sv_allowwidescreen)
{
    if (!I_VideoInitialized() || gamestate == GS_STARTUP)
        return;

    if (!CheckWideModeAdjustment())
        return;

    V_ForceVideoModeAdjustment();
}

CVAR_FUNC_IMPL(vid_maxfps)
{
    if (var == 0)
    {
        capfps = false;
        maxfps = 99999.0f;
    }
    else
    {
        if (var < 35.0f)
            var.Set(35.0f);
        else
        {
            capfps = true;
            maxfps = var;
        }
    }
}

//
// vid_listmodes
//
// Prints a list of all supported video modes, highlighting the current
// video mode. Requires I_VideoInitialized() to be true.
//
BEGIN_COMMAND(vid_listmodes)
{
    const IVideoModeList *modelist = I_GetVideoCapabilities()->getSupportedVideoModes();

    for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
    {
        if (*it == I_GetWindow()->getVideoMode())
            Printf_Bold("%s\n", I_GetVideoModeString(*it).c_str());
        else
            Printf(PRINT_HIGH, "%s\n", I_GetVideoModeString(*it).c_str());
    }
}
END_COMMAND(vid_listmodes)

//
// vid_currentmode
//
// Prints the current video mode. Requires I_VideoInitialized() to be true.
//
BEGIN_COMMAND(vid_currentmode)
{
    std::string pixel_string;

    const PixelFormat *format = IRenderSurface::getCurrentRenderSurface()->getPixelFormat();
    char    temp_str[9] = {0};
    argb_t *d1          = (argb_t *)temp_str;
    argb_t *d2          = (argb_t *)temp_str + 1;

    d1->seta('A');
    d1->setr('R');
    d1->setg('G');
    d1->setb('B');
    d2->seta('0' + format->getABits());
    d2->setr('0' + format->getRBits());
    d2->setg('0' + format->getGBits());
    d2->setb('0' + format->getBBits());

    pixel_string = std::string(temp_str);

    const IVideoMode &mode = I_GetWindow()->getVideoMode();
    Printf(PRINT_HIGH, "%s %s surface\n", I_GetVideoModeString(mode).c_str(), pixel_string.c_str());
}
END_COMMAND(vid_currentmode)

BEGIN_COMMAND(checkres)
{
    Printf(PRINT_HIGH, "%dx%d\n", I_GetVideoWidth(), I_GetVideoHeight());
}
END_COMMAND(checkres)

//
// vid_setmode
//
// Sets the video mode resolution.
// The actual change of resolution will take place at the start of the next frame.
//
BEGIN_COMMAND(vid_setmode)
{
    int32_t width = 0, height = 0;

    // No arguments
    if (argc == 1)
    {
        Printf(PRINT_HIGH, "Usage: vid_setmode <width> <height>\n");
        return;
    }

    // Width
    if (argc > 1)
        width = atoi(argv[1]);

    // Height (optional)
    if (argc > 2)
        height = atoi(argv[2]);
    if (height == 0)
        height = vid_defheight;

    if (width < 320 || height < 200)
    {
        Printf(PRINT_WARNING, "%dx%d is too small.  Minimum resolution is 320x200.\n", width, height);
        return;
    }

    if (width > MAXWIDTH || height > MAXHEIGHT)
    {
        Printf(PRINT_WARNING, "%dx%d is too large.  Maximum resolution is %dx%d.\n", width, height, MAXWIDTH,
               MAXHEIGHT);
        return;
    }

    vid_defwidth.Set(width);
    vid_defheight.Set(height);
}
END_COMMAND(vid_setmode)

//
// V_UseWidescreen
//
//
bool V_UseWidescreen()
{
    int32_t width = I_GetVideoWidth(), height = I_GetVideoHeight();

    if (width == 0 || height == 0)
        return false;

    return (vid_widescreen.asInt() > 0 && (serverside || sv_allowwidescreen));
}

//
// V_DoSetResolution
//
// Changes the video resolution to the given dimensions. This should only
// be called at the begining of drawing a frame (or during startup)!
//
static bool V_DoSetResolution()
{
    const IVideoMode requested_mode = V_GetRequestedVideoMode();

    I_SetVideoMode(requested_mode);
    if (!I_VideoInitialized())
        return false;

    V_Init();

    return true;
}

//
// V_Close
//
void STACK_ARGS V_Close()
{

}

//
// V_Init
//
void V_Init()
{
    if (!I_VideoInitialized())
    {
        int32_t video_width  = M_GetParmValue("-width");
        int32_t video_height = M_GetParmValue("-height");
        int32_t video_bpp    = M_GetParmValue("-bits");

        // ensure the width & height cvars are sane
        if (vid_defwidth.asInt() <= 0 || vid_defheight.asInt() <= 0)
        {
            vid_defwidth.RestoreDefault();
            vid_defheight.RestoreDefault();
        }

        if (video_width == 0 && video_height == 0)
        {
            video_width  = vid_defwidth.asInt();
            video_height = vid_defheight.asInt();
        }
        else if (video_width == 0)
        {
            video_width = video_height * 4 / 3;
        }
        else if (video_height == 0)
        {
            video_height = video_width * 3 / 4;
        }

        vid_defwidth.Set(video_width);
        vid_defheight.Set(video_height);

        video_bpp = 32;
        V_DoSetResolution();

        Printf(PRINT_HIGH, "V_Init: using %s video driver.\n", I_GetVideoDriverName().c_str());
    }

    if (!I_VideoInitialized())
        I_Error("Failed to initialize display");

    V_InitPalette();

    R_InitColumnDrawers();

    I_SetWindowCaption(D_GetTitleString());
    I_SetWindowIcon();

    BuildTransTable(V_GetDefaultPalette()->basecolors);

    vid_widescreen_old = vid_widescreen.asInt();
}

//
// V_MarkRect
//
void V_MarkRect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    dirtybox.AddToBox(x, y);
    dirtybox.AddToBox(x + width - 1, y + height - 1);
}

const int32_t    GRAPH_WIDTH           = 140;
const int32_t    GRAPH_HEIGHT          = 80;
const double GRAPH_BASELINE        = 1000 / 60.0;
const double GRAPH_CAPPED_BASELINE = 1000 / 35.0;

struct frametimeGraph_t
{
    double data[256];
    size_t tail; // Next insert location.
    double minimum;
    double maximum;

    frametimeGraph_t() : tail(0), minimum(::GRAPH_BASELINE), maximum(::GRAPH_BASELINE)
    {
        ArrayInit(data, ::GRAPH_BASELINE);
    }

    void clear()
    {
        ArrayInit(data, ::GRAPH_BASELINE);
        tail    = 0;
        minimum = ::GRAPH_BASELINE;
        maximum = ::GRAPH_BASELINE;
    }

    void refit()
    {
        double newmin = data[0];
        double newmax = data[0];

        for (size_t i = 0; i < ARRAY_LENGTH(data); i++)
        {
            if (data[i] < newmin)
                newmin = data[i];
            if (data[i] > newmax)
                newmax = data[i];
        }

        // Round to nearest power of two.
        if (newmin <= 0.0)
        {
            minimum = 0.0;
        }
        else
        {
            double low = ::GRAPH_BASELINE;
            while (low > newmin)
                low /= 2;
            minimum = low;
        }

        if (newmax >= 1000.0)
        {
            newmax = 1000.0;
        }
        else
        {
            double hi = ::GRAPH_BASELINE;
            while (hi < newmax)
                hi *= 2;
            maximum = hi;
        }
    }

    void push(const double val)
    {
        if (val < minimum)
            minimum = val;
        if (val > maximum)
            maximum = val;

        data[tail] = val;
        tail       = (tail + 1) & 0xFF;
    }

    double getTail(const size_t i)
    {
        size_t idx = (tail - 1 - i) & 0xFF;
        return data[idx];
    }

    double normalize(const double n)
    {
        return (n - minimum) / (maximum - minimum);
    }
} g_GraphData;

// Build the tables necessary for translucency
static void BuildTransTable(const argb_t *palette_colors)
{
    // create the small RGB table
    for (int32_t r = 0; r < 32; r++)
        for (int32_t g = 0; g < 32; g++)
            for (int32_t b = 0; b < 32; b++)
                RGB32k[r][g][b] =
                    V_BestColor(palette_colors, (r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2));

    for (int32_t x = 0; x < 65; x++)
        for (int32_t y = 0; y < 256; y++)
            Col2RGB8[x][y] = (((palette_colors[y].getr() * x) >> 4) << 20) | ((palette_colors[y].getg() * x) >> 4) |
                             (((palette_colors[y].getb() * x) >> 4) << 10);
}

VERSION_CONTROL(v_video_cpp, "$Id: 403580d5cf355aba57962141deccd21f0c74ecce $")
