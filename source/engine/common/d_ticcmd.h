// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 4e5dad54c1fb71bbeda0a9d2f833a86f0c51e66f $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------

#pragma once

#include "farchive.h"

// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.
struct ticcmd_t
{
  private:
    static void readByte(std::string::const_iterator it, uint8_t &b)
    {
        b = static_cast<uint8_t>(*it);
    }

    static void readShort(std::string::const_iterator it, int16_t &s)
    {
        s = static_cast<uint8_t>(*it);
        s |= static_cast<uint8_t>(*(it + 1)) << 8;
    }

    static void writeByte(std::string::iterator it, uint8_t b)
    {
        *it = b;
    }

    static void writeShort(std::string::iterator it, int16_t s)
    {
        *it       = s & 0xFF;
        *(it + 1) = s >> 8;
    }

  public:
    static const size_t SERIALIZED_SIZE = 2 + sizeof(int16_t) * 5;

    ticcmd_t()
    {
        clear();
    }

    void clear()
    {
        buttons     = 0;
        pitch       = 0;
        yaw         = 0;
        forwardmove = 0;
        sidemove    = 0;
        upmove      = 0;
        impulse     = 0;
    }

    void serialize(std::string &out)
    {
        out.resize(SERIALIZED_SIZE);
        writeByte(out.begin(), buttons);
        writeShort(out.begin() + 1, pitch);
        writeShort(out.begin() + 3, yaw);
        writeShort(out.begin() + 5, forwardmove);
        writeShort(out.begin() + 7, sidemove);
        writeShort(out.begin() + 9, upmove);
        writeByte(out.begin() + 11, impulse);
    }

    void unserialize(const std::string &in)
    {
        if (in.size() != SERIALIZED_SIZE)
            return;
        readByte(in.begin(), buttons);
        readShort(in.begin() + 1, pitch);
        readShort(in.begin() + 3, yaw);
        readShort(in.begin() + 5, forwardmove);
        readShort(in.begin() + 7, sidemove);
        readShort(in.begin() + 9, upmove);
        readByte(in.begin() + 11, impulse);
    }

    int32_t tic;   // the client's tic when this cmd was sent

    uint8_t buttons;
    int16_t pitch; // up/down. currently just a y-sheering amount
    int16_t yaw;   // left/right
    int16_t forwardmove;
    int16_t sidemove;
    int16_t upmove;
    uint8_t impulse;
};

#define UCMDF_BUTTONS     0x01
#define UCMDF_PITCH       0x02
#define UCMDF_YAW         0x04
#define UCMDF_FORWARDMOVE 0x08
#define UCMDF_SIDEMOVE    0x10
#define UCMDF_UPMOVE      0x20
#define UCMDF_IMPULSE     0x40

inline FArchive &operator<<(FArchive &arc, ticcmd_t &cmd)
{
    uint8_t buf[256], *ptr = buf;

    uint8_t flags = 0;

    if (cmd.buttons)
    {
        flags |= UCMDF_BUTTONS;
        *ptr++ = cmd.buttons;
    }
    if (cmd.pitch)
    {
        flags |= UCMDF_PITCH;
        *ptr++ = cmd.pitch >> 8;
        *ptr++ = cmd.pitch & 0xFF;
    }
    if (cmd.yaw)
    {
        flags |= UCMDF_YAW;
        *ptr++ = cmd.yaw >> 8;
        *ptr++ = cmd.yaw & 0xFF;
    }
    if (cmd.forwardmove)
    {
        flags |= UCMDF_FORWARDMOVE;
        *ptr++ = cmd.forwardmove >> 8;
        *ptr++ = cmd.forwardmove & 0xFF;
    }
    if (cmd.sidemove)
    {
        flags |= UCMDF_SIDEMOVE;
        *ptr++ = cmd.sidemove >> 8;
        *ptr++ = cmd.sidemove & 0xFF;
    }
    if (cmd.upmove)
    {
        flags |= UCMDF_UPMOVE;
        *ptr++ = cmd.upmove >> 8;
        *ptr++ = cmd.upmove & 0xFF;
    }
    if (cmd.impulse)
    {
        flags |= UCMDF_IMPULSE;
        *ptr++ = cmd.impulse;
    }

    uint8_t len = ptr - buf;
    arc << (uint8_t)(len + 1) << flags;
    arc.Write(buf, len);

    return arc;
}

inline FArchive &operator>>(FArchive &arc, ticcmd_t &cmd)
{
    uint8_t buf[256], *ptr = buf;

    uint8_t len, flags;
    arc >> len >> flags;
    arc.Read(buf, len - 1);

    // make sure the ucmd is empty
    cmd.clear();

    if (flags & UCMDF_BUTTONS)
    {
        cmd.buttons = *ptr++;
    }
    if (flags & UCMDF_PITCH)
    {
        cmd.pitch = *ptr++ << 8;
        cmd.pitch |= *ptr++;
    }
    if (flags & UCMDF_YAW)
    {
        cmd.pitch = *ptr++ << 8;
        cmd.pitch |= *ptr++;
    }
    if (flags & UCMDF_FORWARDMOVE)
    {
        cmd.forwardmove = *ptr++ << 8;
        cmd.forwardmove |= *ptr++;
    }
    if (flags & UCMDF_SIDEMOVE)
    {
        cmd.sidemove = *ptr++ << 8;
        cmd.sidemove |= *ptr++;
    }
    if (flags & UCMDF_UPMOVE)
    {
        cmd.upmove = *ptr++ << 8;
        cmd.upmove |= *ptr++;
    }
    if (flags & UCMDF_IMPULSE)
    {
        cmd.impulse = *ptr++;
    }

    return arc;
}
