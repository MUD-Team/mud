// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: cb4a805320ca661c8df874d5221512881c34a067 $
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
//
// Low-level video hardware management.
//
//-----------------------------------------------------------------------------

#include "i_video.h"

#include <algorithm>
#include <climits>
#include <cstdlib>

#include "cmdlib.h"
#include "i_input.h"
#include "i_sdl.h"
#include "i_system.h"
#include "i_video_sdl20.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "mud_includes.h"
#include "v_video.h"
#include "w_wad.h"
#if defined(_WIN32)
#include "SDL_syswm.h"
#include "resource.h"
#include "win32inc.h"
#endif // _WIN32

#include "ui/ui_public.h"

// Declared in doomtype.h as part of argb_t
uint8_t argb_t::a_num, argb_t::r_num, argb_t::g_num, argb_t::b_num;

// Global IVideoSubsystem instance for video startup and shutdown
static IVideoSubsystem *video_subsystem = NULL;

extern int32_t NewWidth, NewHeight, NewBits, DisplayBits;

EXTERN_CVAR(vid_fullscreen)
EXTERN_CVAR(vid_vsync)
EXTERN_CVAR(vid_filter)
EXTERN_CVAR(vid_overscan)
EXTERN_CVAR(vid_autoadjust)
EXTERN_CVAR(vid_displayfps)
EXTERN_CVAR(vid_ticker)
EXTERN_CVAR(vid_widescreen)
EXTERN_CVAR(sv_allowwidescreen)

// ****************************************************************************

// ============================================================================
//
// IWindowSurface abstract base class implementation
//
// Default implementation for the IWindowSurface base class.
//
// ============================================================================

IRenderSurface *IRenderSurface::mCurrentRenderSurface = nullptr;

//
// IWindowSurface::IWindowSurface
//
// Initializes a new IWindowSurface given a width, height, and PixelFormat.
// If a buffer is not given, one will be allocated and marked for deallocation
// from the destructor. If the pitch is not given, the width will be used as
// the basis for the pitch.
//
IRenderSurface::IRenderSurface(uint16_t width, uint16_t height, const PixelFormat *format, void *buffer, uint16_t pitch)
    : mSurfaceBuffer((uint8_t *)buffer), mOwnsSurfaceBuffer(buffer == NULL), mPalette(V_GetDefaultPalette()->colors),
      mPixelFormat(*format), mWidth(width), mHeight(height), mPitch(pitch), mLocks(0)
{
    const uintptr_t alignment = 16;

    // Not given a pitch? Just base pitch on the given width
    if (pitch == 0)
    {
        // make the pitch a multiple of the alignment value
        mPitch = (mWidth * mPixelFormat.getBytesPerPixel() + alignment - 1) & ~(alignment - 1);
        // add a little to the pitch to prevent cache thrashing if it's 512 or 1024
        if ((mPitch & 511) == 0)
            mPitch += alignment;
    }

    mPitchInPixels = mPitch / mPixelFormat.getBytesPerPixel();

    if (mOwnsSurfaceBuffer)
    {
        uint8_t *buffer = new uint8_t[mPitch * mHeight + alignment];

        // calculate the offset from buffer to the next aligned memory address
        uintptr_t offset = ((uintptr_t)(buffer + alignment) & ~(alignment - 1)) - (uintptr_t)buffer;

        mSurfaceBuffer = buffer + offset;

        // verify we'll have enough room to store offset immediately
        // before mSurfaceBuffer's address and that offset's value is sane.
        assert(offset > 0 && offset <= alignment);

        // store mSurfaceBuffer's offset from buffer so that mSurfaceBuffer
        // can be properly freed later.
        buffer[offset - 1] = offset;
    }
}

//
// IWindowSurface::~IWindowSurface
//
// Frees all of the DCanvas objects that were instantiated by this surface.
//
IRenderSurface::~IRenderSurface()
{
    // calculate the buffer's original address when freeing mSurfaceBuffer
    if (mOwnsSurfaceBuffer)
    {
        ptrdiff_t offset = mSurfaceBuffer[-1];
        delete[] (mSurfaceBuffer - offset);
    }
}

