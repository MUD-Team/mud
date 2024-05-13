// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
//
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
//  UIInput module.
//
//-----------------------------------------------------------------------------

#include "ui_input.h"
#include "ui_playerview.h"

#include <RmlUi/Core/Core.h>

#include "../sdl/i_input.h"

static Rml::Input::KeyIdentifier ConvertKey(int32_t sdlkey);
static int32_t                       ConvertMouseButton(int32_t button);
static int32_t                       GetKeyModifierState();



UIInput::UIInput()
{
    mSDLEvents.reserve(16384);
}

void UIInput::postEvent(SDL_Event &ev)
{
    mSDLEvents.push_back(ev);
}

void UIInput::processEvents()
{
    Rml::Context *context = Rml::GetContext("play");
    if (!context)
    {
        mSDLEvents.clear();
        return;
    }

    Rml::Element *focus = context->GetFocusElement();
	
    if (focus && rmlui_dynamic_cast<ElementPlayerView *>(focus))
    {
		event_t mouse_move_event(ev_mouse);
        for (auto ev : mSDLEvents)
        {
            event_t event;
            if (I_TranslateSDLEvent(ev, event))
            {	
				// consolidate mouse move events to a single event		
				if (event.type == ev_mouse && (event.data2 || event.data3))
				{
					mouse_move_event.data2 += event.data2;
					mouse_move_event.data3 += event.data3;
				}
				else
				{
					I_PostInputEvent(event);
				}				
            }
        }		

		if (mouse_move_event.data2 || mouse_move_event.data3)
		{
			I_PostInputEvent(mouse_move_event);
		}

		I_HandleInputEvents();		
    }

    for (auto ev : mSDLEvents)
    {
        switch (ev.type)
        {
        case SDL_MOUSEMOTION:
            context->ProcessMouseMove(ev.motion.x, ev.motion.y, GetKeyModifierState());
            break;
        case SDL_MOUSEBUTTONDOWN:
            context->ProcessMouseButtonDown(ConvertMouseButton(ev.button.button), GetKeyModifierState());
            SDL_CaptureMouse(SDL_TRUE);
            break;
        case SDL_MOUSEBUTTONUP:
            SDL_CaptureMouse(SDL_FALSE);
            context->ProcessMouseButtonUp(ConvertMouseButton(ev.button.button), GetKeyModifierState());
            break;
        case SDL_MOUSEWHEEL:
            context->ProcessMouseWheel(float(-ev.wheel.y), GetKeyModifierState());
            break;
        case SDL_KEYDOWN:
            context->ProcessKeyDown(ConvertKey(ev.key.keysym.sym), GetKeyModifierState());
            break;
        case SDL_KEYUP:
            context->ProcessKeyUp(ConvertKey(ev.key.keysym.sym), GetKeyModifierState());
            break;
        case SDL_TEXTINPUT:
            context->ProcessTextInput(Rml::String(&ev.text.text[0]));
            break;
        case SDL_WINDOWEVENT: {
            switch (ev.window.event)
            {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                Rml::Vector2i dimensions(ev.window.data1, ev.window.data2);
                context->SetDimensions(dimensions);
            }
            break;
            case SDL_WINDOWEVENT_LEAVE:
                context->ProcessMouseLeave();
                break;
            }
        }
        break;
        default:
            break;
        }
    }

    mSDLEvents.clear();
}

