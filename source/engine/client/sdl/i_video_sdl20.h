// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: a95b41ce3d00c4c009e218723a02e4bb5392d825 $
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
// SDL 2.0 implementation of the IWindow class
//
//-----------------------------------------------------------------------------

#pragma once

#include "i_sdl.h"
#include "i_video.h"

#ifdef SDL20

class ISDL20Window;

// ============================================================================
//
// ISDL20VideoCapabilities class interface
//
// Defines an interface for querying video capabilities. This includes listing
// availible video modes and supported operations.
//
// ============================================================================

class ISDL20VideoCapabilities : public IVideoCapabilities
{
  public:
    ISDL20VideoCapabilities();
    virtual ~ISDL20VideoCapabilities()
    {
    }

    virtual const IVideoModeList *getSupportedVideoModes() const
    {
        return &mModeList;
    }

    virtual const EDisplayType getDisplayType() const
    {
        return DISPLAY_Both;
    }

    virtual const IVideoMode &getNativeMode() const
    {
        return mNativeMode;
    }

  private:
    IVideoModeList mModeList;
    IVideoMode     mNativeMode;
};

// ============================================================================
//
// ISDL20Window class interface
//
// ============================================================================

class ISDL20Window : public IWindow
{
  public:
    ISDL20Window(uint16_t width, uint16_t height, uint8_t bpp, EWindowMode window_mode, bool vsync);

    virtual ~ISDL20Window();

    virtual uint16_t getWidth() const
    {
        return mVideoMode.width;
    }

    virtual uint16_t getHeight() const
    {
        return mVideoMode.height;
    }

    virtual uint8_t getBitsPerPixel() const
    {
        return mVideoMode.bpp;
    }

    virtual int32_t getBytesPerPixel() const
    {
        return mVideoMode.bpp >> 3;
    }

    virtual const IVideoMode &getVideoMode() const
    {
        return mVideoMode;
    }

    virtual const PixelFormat *getPixelFormat() const
    {
        return &mPixelFormat;
    }

    virtual bool setMode(const IVideoMode &video_mode);

    virtual bool isFullScreen() const
    {
        return mVideoMode.isFullScreen();
    }

    virtual EWindowMode getWindowMode() const
    {
        return mVideoMode.window_mode;
    }

    virtual bool isFocused() const;

    virtual void flashWindow() const;

    virtual bool usingVSync() const
    {
        return mVideoMode.vsync;
    }

    virtual void enableRefresh()
    {
        mBlit = true;
    }

    virtual void disableRefresh()
    {
        mBlit = false;
    }

    virtual void startRefresh();
    virtual void finishRefresh();

    virtual void setWindowTitle(const std::string &str = "");
    virtual void setWindowIcon();

    virtual std::string getVideoDriverName() const;

  private:
    // disable copy constructor and assignment operator
    ISDL20Window(const ISDL20Window &);
    ISDL20Window &operator=(const ISDL20Window &);

    friend class UIRenderInterface;

    void        discoverNativePixelFormat();
    PixelFormat buildSurfacePixelFormat(uint8_t bpp);
    void        setRendererDriver();
    bool        isRendererDriverAvailable(const char *driver) const;
    const char *getRendererDriver() const;
    void        getEvents();

    uint16_t    getCurrentWidth() const;
    uint16_t    getCurrentHeight() const;
    EWindowMode getCurrentWindowMode() const;

    SDL_Window *mSDLWindow;    

    IVideoMode  mVideoMode;
    PixelFormat mPixelFormat;

    bool mNeedPaletteRefresh;
    bool mBlit;

    bool mMouseFocus;
    bool mKeyboardFocus;

    int32_t mAcceptResizeEventsTime;    
};

// ****************************************************************************

// ============================================================================
//
// ISDL20VideoSubsystem class interface
//
// Provides intialization and shutdown mechanics for the video subsystem.
// This is really an abstract factory pattern as it instantiates a family
// of concrete types.
//
// ============================================================================

class ISDL20VideoSubsystem : public IVideoSubsystem
{
  public:
    ISDL20VideoSubsystem();
    virtual ~ISDL20VideoSubsystem();

    virtual const IVideoCapabilities *getVideoCapabilities() const
    {
        return mVideoCapabilities;
    }

    virtual const IWindow *getWindow() const
    {
        return mWindow;
    }

    virtual int32_t getMonitorCount() const;

  private:
    const IVideoCapabilities *mVideoCapabilities;

    IWindow *mWindow;
};
#endif // SDL20
