//----------------------------------------------------------------------------
//  EDGE SDL Controller Stuff
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

#include <vector>

#include "dm_defs.h"
#include "dm_state.h"
#include "e_event.h"
#include "e_input.h"
#include "e_main.h"
#include "edge_profiling.h"
#include "epi.h"
#include "epi_str_util.h"
#include "gamepad/Gamepad.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "r_modes.h"
#include "sokol_time.h"

extern ConsoleVariable double_framerate;

static std::vector<sapp_event> s_control_events;

bool no_joystick;                // what a wowser, joysticks completely disabled

Gamepad_device     *gamepad_info    = nullptr;

// Track trigger state to avoid pushing multiple unnecessary trigger events
bool right_trigger_pulled = false;
bool left_trigger_pulled  = false;

//
// Translates a key from Sokol -> EDGE
// Returns -1 if no suitable translation exists.
//
int TranslateSokolKey(sapp_keycode key)
{
    switch (key)
    {
    case SAPP_KEYCODE_GRAVE_ACCENT:
        return kTilde;
    case SAPP_KEYCODE_MINUS:
        return kMinus;
    case SAPP_KEYCODE_EQUAL:
        return kEquals;

    case SAPP_KEYCODE_TAB:
        return kTab;
    case SAPP_KEYCODE_ENTER:
        return kEnter;
    case SAPP_KEYCODE_ESCAPE:
        return kEscape;
    case SAPP_KEYCODE_BACKSPACE:
        return kBackspace;

    case SAPP_KEYCODE_UP:
        return kUpArrow;
    case SAPP_KEYCODE_DOWN:
        return kDownArrow;
    case SAPP_KEYCODE_LEFT:
        return kLeftArrow;
    case SAPP_KEYCODE_RIGHT:
        return kRightArrow;

    case SAPP_KEYCODE_HOME:
        return kHome;
    case SAPP_KEYCODE_END:
        return kEnd;
    case SAPP_KEYCODE_INSERT:
        return kInsert;
    case SAPP_KEYCODE_DELETE:
        return kDelete;
    case SAPP_KEYCODE_PAGE_UP:
        return kPageUp;
    case SAPP_KEYCODE_PAGE_DOWN:
        return kPageDown;

    case SAPP_KEYCODE_F1:
        return kFunction1;
    case SAPP_KEYCODE_F2:
        return kFunction2;
    case SAPP_KEYCODE_F3:
        return kFunction3;
    case SAPP_KEYCODE_F4:
        return kFunction4;
    case SAPP_KEYCODE_F5:
        return kFunction5;
    case SAPP_KEYCODE_F6:
        return kFunction6;
    case SAPP_KEYCODE_F7:
        return kFunction7;
    case SAPP_KEYCODE_F8:
        return kFunction8;
    case SAPP_KEYCODE_F9:
        return kFunction9;
    case SAPP_KEYCODE_F10:
        return kFunction10;
    case SAPP_KEYCODE_F11:
        return kFunction11;
    case SAPP_KEYCODE_F12:
        return kFunction12;

    case SAPP_KEYCODE_KP_0:
        return kKeypad0;
    case SAPP_KEYCODE_KP_1:
        return kKeypad1;
    case SAPP_KEYCODE_KP_2:
        return kKeypad2;
    case SAPP_KEYCODE_KP_3:
        return kKeypad3;
    case SAPP_KEYCODE_KP_4:
        return kKeypad4;
    case SAPP_KEYCODE_KP_5:
        return kKeypad5;
    case SAPP_KEYCODE_KP_6:
        return kKeypad6;
    case SAPP_KEYCODE_KP_7:
        return kKeypad7;
    case SAPP_KEYCODE_KP_8:
        return kKeypad8;
    case SAPP_KEYCODE_KP_9:
        return kKeypad9;

    case SAPP_KEYCODE_KP_DECIMAL:
        return kKeypadDot;
    case SAPP_KEYCODE_KP_ADD:
        return kKeypadPlus;
    case SAPP_KEYCODE_KP_SUBTRACT:
        return kKeypadMinus;
    case SAPP_KEYCODE_KP_MULTIPLY:
        return kKeypadStar;
    case SAPP_KEYCODE_KP_DIVIDE:
        return kKeypadSlash;
    case SAPP_KEYCODE_KP_EQUAL:
        return kKeypadEquals;
    case SAPP_KEYCODE_KP_ENTER:
        return kKeypadEnter;

    case SAPP_KEYCODE_PRINT_SCREEN:
        return kPrintScreen;
    case SAPP_KEYCODE_CAPS_LOCK:
        return kCapsLock;
    case SAPP_KEYCODE_NUM_LOCK:
        return kNumberLock;
    case SAPP_KEYCODE_SCROLL_LOCK:
        return kScrollLock;
    case SAPP_KEYCODE_PAUSE:
        return kPause;

    case SAPP_KEYCODE_LEFT_SHIFT:
    case SAPP_KEYCODE_RIGHT_SHIFT:
        return kRightShift;
    case SAPP_KEYCODE_LEFT_CONTROL:
    case SAPP_KEYCODE_RIGHT_CONTROL:
        return kRightControl;
    case SAPP_KEYCODE_LEFT_SUPER:
    case SAPP_KEYCODE_LEFT_ALT:
        return kLeftAlt;
    case SAPP_KEYCODE_RIGHT_SUPER:
    case SAPP_KEYCODE_RIGHT_ALT:
        return kRightAlt;

    default:
        break;
    }

    // TODO: Use evt.char_code which is only valid for CHAR events
    if (key >= 32 && key <= 96)
        return epi::ToLowerASCII(key);

    return -1;
}

