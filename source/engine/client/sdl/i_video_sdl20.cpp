// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 701bbcdb6e1a40f728d014baddb3d8e4677e368a $
//
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
//	SDL 2.0 implementation of the IVideo class.
//
//-----------------------------------------------------------------------------

#include "i_video_sdl20.h"

#include <stdlib.h>
// [Russell] - Just for windows, display the icon in the system menu and alt-tab display
#include "win32inc.h"
#if defined(_WIN32)
#include <SDL_syswm.h>

#include "resource.h"
#endif // WIN32

#include <algorithm>
#include <cassert>
#include <functional>

#include "../ui/ui_public.h"
#include "c_dispatch.h"
#include "i_icon.h"
#include "i_input.h"
#include "i_sdl.h"
#include "i_system.h"
#include "i_video.h"
#include "mud_includes.h"
#include "mud_profiling.h"
#include "ui_public.h"
#include "v_palette.h"
#include "v_video.h"

EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_widescreen)

#ifdef SDL20

// ============================================================================
//
// ISDL20VideoCapabilities class implementation
//
// ============================================================================

//
// I_AddSDL20VideoModes
//
// Queries SDL to find the supported video modes at the given bit depth
// and then adds them to modelist.
//
static void I_AddSDL20VideoModes(IVideoModeList *modelist, int32_t bpp)
{
    int32_t         display_index = 0;
    SDL_DisplayMode mode          = {SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0};

    int32_t display_mode_count = SDL_GetNumDisplayModes(display_index);
    if (display_mode_count < 1)
    {
        I_Error("SDL_GetNumDisplayModes failed: %s", SDL_GetError());
        return;
    }

    for (int32_t i = 0; i < display_mode_count; i++)
    {
        if (SDL_GetDisplayMode(display_index, i, &mode) != 0)
        {
            I_Error("SDL_GetDisplayMode failed: %s", SDL_GetError());
            return;
        }

        int32_t width = mode.w, height = mode.h;
        // add this video mode to the list (both fullscreen & windowed)
        modelist->push_back(IVideoMode(width, height, bpp, WINDOW_Windowed));
        modelist->push_back(IVideoMode(width, height, bpp, WINDOW_DesktopFullscreen));
        modelist->push_back(IVideoMode(width, height, bpp, WINDOW_Fullscreen));
    }
}

