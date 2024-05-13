// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 397681164b2fb8ef3eadabded7dde1af49c8bad8 $
//
// Copyright (C) 2006-2019 by The Odamex Team.
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
//	Common descriptors for commands on the menu.
//  That way, responders will be way cleaner depending on what platform they are.
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdint.h>

// Movement Keys
bool Key_IsUpKey(int32_t key, bool numlock);
bool Key_IsDownKey(int32_t key, bool numlock);
bool Key_IsLeftKey(int32_t key, bool numlock);
bool Key_IsRightKey(int32_t key, bool numlock);

bool Key_IsPageUpKey(int32_t key, bool numlock);
bool Key_IsPageDownKey(int32_t key, bool numlock);
bool Key_IsHomeKey(int32_t key, bool numlock);
bool Key_IsEndKey(int32_t key, bool numlock);
bool Key_IsInsKey(int32_t key, bool numlock);
bool Key_IsDelKey(int32_t key, bool numlock);

bool Key_IsAcceptKey(int32_t key);
bool Key_IsCancelKey(int32_t key);
bool Key_IsMenuKey(int32_t key);
bool Key_IsYesKey(int32_t key);
bool Key_IsNoKey(int32_t key);
bool Key_IsUnbindKey(int32_t key);

bool Key_IsSpyPrevKey(int32_t key);
bool Key_IsSpyNextKey(int32_t key);

bool Key_IsTabulationKey(int32_t key);
