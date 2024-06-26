// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 5cec40df1dc7e08ac018a55f000d3d23ff112e87 $
//
// Copyright (C) 2012 by Alex Mayfield.
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
//  Serverside maplist-related functionality.
//
//-----------------------------------------------------------------------------

#pragma once

#include <c_maplist.h>
#include <d_player.h>

#include <map>

// Serverside maplist structure
class Maplist
{
  private:
    bool                         entered_once;
    std::string                  error;
    size_t                       index;
    bool                         in_maplist;
    std::vector<maplist_entry_t> maplist;
    bool                         shuffled;
    size_t                       s_index;
    std::vector<size_t>          s_maplist;
    std::map<int32_t, uint64_t>  timeout;
    uint8_t                      version;
    void                         shuffle(void);
    void                         update_shuffle_index(void);

    maplist_entry_t lobbymap;

  public:
    Maplist() : entered_once(false), error(""), index(0), in_maplist(false), shuffled(false), s_index(0), version(0) {};
    static Maplist &instance(void);
    // Modifiers
    bool add(maplist_entry_t &maplist_entry);
    bool insert(const size_t &position, maplist_entry_t &maplist_entry);
    bool remove(const size_t &position);
    bool clear(void);

    // Elements
    bool        empty(void);
    std::string get_error(void);
    bool        get_map_by_index(const size_t &index, maplist_entry_t &maplist_entry);
    bool        get_next_index(size_t &index);
    bool        get_this_index(size_t &index);
    uint8_t     get_version(void);
    bool        query(std::vector<std::pair<size_t, maplist_entry_t *>> &result);
    bool        query(const std::vector<std::string> &query, std::vector<std::pair<size_t, maplist_entry_t *>> &result);
    // Settings
    bool set_index(const size_t &index);
    void set_shuffle(const bool setting);
    // Timeout
    bool pid_timeout(const int32_t index);
    bool pid_cached(const int32_t index);
    void set_timeout(const int32_t index);
    void clear_timeout(const int32_t index);

    // Lobby
    void            set_lobbymap(maplist_entry_t map);
    maplist_entry_t get_lobbymap();
    void            clear_lobbymap();
    bool            lobbyempty();
};

void SV_Maplist(player_t &player);
void SV_MaplistUpdate(player_t &player);

void Maplist_Disconnect(player_t &player);

bool CMD_Randmap(std::string &error);