//
// ISDL20VideoCapabilities::ISDL20VideoCapabilities
//
// Discovers the native desktop resolution and queries SDL for a list of
// supported fullscreen video modes.
//
// NOTE: discovering the native desktop resolution only works if this is called
// prior to the first SDL_SetVideoMode call!
//
// NOTE: SDL has palette issues in 8bpp fullscreen mode on Windows platforms. As
// a workaround, we force a 32bpp resolution in this case by removing all
// fullscreen 8bpp video modes on Windows platforms.
//
ISDL20VideoCapabilities::ISDL20VideoCapabilities() : IVideoCapabilities(), mNativeMode(0, 0, 0, WINDOW_Windowed)
{
    const int32_t display_index = 0;
    int32_t       bpp;
    Uint32        Rmask, Gmask, Bmask, Amask;

    SDL_DisplayMode sdl_display_mode;
    if (SDL_GetDesktopDisplayMode(display_index, &sdl_display_mode) != 0)
    {
        I_Error("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        return;
    }

    // SDL_BITSPERPIXEL appears to not translate the mode properly, sometimes it reports
    // a 24bpp mode when it is actually 32bpp, this function clears that up
    SDL_PixelFormatEnumToMasks(sdl_display_mode.format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);

    mNativeMode = IVideoMode(sdl_display_mode.w, sdl_display_mode.h, bpp, WINDOW_Fullscreen);

    I_AddSDL20VideoModes(&mModeList, 8);
    I_AddSDL20VideoModes(&mModeList, 32);

    // always add the following windowed modes (if windowed modes are supported)
    if (supportsWindowed())
    {
        if (supports32bpp())
        {
            mModeList.push_back(IVideoMode(320, 200, 32, WINDOW_Windowed));
            mModeList.push_back(IVideoMode(320, 240, 32, WINDOW_Windowed));
            mModeList.push_back(IVideoMode(640, 400, 32, WINDOW_Windowed));
            mModeList.push_back(IVideoMode(640, 480, 32, WINDOW_Windowed));
        }
    }

    // reverse sort the modes
    std::sort(mModeList.begin(), mModeList.end(), std::greater<IVideoMode>());

    // get rid of any duplicates (SDL sometimes reports duplicates)
    mModeList.erase(std::unique(mModeList.begin(), mModeList.end()), mModeList.end());

    assert(supportsWindowed() || supportsFullScreen());
    assert(supports32bpp());
}

// ============================================================================
//
// ISDL20Window class implementation
//
// ============================================================================

//
// ISDL20Window::ISDL20Window (if windowed modes are supported)
//
// Constructs a new application window using SDL 1.2.
// A ISDL20WindowSurface object is instantiated for frame rendering.
//
ISDL20Window::ISDL20Window(uint16_t width, uint16_t height, uint8_t bpp, EWindowMode window_mode, bool vsync)
    : IWindow(), mSDLWindow(NULL), mVideoMode(0, 0, 0, WINDOW_Windowed), mNeedPaletteRefresh(true), mBlit(true),
      mMouseFocus(false), mKeyboardFocus(false), mAcceptResizeEventsTime(0)
{
    setRendererDriver();
    const char *driver_name = getRendererDriver();
    Printf(PRINT_HIGH, "V_Init: rendering mode \"%s\"\n", driver_name);

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    uint32_t window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

    if (strncmp(driver_name, "open", 4) == 0)
        window_flags |= SDL_WINDOW_OPENGL;

    if (window_mode == WINDOW_Fullscreen)
        window_flags |= SDL_WINDOW_FULLSCREEN;
    else if (window_mode == WINDOW_DesktopFullscreen)
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    mSDLWindow = SDL_CreateWindow("", // Empty title for now - it will be set later
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);

    if (mSDLWindow == NULL)
        I_Error("I_InitVideo: unable to create window: %s\n", SDL_GetError());

    SDL_SetWindowMinimumSize(mSDLWindow, 320, 200);

    discoverNativePixelFormat();

    mMouseFocus = mKeyboardFocus = true;
}

//
// ISDL20Window::~ISDL20Window
//
ISDL20Window::~ISDL20Window()
{
    if (mSDLWindow)
        SDL_DestroyWindow(mSDLWindow);
}

//
// ISDL20Window::setRendererDriver
//
void ISDL20Window::setRendererDriver()
{
    // Preferred ordering of drivers
    const char *drivers[] = {"direct3d", "opengl", "opengles2", "opengles", "software", ""};

    for (int32_t i = 0; drivers[i][0] != '\0'; i++)
    {
        const char *driver = drivers[i];
        if (isRendererDriverAvailable(driver))
        {
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, driver);
            return;
        }
    }
}

//
// ISDL20Window::isRendererDriverAvailable
//
bool ISDL20Window::isRendererDriverAvailable(const char *driver) const
{
    SDL_RendererInfo info;
    int32_t          num_drivers = SDL_GetNumRenderDrivers();

    for (int32_t i = 0; i < num_drivers; i++)
    {
        SDL_GetRenderDriverInfo(i, &info);
        if (strncmp(info.name, driver, strlen(driver)) == 0)
            return true;
    }
    return false;
}

//
// ISDL20Window::getRendererDriver
//
const char *ISDL20Window::getRendererDriver() const
{
    static char driver_name[20];
    memset(driver_name, 0, sizeof(driver_name));
    const char *hint_value = SDL_GetHint(SDL_HINT_RENDER_DRIVER);
    if (hint_value)
        strncpy(driver_name, hint_value, sizeof(driver_name) - 1);
    return driver_name;
}

