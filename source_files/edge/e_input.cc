//----------------------------------------------------------------------------
//  EDGE Input handling
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// -MH- 1998/07/02 Added key_fly_up and key_fly_down variables (no logic yet)
// -MH- 1998/08/18 Flyup and flydown logic
//

#include "e_input.h"

#include "con_var.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_event.h"
#include "e_main.h"
#include "e_player.h"
#include "epi.h"
#include "epi_str_util.h"
#include "m_math.h"
#include "m_misc.h"
#include "r_misc.h"

extern bool ConsoleResponder(InputEvent *ev);
extern bool GameResponder(InputEvent *ev);

extern ConsoleVariable double_framerate;

//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
//
static constexpr uint8_t kMaximumInputEvents = 128;

static InputEvent events[kMaximumInputEvents];
static int        event_head;
static int        event_tail;

//
// controls (have defaults)
//
int key_right;
int key_left;
int key_look_up;
int key_look_down;
int key_look_center;

// -ES- 1999/03/28 Zoom Key
int key_zoom;

int key_up;
int key_down;
int key_strafe_left;
int key_strafe_right;
int key_fire;
int key_use;
int key_strafe;
int key_speed;
int key_autorun;
int key_next_weapon;
int key_previous_weapon;
int key_map;
int key_180;
int key_talk;
int key_console;
int key_mouselook;
int key_second_attack;
int key_reload;
int key_action1;
int key_action2;

// -MH- 1998/07/10 Flying keys
int key_fly_up;
int key_fly_down;

int key_weapons[10];

int key_inventory_previous;
int key_inventory_use;
int key_inventory_next;

int key_third_attack;
int key_fourth_attack;

static int forward_move[2] = {25, 50};
static int side_move[2]    = {24, 40};
static int upward_move[2]  = {20, 30};

static int angle_turn[3]     = {640, 1280, 320}; // + slow turn
static int mouselook_turn[3] = {400, 800, 200};

static constexpr uint8_t kSlowTurnTics = 6;

static constexpr uint16_t kTotalKeys = 512;

enum GameKeyState
{
    kGameKeyDown = 0x01,
    kGameKeyUp   = 0x02
};

static uint8_t game_key_down[kTotalKeys];

static int turn_held;      // for accelerative turning
static int mouselook_held; // for accelerative mlooking

//-------------------------------------------
// -KM-  1998/09/01 Analogue binding
// -ACB- 1998/09/06 Two-stage turning switch
//
int mouse_x_axis;
int mouse_y_axis;

int joystick_axis[4] = {0, 0, 0, 0};

float joy_raw[4];
static float joy_last_raw[4];

// The last one is ignored (kAxisDisable)
static float ball_deltas[6] = {0, 0, 0, 0, 0, 0};
static float joy_forces[6]  = {0, 0, 0, 0, 0, 0};

EDGE_DEFINE_CONSOLE_VARIABLE_CLAMPED(joystick_deadzone_axis_0, "0.30", kConsoleVariableFlagArchive, 0.01f, 0.99f)
EDGE_DEFINE_CONSOLE_VARIABLE_CLAMPED(joystick_deadzone_axis_1, "0.30", kConsoleVariableFlagArchive, 0.01f, 0.99f)
EDGE_DEFINE_CONSOLE_VARIABLE_CLAMPED(joystick_deadzone_axis_2, "0.30", kConsoleVariableFlagArchive, 0.01f, 0.99f)
EDGE_DEFINE_CONSOLE_VARIABLE_CLAMPED(joystick_deadzone_axis_3, "0.30", kConsoleVariableFlagArchive, 0.01f, 0.99f)
EDGE_DEFINE_CONSOLE_VARIABLE_CLAMPED(joystick_deadzone_axis_4, "0.30", kConsoleVariableFlagArchive, 0.01f, 0.99f)
EDGE_DEFINE_CONSOLE_VARIABLE_CLAMPED(joystick_deadzone_axis_5, "0.30", kConsoleVariableFlagArchive, 0.01f, 0.99f)
float *joystick_deadzones[6] = {
    &joystick_deadzone_axis_0.f_, &joystick_deadzone_axis_1.f_, &joystick_deadzone_axis_2.f_,
    &joystick_deadzone_axis_3.f_, &joystick_deadzone_axis_4.f_, &joystick_deadzone_axis_5.f_,
};