//
// Pixel format conversion function used by IWindowSurface::blit
//
template <typename SOURCE_PIXEL_T, typename DEST_PIXEL_T>
static inline DEST_PIXEL_T ConvertPixel(SOURCE_PIXEL_T value, const argb_t *palette);

template <> inline palindex_t ConvertPixel(palindex_t value, const argb_t *palette)
{
    return value;
}

template <> inline argb_t ConvertPixel(palindex_t value, const argb_t *palette)
{
    return palette[value];
}

template <> inline palindex_t ConvertPixel(argb_t value, const argb_t *palette)
{
    return 0;
}

template <> inline argb_t ConvertPixel(argb_t value, const argb_t *palette)
{
    return value;
}

//
// IWindowSurface::clear
//
// Clears the surface to black.
//
void IRenderSurface::clear()
{
    const argb_t color(255, 0, 0, 0);

    lock();

    argb_t *dest = (argb_t *)getBuffer();

    for (int32_t y = 0; y < getHeight(); y++)
    {
        for (int32_t x = 0; x < getWidth(); x++)
            dest[x] = color;

        dest += getPitchInPixels();
    }

    unlock();
}

// ****************************************************************************

//
// I_GetVideoModeString
//
// Returns a string with a text description of the given video mode.
//
std::string I_GetVideoModeString(const IVideoMode &mode)
{
    const char window_strs[][25] = {"window", "full screen exclusive", "full screen window"};

    std::string str;
    StrFormat(str, "%dx%d %dbpp (%s)", mode.width, mode.height, mode.bpp, window_strs[I_GetWindow()->getWindowMode()]);
    return str;
}

//
// I_IsModeSupported
//
// Helper function for I_ValidateVideoMode. Returns true if there is a video
// mode availible with the desired bpp and screen mode.
//
static bool I_IsModeSupported(uint8_t bpp, EWindowMode window_mode)
{
    const IVideoModeList *modelist = I_GetVideoCapabilities()->getSupportedVideoModes();

    for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
        if (it->bpp == bpp && it->window_mode == window_mode)
            return true;

    return false;
}

//
// I_ValidateVideoMode
//
// Transforms the given video mode into a mode that is valid for the current
// video hardware capabilities.
//
static IVideoMode I_ValidateVideoMode(const IVideoMode &mode)
{
    const IVideoMode invalid_mode(0, 0, 0, WINDOW_Windowed);
    IVideoMode       desired_mode = mode;

    desired_mode.width       = clamp<uint16_t>(mode.width, 320, MAXWIDTH);
    desired_mode.height      = clamp<uint16_t>(mode.height, 200, MAXHEIGHT);
    desired_mode.bpp         = mode.bpp;
    desired_mode.window_mode = mode.window_mode;

    // If the user requested a windowed mode, we don't have to worry about
    // the requested dimensions aligning to an actual video resolution.
    if (mode.window_mode != WINDOW_Fullscreen || !vid_autoadjust)
        return desired_mode;

    // Ensure the display type is adhered to
    if (!I_GetVideoCapabilities()->supportsFullScreen())
        desired_mode.window_mode = WINDOW_Windowed;
    else if (!I_GetVideoCapabilities()->supportsWindowed())
        desired_mode.window_mode = WINDOW_Fullscreen;

    // check if the given bit-depth is supported
    if (!I_IsModeSupported(desired_mode.bpp, desired_mode.window_mode))
    {
        // mode is not supported -- check a different bit depth
        desired_mode.bpp = desired_mode.bpp ^ (32 | 8);
        if (!I_IsModeSupported(desired_mode.bpp, desired_mode.window_mode))
            return invalid_mode;
    }

    uint32_t      closest_dist = UINT_MAX;
    const IVideoMode *closest_mode = NULL;

    const IVideoModeList *modelist = I_GetVideoCapabilities()->getSupportedVideoModes();
    for (int32_t iteration = 0; iteration < 2; iteration++)
    {
        for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
        {
            if (*it == desired_mode) // perfect match?
                return *it;

            if (it->bpp == desired_mode.bpp && it->window_mode == desired_mode.window_mode)
            {
                if (iteration == 0 && (it->width < desired_mode.width || it->height < desired_mode.height))
                    continue;

                uint32_t dist = (it->width - desired_mode.width) * (it->width - desired_mode.width) +
                                    (it->height - desired_mode.height) * (it->height - desired_mode.height);

                if (dist < closest_dist)
                {
                    closest_dist = dist;
                    closest_mode = &(*it);
                }
            }
        }

        if (closest_mode != NULL)
            return *closest_mode;
    }

    return invalid_mode;
}

