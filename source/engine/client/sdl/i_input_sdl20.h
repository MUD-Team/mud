// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 0d3d9012a2ab13c7d9dd683ca127d1075cba8bc3 $
//
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//
//
//-----------------------------------------------------------------------------

#pragma once

#include <list>
#include <queue>

#include "d_event.h"
#include "hashtable.h"
#include "i_input.h"
#include "i_sdl.h"

typedef OHashTable<int32_t, int32_t> KeyTranslationTable;

#ifdef SDL20

// ============================================================================
//
// ISDL20KeyboardInputDevice class interface
//
// ============================================================================

class ISDL20KeyboardInputDevice : public IKeyboardInputDevice
{
  public:
    ISDL20KeyboardInputDevice(int32_t id);
    virtual ~ISDL20KeyboardInputDevice();

    virtual bool active() const;

    virtual void pause();
    virtual void resume();
    virtual void reset();

    virtual void gatherEvents();

    virtual bool hasEvent() const
    {
        return !mEvents.empty();
    }

    virtual void getEvent(event_t *ev);

    virtual void flushEvents();

    virtual void enableTextEntry();
    virtual void disableTextEntry();

  private:
    int32_t translateKey(SDL_Keysym keysym);
    int32_t getTextEventValue();

    bool mActive;
    bool mTextEntry;

    typedef std::queue<event_t> EventQueue;
    EventQueue                  mEvents;
};

// ============================================================================
//
// ISDL20MouseInputDevice class interface
//
// ============================================================================

class ISDL20MouseInputDevice : public IInputDevice
{
  public:
    ISDL20MouseInputDevice(int32_t id);
    virtual ~ISDL20MouseInputDevice();

    virtual bool active() const;

    virtual void pause();
    virtual void resume();
    virtual void reset();

    virtual void gatherEvents();

    virtual bool hasEvent() const
    {
        return !mEvents.empty();
    }

    virtual void getEvent(event_t *ev);

    virtual void flushEvents();

  private:
    bool mActive;

    typedef std::queue<event_t> EventQueue;
    EventQueue                  mEvents;
};

// ============================================================================
//
// ISDL20JoystickInputDevice class interface
//
// ============================================================================

class ISDL20JoystickInputDevice : public IInputDevice
{
  public:
    ISDL20JoystickInputDevice(int32_t id);
    virtual ~ISDL20JoystickInputDevice();

    virtual bool active() const;

    virtual void pause();
    virtual void resume();
    virtual void reset();

    virtual void gatherEvents();

    virtual bool hasEvent() const
    {
        return !mEvents.empty();
    }

    virtual void getEvent(event_t *ev);

    virtual void flushEvents();

  private:
    int32_t calcAxisValue(int32_t raw_value);

    static const int32_t JOY_DEADZONE = 6000;

    bool mActive;

    typedef std::queue<event_t> EventQueue;
    EventQueue                  mEvents;

    int32_t                 mJoystickId;
    SDL_GameController *mJoystick;
};

// ============================================================================
//
// ISDL20InputSubsystem class interface
//
// ============================================================================

class ISDL20InputSubsystem : public IInputSubsystem
{
  public:
    ISDL20InputSubsystem();
    virtual ~ISDL20InputSubsystem();

    virtual void grabInput();
    virtual void releaseInput();

    virtual bool isInputGrabbed() const
    {
        return mInputGrabbed;
    }

    virtual std::vector<IInputDeviceInfo> getKeyboardDevices() const;
    virtual void                          initKeyboard(int32_t id);
    virtual void                          shutdownKeyboard(int32_t id);

    virtual std::vector<IInputDeviceInfo> getMouseDevices() const;
    virtual void                          initMouse(int32_t id);
    virtual void                          shutdownMouse(int32_t id);

    virtual std::vector<IInputDeviceInfo> getJoystickDevices() const;
    virtual void                          initJoystick(int32_t id);
    virtual void                          shutdownJoystick(int32_t id);

  private:
    bool mInputGrabbed;
};

#endif // SDL20