EDGE_DEFINE_CONSOLE_VARIABLE(in_running, "1", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(in_stageturn, "1", kConsoleVariableFlagArchive)

EDGE_DEFINE_CONSOLE_VARIABLE(debug_mouse, "0", kConsoleVariableFlagNone)
EDGE_DEFINE_CONSOLE_VARIABLE(debug_joyaxis, "0", kConsoleVariableFlagNone)

EDGE_DEFINE_CONSOLE_VARIABLE(mouse_x_sensitivity, "10.0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(mouse_y_sensitivity, "10.0", kConsoleVariableFlagArchive)

// Speed controls
EDGE_DEFINE_CONSOLE_VARIABLE(turn_speed, "1.0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(vertical_look_speed, "1.0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(forward_speed, "1.0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(side_speed, "1.0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(fly_speed, "1.0", kConsoleVariableFlagArchive)

static void UpdateJoystickAxis(int n)
{
    if (joystick_axis[n] == kAxisDisable)
        return;

    float raw = joy_raw[n];
    float old = joy_last_raw[n];

    joy_last_raw[n] = raw;

    // cooked value = average of last two raw samples
    float force = (raw + old) * 0.5f;

    if (fabs(force) < *joystick_deadzones[n])
        force = 0;

    // perform inversion
    if ((joystick_axis[n] + 1) & 1)
        force = -force;

    if (debug_joyaxis.d_ == n + 1)
        LogPrint("Axis%d : %+7.3f\n", n + 1, force);

    int axis = (joystick_axis[n] + 1) >> 1;

    joy_forces[axis] = force;
}

bool CheckKeyMatch(int keyvar, int key)
{
    return ((keyvar >> 16) == key) || ((keyvar & 0xffff) == key);
}

bool IsKeyPressed(int keyvar)
{
#ifdef DEVELOPERS
    if ((keyvar >> 16) > kTotalKeys)
        FatalError("Invalid key!");
    else if ((keyvar & 0xffff) > kTotalKeys)
        FatalError("Invalid key!");
#endif

    if (game_key_down[keyvar >> 16] & kGameKeyDown)
        return true;

    if (game_key_down[keyvar & 0xffff] & kGameKeyDown)
        return true;

    return false;
}

static inline void AddKeyForce(int axis, int upkeys, int downkeys, float qty = 1.0f)
{
    // let movement keys cancel each other out
    if (IsKeyPressed(upkeys))
    {
        joy_forces[axis] += qty;
    }
    if (IsKeyPressed(downkeys))
    {
        joy_forces[axis] -= qty;
    }
}

static void UpdateForces(void)
{
    for (int k = 0; k < 6; k++)
        joy_forces[k] = 0;

    // ---Joystick---
    for (int j = 0; j < 4; j++)
        UpdateJoystickAxis(j);

    // ---Keyboard---
    AddKeyForce(kAxisTurn, key_right, key_left);
    AddKeyForce(kAxisMouselook, key_look_up, key_look_down);
    AddKeyForce(kAxisForward, key_up, key_down);
    // -MH- 1998/08/18 Fly down
    AddKeyForce(kAxisFly, key_fly_up, key_fly_down);
    AddKeyForce(kAxisStrafe, key_strafe_right, key_strafe_left);
}

//
// BuildEventTicCommand
//
// Builds a ticcmd from all of the available inputs
//
// -ACB- 1998/07/02 Added Vertical angle checking for mlook.
// -ACB- 1998/07/10 Reformatted: I can read the code! :)
// -ACB- 1998/09/06 Apply speed controls to -KM-'s analogue controls
//
static bool allow_180                = true;
static bool allow_zoom               = true;
static bool allow_autorun            = true;
static bool allow_inventory_previous = true;
static bool allow_inventory_use      = true;
static bool allow_inventory_next     = true;

void BuildEventTicCommand(EventTicCommand *cmd)
{
    UpdateForces();

    *cmd = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    bool strafe = IsKeyPressed(key_strafe);
    int  speed  = IsKeyPressed(key_speed) ? 1 : 0;

    if (in_running.d_)
        speed = !speed;

    //
    // -KM- 1998/09/01 use two stage accelerative turning on all devices
    //
    // -ACB- 1998/09/06 Allow stage turning to be switched off for
    //                  analogue devices...
    //
    int t_speed = speed;

    if (fabs(joy_forces[kAxisTurn]) > 0.2f)
        turn_held++;
    else
        turn_held = 0;

    // slow turn ?
    if (turn_held < kSlowTurnTics && in_stageturn.d_)
        t_speed = 2;

    int m_speed = speed;

    if (fabs(joy_forces[kAxisMouselook]) > 0.2f)
        mouselook_held++;
    else
        mouselook_held = 0;

    // slow mlook ?
    if (mouselook_held < kSlowTurnTics && in_stageturn.d_)
        m_speed = 2;

    // Turning
    if (!strafe)
    {
        float turn = angle_turn[t_speed] / (double_framerate.d_ ? 2 : 1) * joy_forces[kAxisTurn];

        turn *= turn_speed.f_;

        // -ACB- 1998/09/06 Angle Turn Speed Control
        turn += angle_turn[t_speed] * ball_deltas[kAxisTurn] / 64.0;

        cmd->angle_turn = RoundToInteger(turn);
    }

    // MLook
    {
        // -ACB- 1998/07/02 Use VertAngle for Look/up down.
        float mlook = mouselook_turn[m_speed] * joy_forces[kAxisMouselook];

        mlook *= vertical_look_speed.f_;

        mlook += mouselook_turn[m_speed] * ball_deltas[kAxisMouselook] / 64.0;

        cmd->mouselook_turn = RoundToInteger(mlook);
    }

    // Forward [ no change for 70Hz ]
    {
        float forward = forward_move[speed] * joy_forces[kAxisForward];

        forward *= forward_speed.f_;

        // -ACB- 1998/09/06 Forward Move Speed Control
        forward += forward_move[speed] * ball_deltas[kAxisForward] / 64.0;

        forward = glm_clamp(-forward_move[1], forward, forward_move[1]);

        cmd->forward_move = RoundToInteger(forward);
    }

    // Sideways [ no change for 70Hz ]
    {
        float side = side_move[speed] * joy_forces[kAxisStrafe];

        if (strafe)
            side += side_move[speed] * joy_forces[kAxisTurn];

        side *= side_speed.f_;

        // -ACB- 1998/09/06 Side Move Speed Control
        side += side_move[speed] * ball_deltas[kAxisStrafe] / 64.0;

        if (strafe)
            side += side_move[speed] * ball_deltas[kAxisTurn] / 64.0;

        side = glm_clamp(-forward_move[1], side, forward_move[1]);

        cmd->side_move = RoundToInteger(side);
    }

    // Upwards  -MH- 1998/08/18 Fly Up/Down movement
    {
        float upward = upward_move[speed] * joy_forces[kAxisFly];

        upward *= fly_speed.f_;

        upward += upward_move[speed] * ball_deltas[kAxisFly] / 64.0;

        upward = glm_clamp(-forward_move[1], upward, forward_move[1]);

        cmd->upward_move = RoundToInteger(upward);
    }

    // ---Buttons---

    if (IsKeyPressed(key_fire))
        cmd->buttons |= kButtonCodeAttack;

    if (IsKeyPressed(key_use) &&
        players[cmd->player_index]->player_state_ != kPlayerAwaitingRespawn) // Prevent passing use action when hitting
                                                                             // 'use' to respawn
        cmd->buttons |= kButtonCodeUse;

    if (IsKeyPressed(key_second_attack))
        cmd->extended_buttons |= kExtendedButtonCodeSecondAttack;

    if (IsKeyPressed(key_third_attack))
        cmd->extended_buttons |= kExtendedButtonCodeThirdAttack;

    if (IsKeyPressed(key_fourth_attack))
        cmd->extended_buttons |= kExtendedButtonCodeFourthAttack;

    if (IsKeyPressed(key_reload))
        cmd->extended_buttons |= kExtendedButtonCodeReload;

    if (IsKeyPressed(key_action1))
        cmd->extended_buttons |= kExtendedButtonCodeAction1;

    if (IsKeyPressed(key_action2))
        cmd->extended_buttons |= kExtendedButtonCodeAction2;

    // -ACB- 1998/07/02 Use CENTER flag to center the vertical look.
    if (IsKeyPressed(key_look_center))
        cmd->extended_buttons |= kExtendedButtonCodeCenter;

    // -KM- 1998/11/25 Weapon change key
    for (int w = 0; w < 10; w++)
    {
        if (IsKeyPressed(key_weapons[w]))
        {
            cmd->buttons |= kButtonCodeChangeWeapon;
            cmd->buttons |= w << kButtonCodeWeaponMaskShift;
            break;
        }
    }

    if (IsKeyPressed(key_next_weapon))
    {
        cmd->buttons |= kButtonCodeChangeWeapon;
        cmd->buttons |= (kButtonCodeNextWeapon << kButtonCodeWeaponMaskShift);
    }
    else if (IsKeyPressed(key_previous_weapon))
    {
        cmd->buttons |= kButtonCodeChangeWeapon;
        cmd->buttons |= (kButtonCodePreviousWeapon << kButtonCodeWeaponMaskShift);
    }

    // You have to release the 180 deg turn key before you can press it again
    if (IsKeyPressed(key_180))
    {
        if (allow_180)
            cmd->angle_turn ^= 0x8000;

        allow_180 = false;
    }
    else
        allow_180 = true;

    // -ES- 1999/03/28 Zoom Key
    if (IsKeyPressed(key_zoom))
    {
        if (allow_zoom)
        {
            cmd->extended_buttons |= kExtendedButtonCodeZoom;
            allow_zoom = false;
        }
    }
    else
        allow_zoom = true;

    // -AJA- 2000/04/14: Autorun toggle
    if (IsKeyPressed(key_autorun))
    {
        if (allow_autorun)
        {
            in_running    = in_running.d_ ? 0 : 1;
            allow_autorun = false;
        }
    }
    else
        allow_autorun = true;

    if (IsKeyPressed(key_inventory_previous))
    {
        if (allow_inventory_previous)
        {
            cmd->extended_buttons |= kExtendedButtonCodeInventoryPrevious;
            allow_inventory_previous = false;
        }
    }
    else
        allow_inventory_previous = true;

    if (IsKeyPressed(key_inventory_use))
    {
        if (allow_inventory_use)
        {
            cmd->extended_buttons |= kExtendedButtonCodeInventoryUse;
            allow_inventory_use = false;
        }
    }
    else
        allow_inventory_use = true;

    if (IsKeyPressed(key_inventory_next))
    {
        if (allow_inventory_next)
        {
            cmd->extended_buttons |= kExtendedButtonCodeInventoryNext;
            allow_inventory_next = false;
        }
    }
    else
        allow_inventory_next = true;

    cmd->chat_character = 0;

    for (int k = 0; k < 6; k++)
        ball_deltas[k] = 0;
}

//
// Get info needed to make ticcmd_ts for the players.
//
bool InputResponder(InputEvent *ev)
{
    switch (ev->type)
    {
    case kInputEventKeyDown:
        if (ev->value.key.sym < kTotalKeys)
        {
            game_key_down[ev->value.key.sym] &= ~kGameKeyUp;
            game_key_down[ev->value.key.sym] |= kGameKeyDown;
        }

        // eat key down events
        return true;

    case kInputEventKeyUp:
        if (ev->value.key.sym < kTotalKeys)
        {
            game_key_down[ev->value.key.sym] |= kGameKeyUp;
        }

        // always let key up events filter down
        return false;

    case kInputEventKeyMouse: {
        float dx = ev->value.mouse.dx;
        float dy = ev->value.mouse.dy;

        // perform inversion
        if ((mouse_x_axis + 1) & 1)
            dx = -dx;
        if ((mouse_y_axis + 1) & 1)
            dy = -dy;

        dx *= mouse_x_sensitivity.f_;
        dy *= mouse_y_sensitivity.f_;

        if (debug_mouse.d_)
            LogPrint("Mouse %+04d %+04d --> %+7.2f %+7.2f\n", ev->value.mouse.dx, ev->value.mouse.dy, dx, dy);

        // -AJA- 1999/07/27: Mlook key like quake's.
        ball_deltas[(mouse_x_axis + 1) >> 1] += dx;
        ball_deltas[(mouse_y_axis + 1) >> 1] += dy;

        return true; // eat events
    }

    default:
        break;
    }

    return false;
}

//
// Sets the turbo scale (100 is normal)
//
void SetTurboScale(int scale)
{
    forward_move[0] = 25 * scale / 100;
    forward_move[1] = 50 * scale / 100;

    side_move[0] = 24 * scale / 100;
    side_move[1] = 40 * scale / 100;
}

void ClearEventInput(void)
{
    std::fill(game_key_down, game_key_down + kTotalKeys, 0);

    turn_held      = 0;
    mouselook_held = 0;
}

//
// Finds all keys in the game_key_down[] array which have been released
// and clears them.  The value is NOT cleared by InputResponder()
// since that prevents very fast presses (also the mousewheel) from being down
// long enough to be noticed by BuildEventTicCommand().
//
// -AJA- 2005/02/17: added this.
//
void UpdateKeyState(void)
{
    for (int k = 0; k < kTotalKeys; k++)
        if (game_key_down[k] & kGameKeyUp)
            game_key_down[k] = 0;
}

//
// Generate events which should release all current keys.
//
void ReleaseAllKeys(void)
{
    int i;
    for (i = 0; i < kTotalKeys; i++)
    {
        if (game_key_down[i] & kGameKeyDown)
        {
            InputEvent ev;

            ev.type          = kInputEventKeyUp;
            ev.value.key.sym = i;

            PostEvent(&ev);
        }
    }
}

//
// Called by the I/O functions when input is detected
//
void PostEvent(InputEvent *ev)
{
    events[event_head] = *ev;
    event_head         = (event_head + 1) % kMaximumInputEvents;

#ifdef EDGE_DEBUG_KEY_EV //!!!!
    if (ev->type == kInputEventKeyDown || ev->type == kInputEventKeyUp)
    {
        LogDebug("EVENT @ %08x %d %s\n", GetMilliseconds(), ev->value.key,
                 (ev->type == kInputEventKeyUp) ? "DOWN" : "up");
    }
#endif
}

//
// Send all the events of the given timestamp down the responder chain
//
void ProcessInputEvents(void)
{
    InputEvent *ev;

    for (; event_tail != event_head; event_tail = (event_tail + 1) % kMaximumInputEvents)
    {
        ev = &events[event_tail];

        if (ConsoleResponder(ev))
            continue;      // Console ate the event

        GameResponder(ev); // let game eat it, nobody else wanted it
    }
}

//----------------------------------------------------------------------------

struct EventSpecialKey
{
    int key;

    const char *name;
};

static EventSpecialKey special_keys[] = {{kRightArrow, "Right Arrow"},
                                         {kLeftArrow, "Left Arrow"},
                                         {kUpArrow, "Up Arrow"},
                                         {kDownArrow, "Down Arrow"},
                                         {kEscape, "Escape"},
                                         {kEnter, "Enter"},
                                         {kTab, "Tab"},

                                         {kBackspace, "Backspace"},
                                         {kEquals, "Equals"},
                                         {kMinus, "Minus"},
                                         {kRightShift, "Shift"},
                                         {kRightControl, "Ctrl"},
                                         {kRightAlt, "Alt"},
                                         {kInsert, "Insert"},
                                         {kDelete, "Delete"},
                                         {kPageDown, "PageDown"},
                                         {kPageUp, "PageUp"},
                                         {kHome, "Home"},
                                         {kEnd, "End"},
                                         {kScrollLock, "ScrollLock"},
                                         {kNumberLock, "NumLock"},
                                         {kCapsLock, "CapsLock"},
                                         {kEnd, "End"},
                                         {'\'', "\'"},
                                         {kSpace, "Space"},
                                         {kTilde, "`"},
                                         {kPause, "Pause"},

                                         // function keys
                                         {kFunction1, "F1"},
                                         {kFunction2, "F2"},
                                         {kFunction3, "F3"},
                                         {kFunction4, "F4"},
                                         {kFunction5, "F5"},
                                         {kFunction6, "F6"},
                                         {kFunction7, "F7"},
                                         {kFunction8, "F8"},
                                         {kFunction9, "F9"},
                                         {kFunction10, "F10"},
                                         {kFunction11, "F11"},
                                         {kFunction12, "F12"},

                                         // numeric keypad
                                         {kKeypad0, "KP_0"},
                                         {kKeypad1, "KP_1"},
                                         {kKeypad2, "KP_2"},
                                         {kKeypad3, "KP_3"},
                                         {kKeypad4, "KP_4"},
                                         {kKeypad5, "KP_5"},
                                         {kKeypad6, "KP_6"},
                                         {kKeypad7, "KP_7"},
                                         {kKeypad8, "KP_8"},
                                         {kKeypad9, "KP_9"},

                                         {kKeypadDot, "KP_DOT"},
                                         {kKeypadPlus, "KP_PLUS"},
                                         {kKeypadMinus, "KP_MINUS"},
                                         {kKeypadStar, "KP_STAR"},
                                         {kKeypadSlash, "KP_SLASH"},
                                         {kKeypadEquals, "KP_EQUAL"},
                                         {kKeypadEnter, "KP_ENTER"},

                                         // mouse buttons
                                         {kMouse1, "Mouse1"},
                                         {kMouse2, "Mouse2"},
                                         {kMouse3, "Mouse3"},
                                         {kMouse4, "Mouse4"},
                                         {kMouse5, "Mouse5"},
                                         {kMouse6, "Mouse6"},
                                         {kMouseWheelUp, "Wheel Up"},
                                         {kMouseWheelDown, "Wheel Down"},

                                         // gamepad buttons
                                         {kGamepadSouth, "A Button"},
                                         {kGamepadEast, "B Button"},
                                         {kGamepadWest, "X Button"},
                                         {kGamepadNorth, "Y Button"},
                                         {kGamepadBack, "Back Button"},
                                         {kGamepadGuide, "Guide Button"},
                                         {kGamepadStart, "Start Button"},
                                         {kGamepadLeftStick, "Left Stick"},
                                         {kGamepadRightStick, "Right Stick"},
                                         {kGamepadLeftShoulder, "Left Shoulder"},
                                         {kGamepadRightShoulder, "Right Shoulder"},
                                         {kGamepadUp, "DPad Up"},
                                         {kGamepadDown, "DPad Down"},
                                         {kGamepadLeft, "DPad Left"},
                                         {kGamepadRight, "DPad Right"},
                                         {kGamepadLeftTrigger, "Left Trigger"},
                                         {kGamepadRightTrigger, "Right Trigger"},

                                         // THE END
                                         {-1, nullptr}};

const char *GetKeyName(int key)
{
    static char buffer[32];

    if (epi::ToUpperASCII(key) >= ',' && epi::ToUpperASCII(key) <= ']')
    {
        buffer[0] = key;
        buffer[1] = 0;

        return buffer;
    }

    for (int i = 0; special_keys[i].name; i++)
    {
        if (special_keys[i].key == key)
            return special_keys[i].name;
    }

    sprintf(buffer, "Key%03d", key);

    return buffer;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