//
// I_SetVideoMode
//
// Main function to set the video mode at the hardware level.
//
void I_SetVideoMode(const IVideoMode &requested_mode)
{
    // ensure the requested mode is valid
    IVideoMode validated_mode   = I_ValidateVideoMode(requested_mode);
    validated_mode.vsync        = bool(vid_vsync.asInt());
    validated_mode.stretch_mode = std::string(vid_filter);
    assert(validated_mode.isValid());

    IWindow *window = I_GetWindow();

    window->setMode(validated_mode);
    I_ForceUpdateGrab();

    // [SL] 2011-11-30 - Prevent the player's view angle from moving
    I_FlushInput();

    assert(I_VideoInitialized());

    if (window->getVideoMode() != requested_mode)
        DPrintf("I_SetVideoMode: could not set video mode to %s. Using %s instead.\n",
                I_GetVideoModeString(requested_mode).c_str(), I_GetVideoModeString(window->getVideoMode()).c_str());
    else
        DPrintf("I_SetVideoMode: set video mode to %s\n", I_GetVideoModeString(window->getVideoMode()).c_str());

    window->enableRefresh();
}

//
// I_VideoInitialized
//
// Returns true if the video subsystem has been initialized.
//
bool I_VideoInitialized()
{
    extern bool UI_RenderInitialized();
    return video_subsystem != NULL && I_GetWindow() != NULL && UI_RenderInitialized();
}

//
// I_ShutdownHardware
//
// Destroys the application window and frees its memory.
//
void STACK_ARGS I_ShutdownHardware()
{
    UI_Shutdown();       

    delete video_subsystem;
    video_subsystem = NULL;
}

//
// I_InitHardware
//
void I_InitHardware()
{
    if (I_IsHeadless())
    {
        video_subsystem = new IDummyVideoSubsystem();
    }
    else
    {
#if defined(SDL20)
        video_subsystem = new ISDL20VideoSubsystem();
#endif
        assert(video_subsystem != NULL);

        const IVideoMode &native_mode = I_GetVideoCapabilities()->getNativeMode();
        Printf(PRINT_HIGH, "I_InitHardware: native resolution: %s\n", I_GetVideoModeString(native_mode).c_str());
    }
}

//
// I_GetVideoCapabilities
//
const IVideoCapabilities *I_GetVideoCapabilities()
{
    // a valid IWindow is not needed for querying video capabilities
    // so don't call I_VideoInitialized
    if (video_subsystem)
        return video_subsystem->getVideoCapabilities();
    return NULL;
}

//
// I_GetWindow
//
// Returns a pointer to the application window object.
//
IWindow *I_GetWindow()
{
    if (video_subsystem)
        return video_subsystem->getWindow();
    return NULL;
}

//
// I_GetVideoWidth
//
// Returns the width of the current video mode. Assumes that the video
// window has already been created.
//
int32_t I_GetVideoWidth()
{
    if (I_VideoInitialized())
        return I_GetWindow()->getWidth();
    return 0;
}

//
// I_GetVideoHeight
//
// Returns the height of the current video mode. Assumes that the video
// window has already been created.
//
int32_t I_GetVideoHeight()
{
    if (I_VideoInitialized())
        return I_GetWindow()->getHeight();
    return 0;
}

//
// I_GetVideoBitDepth
//
// Returns the bits per pixelof the current video mode. Assumes that the video
// window has already been created.
//
int32_t I_GetVideoBitDepth()
{
    if (I_VideoInitialized())
        return I_GetWindow()->getBitsPerPixel();
    return 0;
}

