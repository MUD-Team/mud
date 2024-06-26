// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 8da7d4a1a806d91eee8bff97e94f60bfa16250ce $
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
//  Serverside "pickup" functionality.  Used to distribute players
//  between teams.
//
//-----------------------------------------------------------------------------

#pragma once

bool Pickup_DistributePlayers(size_t num_players, std::string &error);
bool CMD_RandpickupCheck(const std::vector<std::string> &args, std::string &error, size_t &num_players);