void HandleFocusGain(void)
{
    // Now active again
    app_state |= kApplicationActive;
}

void HandleFocusLost(void)
{
    GrabCursor(false);

    EdgeIdle();

    // No longer active
    app_state &= ~kApplicationActive;
}

static void HandleKeyEvent(sapp_event *ev)
{
    if (ev->type != SAPP_EVENTTYPE_KEY_DOWN && ev->type != SAPP_EVENTTYPE_KEY_UP)
        return;

    InputEvent event;
    event.value.key.sym = TranslateSokolKey(ev->key_code);

    // handle certain keys which don't behave normally
    if (ev->key_code == SAPP_KEYCODE_CAPS_LOCK || ev->key_code == SAPP_KEYCODE_NUM_LOCK)
    {
        if (ev->type != SAPP_EVENTTYPE_KEY_DOWN)
            return;
        event.type = kInputEventKeyDown;
        PostEvent(&event);

        event.type = kInputEventKeyUp;
        PostEvent(&event);
        return;
    }

    event.type = (ev->type == SAPP_EVENTTYPE_KEY_DOWN) ? kInputEventKeyDown : kInputEventKeyUp;

    if (event.value.key.sym < 0)
    {
        // No translation possible for SDL symbol and no unicode value
        return;
    }

    PostEvent(&event);
}

static void HandleCharEvent(sapp_event *ev)
{
}

void HandleMouseButtonEvent(sapp_event *ev)
{
    InputEvent event;

    if (ev->type == SAPP_EVENTTYPE_MOUSE_DOWN)
        event.type = kInputEventKeyDown;
    else if (ev->type == SAPP_EVENTTYPE_MOUSE_UP)
        event.type = kInputEventKeyUp;
    else
        return;

    switch (ev->mouse_button)
    {
    case 0:
        event.value.key.sym = kMouse1;
        break;
    case 1:
        event.value.key.sym = kMouse2;
        break;
    case 2:
        event.value.key.sym = kMouse3;
        break;

    default:
        return;
    }

    PostEvent(&event);
}

void HandleMouseWheelEvent(sapp_event *ev)
{
    InputEvent event;
    InputEvent release;

    event.type   = kInputEventKeyDown;
    release.type = kInputEventKeyUp;

    if (ev->scroll_y > 0)
    {
        event.value.key.sym   = kMouseWheelUp;
        release.value.key.sym = kMouseWheelUp;
    }
    else if (ev->scroll_y < 0)
    {
        event.value.key.sym   = kMouseWheelDown;
        release.value.key.sym = kMouseWheelDown;
    }
    else
    {
        return;
    }
    PostEvent(&event);
    PostEvent(&release);
}

static void HandleGamepadButtonRelease(Gamepad_device *device, unsigned int buttonID, double timestamp, void * context)
{
    (void)context;
    EPI_ASSERT(device);

    // ignore other gamepads;
    if (device != gamepad_info)
        return;

    InputEvent event;

    event.type = kInputEventKeyUp;
    event.value.key.sym = kGamepadA + buttonID;

    PostEvent(&event);
}