//
// I_AllocateSurface
//
// Creates a new (non-primary) surface and returns it.
//
IRenderSurface *I_AllocateSurface(int32_t width, int32_t height, int32_t bpp)
{
    const PixelFormat *format;

    if (IRenderSurface::getCurrentRenderSurface() &&
        bpp == IRenderSurface::getCurrentRenderSurface()->getBitsPerPixel())
        format = IRenderSurface::getCurrentRenderSurface()->getPixelFormat();
    else if (bpp == 8)
        I_Error("I_AllocateSurface: Requesting 8bpp surface");
    else
        format = I_Get32bppPixelFormat();

    return new IRenderSurface(width, height, format);
}

//
// I_FreeSurface
//
void I_FreeSurface(IRenderSurface *&surface)
{
    delete surface;
    surface = NULL;
}

//
// I_GetSurfaceWidth
//
int32_t I_GetSurfaceWidth()
{
    if (I_VideoInitialized())
        return IRenderSurface::getCurrentRenderSurface()->getWidth();
    return 0;
}

//
// I_GetSurfaceHeight
//
int32_t I_GetSurfaceHeight()
{
    if (I_VideoInitialized())
        return IRenderSurface::getCurrentRenderSurface()->getHeight();
    return 0;
}

bool I_IsWideResolution()
{
    IRenderSurface *surface = IRenderSurface::getCurrentRenderSurface();
    if (!surface)
    {
        return false;
    }

    return I_IsWideResolution(surface->getWidth(), surface->getHeight());
}
//
// I_IsWideResolution
//
bool I_IsWideResolution(int32_t width, int32_t height)
{

    // consider the mode widescreen if it's width-to-height ratio is
    // closer to 16:10 than it is to 4:3
    return abs(15 * width - 20 * height) > abs(15 * width - 24 * height);
}

//
// I_BeginUpdate
//
// Called at the start of drawing a new video frame. This locks the primary
// surface for drawing.
//
void I_BeginUpdate()
{
    if (I_VideoInitialized())
    {
        I_GetWindow()->startRefresh();              
    }
}

//
// I_FinishUpdate
//
// Called at the end of drawing a video frame. This unlocks the primary
// surface for drawing and blits the contents to the video screen.
//
void I_FinishUpdate()
{
    if (I_VideoInitialized())
    {        
        I_GetWindow()->finishRefresh();
    }
}

//
// I_SetWindowCaption
//
// Set the window caption.
//
void I_SetWindowCaption(const std::string &caption)
{
    // [Russell] - A basic version string that will eventually get replaced

    std::string title("MUD ");
    title += NiceVersion();

    if (!caption.empty())
        title += " - " + caption;

    I_GetWindow()->setWindowTitle(title);
}

//
// I_SetWindowIcon
//
// Set the window icon (currently Windows only).
//
void I_SetWindowIcon()
{
    I_GetWindow()->setWindowIcon();
}

//
// I_GetMonitorCount
//
int32_t I_GetMonitorCount()
{
    if (I_VideoInitialized())
        return video_subsystem->getMonitorCount();
    return 0;
}

//
// I_GetVideoDriverName
//
// Returns the name of the current video driver in-use.
//
std::string I_GetVideoDriverName()
{
    if (I_VideoInitialized())
        return I_GetWindow()->getVideoDriverName();
    return std::string();
}

//
// I_Get32bppPixelFormat
//
// Returns the platform's PixelFormat for 32bpp video modes.
// This changes on certain platforms depending on the current video mode
// in use (or fullscreen vs windowed).
//
const PixelFormat *I_Get32bppPixelFormat()
{
    if (IRenderSurface::getCurrentRenderSurface() && IRenderSurface::getCurrentRenderSurface()->getBitsPerPixel() == 32)
        return IRenderSurface::getCurrentRenderSurface()->getPixelFormat();

#ifdef __BIG_ENDIAN__
    static PixelFormat format(32, 0, 0, 0, 0, 0, 8, 16, 24);
#else
    static PixelFormat format(32, 0, 0, 0, 0, 24, 16, 8, 0);
#endif

    return &format;
}

VERSION_CONTROL(i_video_cpp, "$Id: cb4a805320ca661c8df874d5221512881c34a067 $")
