// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: dbbb256d419eb638c44ac5ab6fd24e4b707487a0 $
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
//	System specific interface stuff.
//	[Odamex] Fitted to work with SDL
//
//-----------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <cstdlib>
#include <vector>

#include "doomtype.h"
#include "v_pixelformat.h"

enum EDisplayType
{
    DISPLAY_WindowOnly,
    DISPLAY_FullscreenOnly,
    DISPLAY_Both
};

enum EWindowMode
{
    WINDOW_Windowed          = 0,
    WINDOW_Fullscreen        = 1,
    WINDOW_DesktopFullscreen = 2,
};

// forward definitions
class IVideoMode;
class IVideoCapabilities;
class IWindow;
class IRenderSurface;

void            I_InitHardware();
void STACK_ARGS I_ShutdownHardware();
bool            I_VideoInitialized();

void I_SetVideoMode(const IVideoMode &video_mode);

const IVideoCapabilities *I_GetVideoCapabilities();
int32_t                   I_GetMonitorCount();
IWindow                  *I_GetWindow();

IRenderSurface *I_AllocateSurface(int32_t width, int32_t height, int32_t bpp);
void            I_FreeSurface(IRenderSurface *&surface);

int32_t I_GetVideoWidth();
int32_t I_GetVideoHeight();
int32_t I_GetVideoBitDepth();

int32_t I_GetSurfaceWidth();
int32_t I_GetSurfaceHeight();

bool I_IsWideResolution();
bool I_IsWideResolution(int32_t width, int32_t height);

void I_BeginUpdate();
void I_FinishUpdate();

void I_WaitVBL(int32_t count);

void I_SetWindowCaption(const std::string &caption = "");
void I_SetWindowIcon();

std::string I_GetVideoModeString(const IVideoMode &mode);
std::string I_GetVideoDriverName();

const PixelFormat *I_Get32bppPixelFormat();

// ****************************************************************************

// ============================================================================
//
// IVideoMode class interface
//
// ============================================================================

class IVideoMode
{
  public:
    IVideoMode(uint16_t _width, uint16_t _height, uint8_t _bpp, EWindowMode _window_mode, bool _vsync = false,
               std::string _stretch_mode = "")
        : width(_width), height(_height), bpp(_bpp), window_mode(_window_mode), vsync(_vsync),
          stretch_mode(_stretch_mode)
    {
    }

    bool isFullScreen() const
    {
        return window_mode != WINDOW_Windowed;
    }

    bool operator==(const IVideoMode &other) const
    {
        return width == other.width && height == other.height && bpp == other.bpp && window_mode == other.window_mode &&
               vsync == other.vsync && stretch_mode == other.stretch_mode;
    }

    bool operator!=(const IVideoMode &other) const
    {
        return !(operator==(other));
    }

    bool operator<(const IVideoMode &other) const
    {
        if (width != other.width)
            return width < other.width;
        if (height != other.height)
            return height < other.height;
        if (bpp != other.bpp)
            return bpp < other.bpp;
        if (window_mode != other.window_mode)
            return (int32_t)window_mode < (int32_t)other.window_mode;
        if (vsync != other.vsync)
            return (int32_t)vsync < (int32_t)other.vsync;
        if (stretch_mode != other.stretch_mode)
            return stretch_mode < other.stretch_mode;
        return false;
    }

    bool operator>(const IVideoMode &other) const
    {
        return !operator==(other) && !operator<(other);
    }

    bool operator<=(const IVideoMode &other) const
    {
        return operator<(other) || operator==(other);
    }

    bool operator>=(const IVideoMode &other) const
    {
        return operator>(other) || operator==(other);
    }

    bool isValid() const
    {
        return width > 0 && height > 0 && (bpp == 8 || bpp == 32);
    }

    bool isWideScreen() const
    {
        return I_IsWideResolution(width, height);
    }

    double getAspectRatio() const
    {
        return double(width) / double(height);
    }

    double getPixelAspectRatio() const
    {
        // assume that all widescreen modes have square pixels
        if (isWideScreen())
            return 1.0;

        return double(width) * 0.75 / double(height);
    }

    uint16_t    width;
    uint16_t    height;
    uint8_t     bpp;
    EWindowMode window_mode;
    bool        vsync;
    std::string stretch_mode;
};

typedef std::vector<IVideoMode> IVideoModeList;

// ****************************************************************************

// ============================================================================
//
// IVideoCapabilities abstract base class interface
//
// Defines an interface for querying video capabilities. This includes listing
// availible video modes and supported operations.
//
// ============================================================================

class IVideoCapabilities
{
  public:
    virtual ~IVideoCapabilities()
    {
    }

    virtual const IVideoModeList *getSupportedVideoModes() const = 0;

    virtual bool supportsFullScreen() const
    {
        return getDisplayType() == DISPLAY_FullscreenOnly || getDisplayType() == DISPLAY_Both;
    }