//
// ISDL20Window::getEvents
//
// Retrieves events for the application window and processes them.
//
void ISDL20Window::getEvents()
{
    // Force SDL to gather events from input devices. This is called
    // implicitly from SDL_PollEvent but since we're using SDL_PeepEvents to
    // process only mouse events, SDL_PumpEvents is necessary.
    SDL_PumpEvents();

    // Retrieve chunks of up to 1024 events from SDL
    int32_t       num_events = 0;
    const int32_t max_events = 1024;
    SDL_Event     sdl_events[max_events];

    while ((num_events = SDL_PeepEvents(sdl_events, max_events, SDL_GETEVENT, SDL_QUIT, SDL_SYSWMEVENT)))
    {
        for (int32_t i = 0; i < num_events; i++)
        {
            const SDL_Event &sdl_ev = sdl_events[i];

            if (sdl_ev.type == SDL_QUIT ||
                (sdl_ev.type == SDL_WINDOWEVENT && sdl_ev.window.event == SDL_WINDOWEVENT_CLOSE))
            {
                AddCommandString("quit");
            }
            else if (sdl_ev.type == SDL_WINDOWEVENT)
            {
                if (sdl_ev.window.event == SDL_WINDOWEVENT_SHOWN)
                {
                    DPrintf("SDL_WINDOWEVENT_SHOWN\n");
                }
                else if (sdl_ev.window.event == SDL_WINDOWEVENT_HIDDEN)
                {
                    DPrintf("SDL_WINDOWEVENT_HIDDEN\n");
                    mMouseFocus = mKeyboardFocus = false;
                }
                else if (sdl_ev.window.event == SDL_WINDOWEVENT_EXPOSED)
                {
                    DPrintf("SDL_WINDOWEVENT_EXPOSED\n");
                    mMouseFocus = mKeyboardFocus = true;
                }
                else if (sdl_ev.window.event == SDL_WINDOWEVENT_MINIMIZED)
                {
                    DPrintf("SDL_WINDOWEVENT_MINIMIZED\n");
                    mMouseFocus = mKeyboardFocus = false;
                }
                else if (sdl_ev.window.event == SDL_WINDOWEVENT_MAXIMIZED)
                {
                    DPrintf("SDL_WINDOWEVENT_MAXIMIZED\n");
                }
                else if (sdl_ev.window.event == SDL_WINDOWEVENT_RESTORED)
                {
                    DPrintf("SDL_WINDOWEVENT_RESTORED\n");
                }
                else if (sdl_ev.window.event == SDL_WINDOWEVENT_ENTER)
                {
                    DPrintf("SDL_WINDOWEVENT_ENTER\n");
                    mMouseFocus = true;
                }
                else if (sdl_ev.window.event == SDL_WINDOWEVENT_LEAVE)
                {
                    DPrintf("SDL_WINDOWEVENT_LEAVE\n");
                    mMouseFocus = false;
                }
                else if (sdl_ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                {
                    DPrintf("SDL_WINDOWEVENT_FOCUS_GAINED\n");
                    mKeyboardFocus = true;
                }
                else if (sdl_ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                {
                    DPrintf("SDL_WINDOWEVENT_FOCUS_LOST\n");
                    mKeyboardFocus = false;
                }
                else if (sdl_ev.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    // Resizable window mode resolutions
                    uint16_t width  = sdl_ev.window.data1;
                    uint16_t height = sdl_ev.window.data2;
                    DPrintf("SDL_WINDOWEVENT_RESIZED (%dx%d)\n", width, height);

                    int32_t current_time = I_MSTime();
                    if ((EWindowMode)vid_fullscreen.asInt() == WINDOW_Windowed &&
                        current_time > mAcceptResizeEventsTime)
                    {
                        char tmp[30];
                        sprintf(tmp, "vid_setmode %d %d", width, height);
                        AddCommandString(tmp);
                    }
                }
            }
        }
    }
}

//
// ISDL20Window::startRefresh
//
void ISDL20Window::startRefresh()
{
    getEvents();
}

//
// ISDL20Window::finishRefresh
//
void ISDL20Window::finishRefresh()
{
    if (mNeedPaletteRefresh)
    {
        //		if (sdlsurface->format->BitsPerPixel == 8)
        //			SDL_SetSurfacePalette(sdlsurface, sdlsurface->format->palette);
    }

    mNeedPaletteRefresh = false;

    MUD_FrameMark;
}

//
// ISDL20Window::setWindowTitle
//
// Sets the title caption of the window.
//
void ISDL20Window::setWindowTitle(const std::string &str)
{
    SDL_SetWindowTitle(mSDLWindow, str.c_str());
}

//
// ISDL20Window::setWindowIcon
//
// Sets the icon for the application window, which will appear in the
// window manager's task list.
//
void ISDL20Window::setWindowIcon()
{
#if defined(_WIN32)
    // [SL] Use Win32-specific code to make use of multiple-icon sizes
    // stored in the executable resources.
    //
    // [Russell] - Just for windows, display the icon in the system menu and
    // alt-tab display

    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

    if (hIcon)
    {
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version)
        SDL_GetWindowWMInfo(mSDLWindow, &wminfo);
        HWND WindowHandle = wminfo.info.win.window;

        if (WindowHandle)
        {
            SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(WindowHandle, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
    }
#endif // _WIN32

#if !defined(_WIN32)
    SDL_Surface *icon_surface = SDL_CreateRGBSurfaceFrom(
        (void *)app_icon.pixel_data, app_icon.width, app_icon.height, app_icon.bytes_per_pixel * 8,
        app_icon.width * app_icon.bytes_per_pixel, 0xff << 0, 0xff << 8, 0xff << 16, 0xff << 24);

    SDL_SetWindowIcon(mSDLWindow, icon_surface);
    SDL_FreeSurface(icon_surface);
#endif // !_WIN32
}

//
// ISDL20Window::isFocused
//
// Returns true if this window has input focus.
//
bool ISDL20Window::isFocused() const
{
    return mMouseFocus && mKeyboardFocus;
}

//
// ISDL20Window:flashWindow
//
// Flashes the taskbar icon if the window is not focused
//
void ISDL20Window::flashWindow() const
{
#if defined(SDL2016)
    if (!this->isFocused())
        SDL_FlashWindow(mSDLWindow, SDL_FLASH_UNTIL_FOCUSED);
#endif
}

//
// ISDL20Window::getVideoDriverName
//
// Returns the name of the video driver that SDL is currently
// configured to use.
//
std::string ISDL20Window::getVideoDriverName() const
{
    const char *driver_name = SDL_GetCurrentVideoDriver();
    if (driver_name)
        return std::string(driver_name);
    return "";
}

//
// I_BuildPixelFormatFromSDLPixelFormatEnum
//
// Helper function that extracts information about the pixel format
// from an SDL_PixelFormatEnum value and uses it to initialize a PixelFormat
// object.
//
static void I_BuildPixelFormatFromSDLPixelFormatEnum(uint32_t sdl_fmt, PixelFormat *format)
{
    uint32_t amask, rmask, gmask, bmask;
    int32_t  bpp;
    SDL_PixelFormatEnumToMasks(sdl_fmt, &bpp, &rmask, &gmask, &bmask, &amask);

    uint32_t rshift = 0, rloss = 8;
    for (uint32_t n = rmask; (n & 1) == 0; n >>= 1, rshift++)
    {
    }
    for (uint32_t n = rmask >> rshift; (n & 1) == 1; n >>= 1, rloss--)
    {
    }

    uint32_t gshift = 0, gloss = 8;
    for (uint32_t n = gmask; (n & 1) == 0; n >>= 1, gshift++)
    {
    }
    for (uint32_t n = gmask >> gshift; (n & 1) == 1; n >>= 1, gloss--)
    {
    }

    uint32_t bshift = 0, bloss = 8;
    for (uint32_t n = bmask; (n & 1) == 0; n >>= 1, bshift++)
    {
    }
    for (uint32_t n = bmask >> bshift; (n & 1) == 1; n >>= 1, bloss--)
    {
    }

    // handle SDL not reporting correct value for amask
    uint8_t ashift = bpp == 32 ? 48 - rshift - gshift - bshift : 0;
    uint8_t aloss  = bpp == 32 ? 0 : 8;

    // Create the PixelFormat specification
    *format = PixelFormat(bpp, 8 - aloss, 8 - rloss, 8 - gloss, 8 - bloss, ashift, rshift, gshift, bshift);
}

//
// ISDL20Window::discoverNativePixelFormat
//
void ISDL20Window::discoverNativePixelFormat()
{
    SDL_DisplayMode sdl_mode;
    SDL_GetWindowDisplayMode(mSDLWindow, &sdl_mode);
    I_BuildPixelFormatFromSDLPixelFormatEnum(sdl_mode.format, &mPixelFormat);
}

//
// ISDL20Window::buildSurfacePixelFormat
//
// Creates a PixelFormat instance based on the user's requested BPP
// and the native desktop video mode.
//
PixelFormat ISDL20Window::buildSurfacePixelFormat(uint8_t bpp)
{
    uint8_t native_bpp = getPixelFormat()->getBitsPerPixel();
    if (bpp == 8)
        I_Error("Invalid 8pp pixel format");
    else if (bpp == 32 && native_bpp == 32)
        return *getPixelFormat();
    else
        I_Error("Invalid video surface conversion from %i-bit to %i-bit", bpp, native_bpp);
    return PixelFormat(); // shush warnings regarding no return value
}

//
// ISDL20Window::setMode
//
// Sets the window size to the specified size and frees the existing primary
// surface before instantiating a new primary surface. This function performs
// no sanity checks on the desired video mode.
//
bool ISDL20Window::setMode(const IVideoMode &video_mode)
{
    bool change_dimensions  = (video_mode.width != mVideoMode.width || video_mode.height != mVideoMode.height);
    bool change_window_mode = (video_mode.window_mode != mVideoMode.window_mode);

    if (change_dimensions)
    {
        // SDL has a bug where the window size cannot be changed in full screen modes.
        // If changing resolution in fullscreen, we need to change the window mode
        // to windowed and then back to fullscreen.
        if (mVideoMode.window_mode != WINDOW_Windowed)
        {
            SDL_SetWindowFullscreen(mSDLWindow, 0);
            change_window_mode = true; // need to change the window mode back
        }

        // Set the window size
        SDL_SetWindowSize(mSDLWindow, video_mode.width, video_mode.height);

        mVideoMode.width  = video_mode.width;
        mVideoMode.height = video_mode.height;

        // SDL will publish various window resizing events in response to changing the
        // window dimensions or toggling window modes. We need to ignore these.
        mAcceptResizeEventsTime = I_MSTime() + 60;
    }

    // Set the winodow mode (window / full screen)
    if (change_window_mode)
    {
        uint32_t fullscreen_flags = 0;
        if (video_mode.window_mode == WINDOW_DesktopFullscreen)
            fullscreen_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        else if (video_mode.window_mode == WINDOW_Fullscreen)
            fullscreen_flags |= SDL_WINDOW_FULLSCREEN;
        SDL_SetWindowFullscreen(mSDLWindow, fullscreen_flags);

        // SDL has a bug where it sets the total window size (including window decorations & title bar)
        // to the requested window size when transitioning from fullscreen to windowed.
        // So we need to reset the window size again.
        if (video_mode.window_mode == WINDOW_Windowed)
            SDL_SetWindowSize(mSDLWindow, video_mode.width, video_mode.height);

        mVideoMode.window_mode = video_mode.window_mode;

        // SDL will publish various window resizing events in response to changing the
        // window dimensions or toggling window modes. We need to ignore these.
        mAcceptResizeEventsTime = I_MSTime() + 1000;
    }

    // Set the window position on the screen
    if (!isFullScreen())
        SDL_SetWindowPosition(mSDLWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // Set the surface pixel format
    PixelFormat format = buildSurfacePixelFormat(video_mode.bpp);
    // Discover the argb_t pixel format
    if (format.getBitsPerPixel() == 32)
        argb_t::setChannels(format.getAPos(), format.getRPos(), format.getGPos(), format.getBPos());
    else
    {
        argb_t::setChannels(3, 2, 1, 0);
    }
    mVideoMode.bpp = format.getBitsPerPixel();

    mVideoMode.vsync        = video_mode.vsync;
    mVideoMode.stretch_mode = video_mode.stretch_mode;

    UI_SetMode(mVideoMode.width, mVideoMode.height, &format, this, mVideoMode.vsync, mVideoMode.stretch_mode.c_str());

    return true;
}

//
// ISDL20Window::getCurrentWidth
//
uint16_t ISDL20Window::getCurrentWidth() const
{
    int32_t width;
    SDL_GetWindowSize(mSDLWindow, &width, NULL);
    return width;
}

//
// ISDL20Window::getCurrentHeight
//
uint16_t ISDL20Window::getCurrentHeight() const
{
    int32_t height;
    SDL_GetWindowSize(mSDLWindow, NULL, &height);
    return height;
}

//
// ISDL20Window::getCurrentWindowMode
//
EWindowMode ISDL20Window::getCurrentWindowMode() const
{
    uint32_t window_flags = SDL_GetWindowFlags(mSDLWindow);
    if ((window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP)
        return WINDOW_DesktopFullscreen;
    else if ((window_flags & SDL_WINDOW_FULLSCREEN) == SDL_WINDOW_FULLSCREEN)
        return WINDOW_Fullscreen;
    else
        return WINDOW_Windowed;
}

// ****************************************************************************

// ============================================================================
//
// ISDL20VideoSubsystem class implementation
//
// ============================================================================

//
// ISDL20VideoSubsystem::ISDL20VideoSubsystem
//
// Initializes SDL video and sets a few SDL configuration options.
//
ISDL20VideoSubsystem::ISDL20VideoSubsystem() : IVideoSubsystem()
{
    SDL_version linked, compiled;
    SDL_GetVersion(&linked);
    SDL_VERSION(&compiled);

    if (linked.major != compiled.major || linked.minor != compiled.minor)
    {
        I_Error("SDL version conflict (%d.%d.%d vs %d.%d.%d dll)\n", compiled.major, compiled.minor, compiled.patch,
                linked.major, linked.minor, linked.patch);
        return;
    }

    if (linked.patch != compiled.patch)
    {
        Printf(PRINT_WARNING, "SDL version warning (%d.%d.%d vs %d.%d.%d dll)\n", compiled.major, compiled.minor,
               compiled.patch, linked.major, linked.minor, linked.patch);
    }

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
    {
        I_Error("Could not initialize SDL video.\n");
        return;
    }

    mVideoCapabilities = new ISDL20VideoCapabilities();

    mWindow = new ISDL20Window(640, 480, 8, WINDOW_Windowed, false);
}

//
// ISDL20VideoSubsystem::~ISDL20VideoSubsystem
//
ISDL20VideoSubsystem::~ISDL20VideoSubsystem()
{
    delete mWindow;
    delete mVideoCapabilities;

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

//
// ISDL20VideoSubsystem::getMonitorCount
//
int32_t ISDL20VideoSubsystem::getMonitorCount() const
{
    return SDL_GetNumVideoDisplays();
}
#endif // SDL20

VERSION_CONTROL(i_video_sdl20_cpp, "$Id: 701bbcdb6e1a40f728d014baddb3d8e4677e368a $")
