// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ce9a0a7dd4ad005aefbfac06bcccd56c9fb65eb7 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	C_BIND
//
//-----------------------------------------------------------------------------

#pragma once

#include "d_event.h"
#include "hashtable.h"
#include "physfs.h"

struct OBinding
{
    const char *Key;
    const char *Bind;
};

class OKeyBindings
{
  private:
    typedef OHashTable<int32_t, std::string> BindingTable;

  public:
    BindingTable Binds;
    std::string  command;

    void SetBindingType(std::string cmd);
    void SetBinds(const OBinding *binds);
    void BindAKey(size_t argc, char **argv, const char *msg);
    void DoBind(const char *key, const char *bind);

    void UnbindKey(const char *key);
    void UnbindACommand(const char *str);
    void UnbindAll();

    void ChangeBinding(const char *str, int32_t newone); // Stuff used by the customize controls menu

    const std::string &GetBind(int32_t key);             // Returns string bound to given key (NULL if none)
    std::string        GetNameKeys(int32_t first, int32_t second);
    int32_t                GetKeysForCommand(const char *cmd, int32_t *first, int32_t *second);
    std::string        GetKeynameFromCommand(const char *cmd, bool bTwoEntries = false);

    void ArchiveBindings(PHYSFS_File *f);
};

void C_BindingsInit();
void C_BindDefaults();

// DoKey now have a binding responder, used to switch between Binds
bool C_DoKey(event_t *ev, OKeyBindings *binds, OKeyBindings *doublebinds);

void C_ReleaseKeys();

extern OKeyBindings Bindings, DoubleBindings;
