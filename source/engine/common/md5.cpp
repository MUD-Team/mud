// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: c0031663dc384a51beef9e93f3730982fb88e2b6 $
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
//     Interface to Poco MD5Engine.
//
//-----------------------------------------------------------------------------

#include "md5.h"

#include "Poco/MD5Engine.h"
#include "version.h"

static Poco::MD5Engine hasher;

std::string MD5SUM(const void *in, size_t size)
{
    hasher.reset();

    hasher.update(in, size);

    return Poco::MD5Engine::digestToHex(hasher.digest());
}

std::string MD5SUM(std::string in)
{
    hasher.reset();

    hasher.update(in);

    return Poco::MD5Engine::digestToHex(hasher.digest());
}

VERSION_CONTROL(md5_cpp, "$Id: 7bb3d8b62cd516da2a7036dd930ce4a952065c70 $")
