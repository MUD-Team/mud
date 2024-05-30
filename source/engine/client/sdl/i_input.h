// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 74c1392832348f77ea4b7ef05f41a92454e65d74 $
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
//	SDL input handler
//
//-----------------------------------------------------------------------------

#pragma once

#include <SDL.h>

#include <list>
#include <queue>

#include "d_event.h"
#include "doomtype.h"
#include "hashtable.h"
#include "win32inc.h"

#define MOUSE_DOOM     0
#define MOUSE_ZDOOM_DI 1

bool            I_InitInput(void);
void            I_ShutdownInput(void);
void            I_ForceUpdateGrab();
void            I_FlushInput();

int32_t         I_GetJoystickCount();
std::string I_GetJoystickNameFromIndex(int32_t index);
bool        I_OpenJoystick();
void        I_CloseJoystick();
std::string I_GetKeyName(int32_t key);
int32_t         I_GetKeyFromName(const std::string &name);

void I_GetEvents();

// ============================================================================
//
// IInputDevice abstract base class interface
//
// ============================================================================

class IInputDevice
{
  public:
    virtual ~IInputDevice()
    {
    }

    virtual bool active() const = 0;
    virtual void pause()        = 0;
    virtual void resume()       = 0;
    virtual void reset()        = 0;

    virtual void gatherEvents()        = 0;
    virtual bool hasEvent() const      = 0;
    virtual void getEvent(event_t *ev) = 0;

    virtual void flushEvents()
    {
        event_t ev;
        gatherEvents();
        while (hasEvent())
            getEvent(&ev);
    }
};

struct IInputDeviceInfo
{
    std::string mDeviceName;
    int32_t         mId;
};

// ============================================================================
//
// IKeyboardInputDevice abstract base class interface
//
// ============================================================================

class IKeyboardInputDevice : public IInputDevice
{
  public:
    virtual void enableTextEntry()
    {
    }
    virtual void disableTextEntry()
    {
    }
};

// ============================================================================
//
// IInputSubsystem abstract base class interface
//
// ============================================================================

class IInputSubsystem
{
  public:
    IInputSubsystem();
    virtual ~IInputSubsystem();

    virtual void grabInput()    = 0;
    virtual void releaseInput() = 0;

    virtual bool isInputGrabbed() const
    {
        return false;
    }

    virtual void enableKeyRepeat();
    virtual void disableKeyRepeat();
    virtual void enableTextEntry();
    virtual void disableTextEntry();

    void postEvent(event_t& event)
    {
        if (mRepeating)
            addToEventRepeaters(event);

        mEvents.push(event);
    }
    virtual void flushInput()
    {
        event_t dummy_event;
        gatherEvents();
        while (hasEvent())
            getEvent(&dummy_event);
    }

    virtual bool hasEvent() const
    {
        return mEvents.empty() == false;
    }

    virtual void gatherEvents();
    virtual void gatherMouseEvents();
    virtual void getEvent(event_t *ev);

    virtual std::vector<IInputDeviceInfo> getKeyboardDevices() const = 0;
    virtual void                          initKeyboard(int32_t id)       = 0;
    virtual void                          shutdownKeyboard(int32_t id)   = 0;

    virtual std::vector<IInputDeviceInfo> getMouseDevices() const = 0;
    virtual void                          initMouse(int32_t id)       = 0;
    virtual void                          shutdownMouse(int32_t id)   = 0;

    virtual std::vector<IInputDeviceInfo> getJoystickDevices() const = 0;
    virtual void                          initJoystick(int32_t id)       = 0;
    virtual void                          shutdownJoystick(int32_t id)   = 0;

  protected:
    void registerInputDevice(IInputDevice *device);
    void unregisterInputDevice(IInputDevice *device);

    void setKeyboardInputDevice(IInputDevice *device)
    {
        mKeyboardInputDevice = device;
    }

    IInputDevice *getKeyboardInputDevice()
    {
        return mKeyboardInputDevice;
    }

    void setMouseInputDevice(IInputDevice *device)
    {
        mMouseInputDevice = device;
    }

    IInputDevice *getMouseInputDevice()
    {
        return mMouseInputDevice;
    }

    void setJoystickInputDevice(IInputDevice *device)
    {
        mJoystickInputDevice = device;
    }

    IInputDevice *getJoystickInputDevice()
    {
        return mJoystickInputDevice;
    }

    static const uint64_t mRepeatDelay;
    static const uint64_t mRepeatInterval;

  private:
    void addToEventRepeaters(event_t &ev);
    void repeatEvents();

    // Data for key repeating
    struct EventRepeater
    {
        uint64_t last_time;
        bool     repeating;
        event_t  event;
    };

    // the EventRepeaterTable hashtable typedef uses
    // event_t::data1 as its key as there should only be
    // a single instance with that value in the table.
    typedef OHashTable<int32_t, EventRepeater> EventRepeaterTable;
    EventRepeaterTable                     mEventRepeaters;

    bool mRepeating;

    typedef std::queue<event_t> EventQueue;
    EventQueue                  mEvents;

    // Input device management
    typedef std::list<IInputDevice *> InputDeviceList;
    InputDeviceList                   mInputDevices;

    IInputDevice *mKeyboardInputDevice;
    IInputDevice *mMouseInputDevice;
    IInputDevice *mJoystickInputDevice;
};

typedef OHashTable<int32_t, std::string> KeyNameTable;
extern KeyNameTable                  key_names;

void I_SetRelativeMouseMode(bool relative);
void I_PostInputEvent(event_t& event);
void I_HandleInputEvents();
bool I_TranslateSDLEvent(const SDL_Event& sdl, event_t& event);