    virtual bool supportsWindowed() const
    {
        return getDisplayType() == DISPLAY_WindowOnly || getDisplayType() == DISPLAY_Both;
    }

    virtual const EDisplayType getDisplayType() const = 0;

    virtual bool supports32bpp() const
    {
        const IVideoModeList *modelist = getSupportedVideoModes();
        for (IVideoModeList::const_iterator it = modelist->begin(); it != modelist->end(); ++it)
        {
            if (it->bpp == 32)
                return true;
        }
        return false;
    }

    virtual const IVideoMode &getNativeMode() const = 0;
};

// ============================================================================
//
// IDummyVideoCapabilities class interface
//
// For use with headless clients.
//
// ============================================================================

class IDummyVideoCapabilities : public IVideoCapabilities
{
  public:
    IDummyVideoCapabilities() : IVideoCapabilities(), mVideoMode(320, 200, 8, WINDOW_Windowed)
    {
        mModeList.push_back(mVideoMode);
    }

    virtual ~IDummyVideoCapabilities()
    {
    }

    virtual const IVideoModeList *getSupportedVideoModes() const
    {
        return &mModeList;
    }

    virtual const EDisplayType getDisplayType() const
    {
        return DISPLAY_WindowOnly;
    }

    virtual const IVideoMode &getNativeMode() const
    {
        return mVideoMode;
    }

  private:
    IVideoModeList mModeList;
    IVideoMode     mVideoMode;
};

// ****************************************************************************

// ============================================================================
//
// IWindowSurface abstract base class interface
//
// Wraps the raw device surface and provides methods to access the raw surface
// buffer.
//
// ============================================================================

class IRenderSurface
{
  public:
    IRenderSurface();
    IRenderSurface(uint16_t width, uint16_t height, const PixelFormat *format, void *buffer = NULL, uint16_t pitch = 0);

    ~IRenderSurface();

    inline const uint8_t *getBuffer() const
    {
        return mSurfaceBuffer;
    }

    inline uint8_t *getBuffer()
    {
        return const_cast<uint8_t *>(static_cast<const IRenderSurface &>(*this).getBuffer());
    }

    inline const uint8_t *getBuffer(uint16_t x, uint16_t y) const
    {
        return mSurfaceBuffer + int32_t(y) * getPitch() + int32_t(x) * getBytesPerPixel();
    }

    inline uint8_t *getBuffer(uint16_t x, uint16_t y)
    {
        return const_cast<uint8_t *>(static_cast<const IRenderSurface &>(*this).getBuffer(x, y));
    }

    inline uint16_t getWidth() const
    {
        return mWidth;
    }

    inline uint16_t getHeight() const
    {
        return mHeight;
    }

    inline uint16_t getPitch() const
    {
        return mPitch;
    }

    inline uint16_t getPitchInPixels() const
    {
        return mPitchInPixels;
    }

    inline uint8_t getBitsPerPixel() const
    {
        return mPixelFormat.getBitsPerPixel();
    }

    inline uint8_t getBytesPerPixel() const
    {
        return mPixelFormat.getBytesPerPixel();
    }

    inline const PixelFormat *getPixelFormat() const
    {
        return &mPixelFormat;
    }

    void clear();

    inline void lock()
    {
        mLocks++;
        assert(mLocks >= 1 && mLocks < 100);
    }

    inline void unlock()
    {
        mLocks--;
        assert(mLocks >= 0 && mLocks < 100);
    }

    static IRenderSurface *getCurrentRenderSurface()
    {
        return mCurrentRenderSurface;
    }
    static void setCurrentRenderSurface(IRenderSurface *surface)
    {
        mCurrentRenderSurface = surface;
    }

  private:
    // disable copy constructor and assignment operator
    IRenderSurface(const IRenderSurface &);
    IRenderSurface &operator=(const IRenderSurface &);

    uint8_t *mSurfaceBuffer;
    bool     mOwnsSurfaceBuffer;

    const argb_t *mPalette;

    PixelFormat mPixelFormat;

    uint16_t mWidth;
    uint16_t mHeight;

    uint16_t mPitch;
    uint16_t mPitchInPixels; // mPitch / mPixelFormat.getBytesPerPixel()

    int16_t mLocks;

    static IRenderSurface *mCurrentRenderSurface;
};

// ****************************************************************************

// ============================================================================
//
// IWindow abstract base class interface
//
// Defines an interface for video windows (including both windowed and
// full-screen modes).
// ============================================================================

class IWindow
{
  public:
    virtual ~IWindow()
    {
    }

    virtual uint16_t getWidth() const = 0;

    virtual uint16_t getHeight() const = 0;

    virtual uint8_t getBitsPerPixel() const = 0;

    virtual int32_t getBytesPerPixel() const = 0;

    virtual const IVideoMode &getVideoMode() const = 0;