static void HandleGamepadButtonPress(Gamepad_device *device, unsigned int buttonID, double timestamp, void * context)
{
    (void)context;
    EPI_ASSERT(device);

    // ignore other gamepads;
    if (device != gamepad_info)
        return;

    InputEvent event;

    event.type = kInputEventKeyDown;
    event.value.key.sym = kGamepadA + buttonID;

    PostEvent(&event);
}

#ifdef SOKOL_DISABLED
static void HandleGamepadTriggerEvent(SDL_Event *ev)
{
    // ignore other gamepads
    if (ev->caxis.which != current_gamepad)
        return;

    Uint8 current_axis = ev->caxis.axis;

    // ignore non-trigger axes
    if (current_axis != SDL_CONTROLLER_AXIS_TRIGGERLEFT && current_axis != SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
        return;

    InputEvent event;

    int thresh = RoundToInteger(*joystick_deadzones[current_axis] * 32767.0f);
    int input  = ev->caxis.value;

    if (current_axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
    {
        event.value.key.sym = kGamepadTriggerLeft;
        if (input < thresh)
        {
            if (!left_trigger_pulled)
                return;
            event.type          = kInputEventKeyUp;
            left_trigger_pulled = false;
        }
        else
        {
            if (left_trigger_pulled)
                return;
            event.type          = kInputEventKeyDown;
            left_trigger_pulled = true;
        }
    }
    else
    {
        event.value.key.sym = kGamepadTriggerRight;
        if (input < thresh)
        {
            if (!right_trigger_pulled)
                return;
            event.type           = kInputEventKeyUp;
            right_trigger_pulled = false;
        }
        else
        {
            if (right_trigger_pulled)
                return;
            event.type           = kInputEventKeyDown;
            right_trigger_pulled = true;
        }
    }

    PostEvent(&event);
}
#endif

void HandleMouseMotionEvent(sapp_event *ev)
{
    int dx, dy;

    dx = ev->mouse_dx;
    dy = ev->mouse_dy;

    if (dx || dy)
    {
        InputEvent event;

        event.type           = kInputEventKeyMouse;
        event.value.mouse.dx = dx;
        event.value.mouse.dy = -dy; // -AJA- positive should be "up"

        PostEvent(&event);
    }
}

int JoystickGetAxis(int n)
{
    if (no_joystick || !gamepad_info)
        return 0;

    if (n < gamepad_info->numAxes)
        return gamepad_info->axisStates[n];
    else
        return 0;
}

static void I_OpenJoystick(Gamepad_device *joystick)
{
    gamepad_info = joystick;

    const char *name = gamepad_info->description;
    if (!name)
        name = "(UNKNOWN)";

    int gp_total_joysticksticks = 0;
    int gp_num_triggers         = 0;

    // Until something smarter is sorted out, assume that any gamepad is either going to have
    // one thumbstick, two thumbsticks, or two thumbstick + 2 analog triggers - Dasho
    if (gamepad_info->numAxes == 2)
        gp_total_joysticksticks = 1;
    else if (gamepad_info->numAxes == 4 || gamepad_info->numAxes == 6)
        gp_total_joysticksticks = 2;
    if (gamepad_info->numAxes == 6)
        gp_num_triggers = 2;
    
    int gp_num_buttons = gamepad_info->numButtons;

    LogPrint("Opened gamepad: %s\n", name);
    LogPrint("Sticks:%d Triggers: %d Buttons: %d\n", gp_total_joysticksticks, gp_num_triggers,
             gp_num_buttons);
}

static void JoystickPlugCallback(Gamepad_device *device, void *context)
{
    (void)context;
    EPI_ASSERT(device);
    gamepad_info = nullptr;
    I_OpenJoystick(Gamepad_deviceAtIndex(0));
}

static void JoystickUnplugCallback(Gamepad_device *device, void *context)
{
    (void)context;
    EPI_ASSERT(device);
    gamepad_info = nullptr;
    if (Gamepad_numDevices() > 0)
        I_OpenJoystick(Gamepad_deviceAtIndex(0));
}

//
// Event handling while the application is active
//
void ActiveEventProcess(sapp_event *ev)
{
    bool mouse_locked = sapp_mouse_locked();

    switch (ev->type)
    {
    case SAPP_EVENTTYPE_UNFOCUSED: {
        HandleFocusLost();
        break;
    }

    case SAPP_EVENTTYPE_CHAR:
        HandleCharEvent(ev);
        break;

    case SAPP_EVENTTYPE_KEY_DOWN:
    case SAPP_EVENTTYPE_KEY_UP:
        HandleKeyEvent(ev);
        break;

    case SAPP_EVENTTYPE_MOUSE_DOWN:
        if (!mouse_locked)
        {
            GrabCursor(true);
        }
    case SAPP_EVENTTYPE_MOUSE_UP:
        if (mouse_locked)
        {
            HandleMouseButtonEvent(ev);
        }

        break;

    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        if (mouse_locked)
        {
            HandleMouseWheelEvent(ev);
        }
        break;

    case SAPP_EVENTTYPE_MOUSE_MOVE:
        if (mouse_locked)
        {
            HandleMouseMotionEvent(ev);
        }
        break;

    case SAPP_EVENTTYPE_QUIT_REQUESTED:
        // Note we deliberate clear all other flags here. Its our method of
        // ensuring nothing more is done with events.
        app_state = kApplicationPendingQuit;
        break;

    default:
        break; // Don't care
    }
}

//
// Event handling while the application is not active
//
void InactiveEventProcess(sapp_event *ev)
{
    switch (ev->type)
    {
    case SAPP_EVENTTYPE_FOCUSED:
        if (app_state & kApplicationPendingQuit)
            break; // Don't care: we're going to exit
        HandleFocusGain();
        break;

    case SAPP_EVENTTYPE_QUIT_REQUESTED:
        // Note we deliberate clear all other flags here. Its our method of
        // ensuring nothing more is done with events.
        app_state = kApplicationPendingQuit;
        break;

    default:
        break; // Don't care
    }
}

void I_ShowGamepads(void)
{
    if (no_joystick)
    {
        LogPrint("Gamepad system is disabled.\n");
        return;
    }

    if (!gamepad_info)
    {
        LogPrint("No gamepads found.\n");
        return;
    }

    LogPrint("Gamepads:\n");

    const char *name = gamepad_info->description;
    if (!name)
        name = "(UNKNOWN)";

    LogPrint("%s\n", name);
}

void StartupJoystick(void)
{
    if (FindArgument("no_joystick") > 0)
    {
        LogPrint("StartupControl: Gamepad system disabled.\n");
        no_joystick = true;
        return;
    }

    Gamepad_init();

    int total_joysticks = Gamepad_numDevices();

    LogPrint("StartupControl: %d gamepads found.\n", total_joysticks);

    Gamepad_deviceAttachFunc(JoystickPlugCallback, nullptr);
    Gamepad_deviceRemoveFunc(JoystickUnplugCallback, nullptr);
    Gamepad_buttonDownFunc(HandleGamepadButtonPress, nullptr);
    Gamepad_buttonUpFunc(HandleGamepadButtonRelease, nullptr);

    if (total_joysticks == 0)
        return;
    else
        I_OpenJoystick(Gamepad_deviceAtIndex(0));
}

/****** Input Event Generation ******/

void StartupControl(void)
{
    s_control_events.reserve(4096);

    StartupJoystick();
}

void ControlPostEvent(const sapp_event &event)
{
    s_control_events.emplace_back(event);
}

void ControlGetEvents(void)
{
    EDGE_ZoneScoped;


// Check for plugs/unplugs; not sure if being before or after
// processEvents matters - Dasho
    Gamepad_detectDevices();

    Gamepad_processEvents();

    for (size_t i = 0; i < s_control_events.size(); i++)
    {
        sapp_event &event = s_control_events[i];

        if (app_state & kApplicationActive)
        {
            ActiveEventProcess(&event);
        }
        else
        {
            InactiveEventProcess(&event);
        }
    }

    s_control_events.clear();
}

void ShutdownControl(void)
{
    Gamepad_shutdown();
}

int GetTime(void)
{
    uint32_t t = (uint32_t)stm_ms(stm_now());

    int factor = (double_framerate.d_ ? 70 : 35);

    // more complex than "t*70/1000" to give more accuracy
    return (t / 1000) * factor + (t % 1000) * factor / 1000;
}

int GetMilliseconds(void)
{
    return (int)stm_ms(stm_now());
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