Rml::Input::KeyIdentifier ConvertKey(int32_t sdlkey)
{
    // clang-format off
	switch (sdlkey)
	{
	case SDLK_UNKNOWN:      return Rml::Input::KI_UNKNOWN;
	case SDLK_ESCAPE:       return Rml::Input::KI_ESCAPE;
	case SDLK_SPACE:        return Rml::Input::KI_SPACE;
	case SDLK_0:            return Rml::Input::KI_0;
	case SDLK_1:            return Rml::Input::KI_1;
	case SDLK_2:            return Rml::Input::KI_2;
	case SDLK_3:            return Rml::Input::KI_3;
	case SDLK_4:            return Rml::Input::KI_4;
	case SDLK_5:            return Rml::Input::KI_5;
	case SDLK_6:            return Rml::Input::KI_6;
	case SDLK_7:            return Rml::Input::KI_7;
	case SDLK_8:            return Rml::Input::KI_8;
	case SDLK_9:            return Rml::Input::KI_9;
	case SDLK_a:            return Rml::Input::KI_A;
	case SDLK_b:            return Rml::Input::KI_B;
	case SDLK_c:            return Rml::Input::KI_C;
	case SDLK_d:            return Rml::Input::KI_D;
	case SDLK_e:            return Rml::Input::KI_E;
	case SDLK_f:            return Rml::Input::KI_F;
	case SDLK_g:            return Rml::Input::KI_G;
	case SDLK_h:            return Rml::Input::KI_H;
	case SDLK_i:            return Rml::Input::KI_I;
	case SDLK_j:            return Rml::Input::KI_J;
	case SDLK_k:            return Rml::Input::KI_K;
	case SDLK_l:            return Rml::Input::KI_L;
	case SDLK_m:            return Rml::Input::KI_M;
	case SDLK_n:            return Rml::Input::KI_N;
	case SDLK_o:            return Rml::Input::KI_O;
	case SDLK_p:            return Rml::Input::KI_P;
	case SDLK_q:            return Rml::Input::KI_Q;
	case SDLK_r:            return Rml::Input::KI_R;
	case SDLK_s:            return Rml::Input::KI_S;
	case SDLK_t:            return Rml::Input::KI_T;
	case SDLK_u:            return Rml::Input::KI_U;
	case SDLK_v:            return Rml::Input::KI_V;
	case SDLK_w:            return Rml::Input::KI_W;
	case SDLK_x:            return Rml::Input::KI_X;
	case SDLK_y:            return Rml::Input::KI_Y;
	case SDLK_z:            return Rml::Input::KI_Z;
	case SDLK_SEMICOLON:    return Rml::Input::KI_OEM_1;
	case SDLK_PLUS:         return Rml::Input::KI_OEM_PLUS;
	case SDLK_COMMA:        return Rml::Input::KI_OEM_COMMA;
	case SDLK_MINUS:        return Rml::Input::KI_OEM_MINUS;
	case SDLK_PERIOD:       return Rml::Input::KI_OEM_PERIOD;
	case SDLK_SLASH:        return Rml::Input::KI_OEM_2;
	case SDLK_BACKQUOTE:    return Rml::Input::KI_OEM_3;
	case SDLK_LEFTBRACKET:  return Rml::Input::KI_OEM_4;
	case SDLK_BACKSLASH:    return Rml::Input::KI_OEM_5;
	case SDLK_RIGHTBRACKET: return Rml::Input::KI_OEM_6;
	case SDLK_QUOTEDBL:     return Rml::Input::KI_OEM_7;
	case SDLK_KP_0:         return Rml::Input::KI_NUMPAD0;
	case SDLK_KP_1:         return Rml::Input::KI_NUMPAD1;
	case SDLK_KP_2:         return Rml::Input::KI_NUMPAD2;
	case SDLK_KP_3:         return Rml::Input::KI_NUMPAD3;
	case SDLK_KP_4:         return Rml::Input::KI_NUMPAD4;
	case SDLK_KP_5:         return Rml::Input::KI_NUMPAD5;
	case SDLK_KP_6:         return Rml::Input::KI_NUMPAD6;
	case SDLK_KP_7:         return Rml::Input::KI_NUMPAD7;
	case SDLK_KP_8:         return Rml::Input::KI_NUMPAD8;
	case SDLK_KP_9:         return Rml::Input::KI_NUMPAD9;
	case SDLK_KP_ENTER:     return Rml::Input::KI_NUMPADENTER;
	case SDLK_KP_MULTIPLY:  return Rml::Input::KI_MULTIPLY;
	case SDLK_KP_PLUS:      return Rml::Input::KI_ADD;
	case SDLK_KP_MINUS:     return Rml::Input::KI_SUBTRACT;
	case SDLK_KP_PERIOD:    return Rml::Input::KI_DECIMAL;
	case SDLK_KP_DIVIDE:    return Rml::Input::KI_DIVIDE;
	case SDLK_KP_EQUALS:    return Rml::Input::KI_OEM_NEC_EQUAL;
	case SDLK_BACKSPACE:    return Rml::Input::KI_BACK;
	case SDLK_TAB:          return Rml::Input::KI_TAB;
	case SDLK_CLEAR:        return Rml::Input::KI_CLEAR;
	case SDLK_RETURN:       return Rml::Input::KI_RETURN;
	case SDLK_PAUSE:        return Rml::Input::KI_PAUSE;
	case SDLK_CAPSLOCK:     return Rml::Input::KI_CAPITAL;
	case SDLK_PAGEUP:       return Rml::Input::KI_PRIOR;
	case SDLK_PAGEDOWN:     return Rml::Input::KI_NEXT;
	case SDLK_END:          return Rml::Input::KI_END;
	case SDLK_HOME:         return Rml::Input::KI_HOME;
	case SDLK_LEFT:         return Rml::Input::KI_LEFT;
	case SDLK_UP:           return Rml::Input::KI_UP;
	case SDLK_RIGHT:        return Rml::Input::KI_RIGHT;
	case SDLK_DOWN:         return Rml::Input::KI_DOWN;
	case SDLK_INSERT:       return Rml::Input::KI_INSERT;
	case SDLK_DELETE:       return Rml::Input::KI_DELETE;
	case SDLK_HELP:         return Rml::Input::KI_HELP;
	case SDLK_F1:           return Rml::Input::KI_F1;
	case SDLK_F2:           return Rml::Input::KI_F2;
	case SDLK_F3:           return Rml::Input::KI_F3;
	case SDLK_F4:           return Rml::Input::KI_F4;
	case SDLK_F5:           return Rml::Input::KI_F5;
	case SDLK_F6:           return Rml::Input::KI_F6;
	case SDLK_F7:           return Rml::Input::KI_F7;
	case SDLK_F8:           return Rml::Input::KI_F8;
	case SDLK_F9:           return Rml::Input::KI_F9;
	case SDLK_F10:          return Rml::Input::KI_F10;
	case SDLK_F11:          return Rml::Input::KI_F11;
	case SDLK_F12:          return Rml::Input::KI_F12;
	case SDLK_F13:          return Rml::Input::KI_F13;
	case SDLK_F14:          return Rml::Input::KI_F14;
	case SDLK_F15:          return Rml::Input::KI_F15;
	case SDLK_NUMLOCKCLEAR: return Rml::Input::KI_NUMLOCK;
	case SDLK_SCROLLLOCK:   return Rml::Input::KI_SCROLL;
	case SDLK_LSHIFT:       return Rml::Input::KI_LSHIFT;
	case SDLK_RSHIFT:       return Rml::Input::KI_RSHIFT;
	case SDLK_LCTRL:        return Rml::Input::KI_LCONTROL;
	case SDLK_RCTRL:        return Rml::Input::KI_RCONTROL;
	case SDLK_LALT:         return Rml::Input::KI_LMENU;
	case SDLK_RALT:         return Rml::Input::KI_RMENU;
	case SDLK_LGUI:         return Rml::Input::KI_LMETA;
	case SDLK_RGUI:         return Rml::Input::KI_RMETA;
	/*
	case SDLK_LSUPER:       return Rml::Input::KI_LWIN;
	case SDLK_RSUPER:       return Rml::Input::KI_RWIN;
	*/
	default: break;
	}
    // clang-format on

    return Rml::Input::KI_UNKNOWN;
}

int32_t ConvertMouseButton(int32_t button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT:
        return 0;
    case SDL_BUTTON_RIGHT:
        return 1;
    case SDL_BUTTON_MIDDLE:
        return 2;
    default:
        return 3;
    }
}

int32_t GetKeyModifierState()
{
    SDL_Keymod sdl_mods = SDL_GetModState();

    int32_t retval = 0;

    if (sdl_mods & KMOD_CTRL)
        retval |= Rml::Input::KM_CTRL;

    if (sdl_mods & KMOD_SHIFT)
        retval |= Rml::Input::KM_SHIFT;

    if (sdl_mods & KMOD_ALT)
        retval |= Rml::Input::KM_ALT;

    if (sdl_mods & KMOD_NUM)
        retval |= Rml::Input::KM_NUMLOCK;

    if (sdl_mods & KMOD_CAPS)
        retval |= Rml::Input::KM_CAPSLOCK;

    return retval;
}