    virtual const PixelFormat *getPixelFormat() const = 0;

    virtual bool isFullScreen() const
    {
        return getVideoMode().isFullScreen();
    }

    virtual EWindowMode getWindowMode() const = 0;

    virtual bool isFocused() const
    {
        return false;
    }

    virtual void flashWindow() const
    {
    }

    virtual bool usingVSync() const
    {
        return false;
    }

    virtual bool setMode(const IVideoMode &video_mode) = 0;

    virtual void enableRefresh()
    {
    }
    virtual void disableRefresh()
    {
    }

    virtual void startRefresh()
    {
    }
    virtual void finishRefresh()
    {
    }

    virtual void setWindowTitle(const std::string &caption = "")
    {
    }
    virtual void setWindowIcon()
    {
    }

    virtual std::string getVideoDriverName() const
    {
        return "";
    }
};

// ============================================================================
//
// IDummyWindow abstract base class interface
//
// denis - here is a blank implementation of IWindow that allows the client
// to run without actual video output (e.g. script-controlled demo testing)
//
// ============================================================================

class IDummyWindow : public IWindow
{
  public:
    IDummyWindow()
        : IWindow(), mPrimarySurface(NULL), mVideoMode(320, 200, 32, WINDOW_Windowed),
          mPixelFormat(32, 0, 0, 0, 0, 0, 0, 0, 0)
    {
    }

    virtual ~IDummyWindow()
    {
        delete mPrimarySurface;
    }

    virtual uint16_t getWidth() const
    {
        return getPrimarySurface()->getWidth();
    }

    virtual uint16_t getHeight() const
    {
        return getPrimarySurface()->getHeight();
    }

    virtual uint8_t getBitsPerPixel() const
    {
        return getPrimarySurface()->getBitsPerPixel();
    }

    virtual int32_t getBytesPerPixel() const
    {
        return getPrimarySurface()->getBytesPerPixel();
    }

    virtual const IVideoMode &getVideoMode() const
    {
        return mVideoMode;
    }

    virtual const PixelFormat *getPixelFormat() const
    {
        return &mPixelFormat;
    }

    virtual bool setMode(const IVideoMode &video_mode)
    {
        if (mPrimarySurface == NULL)
        {
            // ignore the requested mode and setup the hardcoded mode
            mPrimarySurface = I_AllocateSurface(mVideoMode.width, mVideoMode.height, mVideoMode.bpp);
        }
        return mPrimarySurface != NULL;
    }

    virtual bool isFullScreen() const
    {
        return mVideoMode.isFullScreen();
    }

    virtual EWindowMode getWindowMode() const
    {
        return mVideoMode.window_mode;
    }

    virtual std::string getVideoDriverName() const
    {
        static const std::string driver_name("headless");
        return driver_name;
    }

  private:
    // disable copy constructor and assignment operator
    IDummyWindow(const IDummyWindow &);
    IDummyWindow &operator=(const IDummyWindow &);

    virtual const IRenderSurface *getPrimarySurface() const
    {
        return mPrimarySurface;
    }

    virtual IRenderSurface *getPrimarySurface()
    {
        return const_cast<IRenderSurface *>(static_cast<const IDummyWindow &>(*this).getPrimarySurface());
    }

    IRenderSurface *mPrimarySurface;

    IVideoMode  mVideoMode;
    PixelFormat mPixelFormat;
};

// ****************************************************************************

// ============================================================================
//
// IVideoSubsystem abstract base class interface
//
// Provides intialization and shutdown mechanics for the video subsystem.
// This is really an abstract factory pattern as it instantiates a family
// of concrete types.
//
// ============================================================================

class IVideoSubsystem
{
  public:
    virtual ~IVideoSubsystem()
    {
    }

    virtual const IVideoCapabilities *getVideoCapabilities() const = 0;

    virtual const IWindow *getWindow() const = 0;

    virtual IWindow *getWindow()
    {
        return const_cast<IWindow *>(static_cast<const IVideoSubsystem &>(*this).getWindow());
    }

    virtual int32_t getMonitorCount() const
    {
        return 1;
    }
};

// ============================================================================
//
// IDummyVideoSubsystem class interface
//
// Video subsystem for headless clients.
//
// ============================================================================

class IDummyVideoSubsystem : public IVideoSubsystem
{
  public:
    IDummyVideoSubsystem() : IVideoSubsystem()
    {
        mVideoCapabilities = new IDummyVideoCapabilities();
        mWindow            = new IDummyWindow();
    }

    virtual ~IDummyVideoSubsystem()
    {
        delete mWindow;
        delete mVideoCapabilities;
    }

    virtual const IVideoCapabilities *getVideoCapabilities() const
    {
        return mVideoCapabilities;
    }

    virtual const IWindow *getWindow() const
    {
        return mWindow;
    }

  private:
    const IVideoCapabilities *mVideoCapabilities;

    IWindow *mWindow;
};
