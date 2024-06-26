// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: bbdd3a5002917c5a1c495bfa2cf8f2e4868a3389 $
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
//	Store and serialize input commands between client and server
//
//-----------------------------------------------------------------------------

#include "d_netcmd.h"

#include "d_player.h"
#include "mud_includes.h"

NetCommand::NetCommand()
{
    clear();
}

void NetCommand::clear()
{
    mFields = mTic = mWorldIndex = 0;
    mButtons = mAngle = mPitch = mForwardMove = mSideMove = mUpMove = mImpulse = 0;
    mDeltaYaw = mDeltaPitch = 0;
}

void NetCommand::fromPlayer(player_t *player)
{
    if (!player || !player->mo)
        return;

    clear();
    setTic(player->cmd.tic);

    setButtons(player->cmd.buttons);
    setImpulse(player->cmd.impulse);

    if (player->playerstate != PST_DEAD)
    {
        setAngle(player->mo->angle);
        setPitch(player->mo->pitch);
        setForwardMove(player->cmd.forwardmove);
        setSideMove(player->cmd.sidemove);
        setUpMove(player->cmd.upmove);
        setDeltaYaw(player->cmd.yaw);
        setDeltaPitch(player->cmd.pitch);
    }
}

void NetCommand::toPlayer(player_t *player) const
{
    if (!player || !player->mo)
        return;

    player->cmd.clear();
    player->cmd.tic = getTic();

    player->cmd.buttons = getButtons();
    player->cmd.impulse = getImpulse();

    if (player->playerstate != PST_DEAD)
    {
        player->cmd.forwardmove = getForwardMove();
        player->cmd.sidemove    = getSideMove();
        player->cmd.upmove      = getUpMove();
        player->cmd.yaw         = getDeltaYaw();
        player->cmd.pitch       = getDeltaPitch();

        player->mo->angle = getAngle();
        player->mo->pitch = getPitch();
    }
}

void NetCommand::write(buf_t *buf)
{
    // Let the recipient know which cmd fields are being sent
    int32_t serialized_fields = getSerializedFields();
    buf->WriteByte(serialized_fields);
    buf->WriteLong(mWorldIndex);

    if (serialized_fields & CMD_BUTTONS)
        buf->WriteByte(mButtons);
    if (serialized_fields & CMD_ANGLE)
        buf->WriteShort((mAngle >> FRACBITS) + mDeltaYaw);
    if (serialized_fields & CMD_PITCH)
    {
        // ZDoom uses a hack to center the view when toggling cl_mouselook
        bool centerview = (mDeltaPitch == CENTERVIEW);
        if (centerview)
            buf->WriteShort(0);
        else
            buf->WriteShort((mPitch >> FRACBITS) + mDeltaPitch);
    }
    if (serialized_fields & CMD_FORWARD)
        buf->WriteShort(mForwardMove);
    if (serialized_fields & CMD_SIDE)
        buf->WriteShort(mSideMove);
    if (serialized_fields & CMD_UP)
        buf->WriteShort(mUpMove);
    if (serialized_fields & CMD_IMPULSE)
        buf->WriteByte(mImpulse);
}

void NetCommand::read(buf_t *buf)
{
    clear();
    mFields     = buf->ReadByte();
    mWorldIndex = buf->ReadLong();

    if (hasButtons())
        mButtons = buf->ReadByte();
    if (hasAngle())
        mAngle = buf->ReadShort() << FRACBITS;
    if (hasPitch())
        mPitch = buf->ReadShort() << FRACBITS;
    if (hasForwardMove())
        mForwardMove = buf->ReadShort();
    if (hasSideMove())
        mSideMove = buf->ReadShort();
    if (hasUpMove())
        mUpMove = buf->ReadShort();
    if (hasImpulse())
        mImpulse = buf->ReadByte();
}

int32_t NetCommand::getSerializedFields()
{
    int32_t serialized_fields = 0;

    if (hasButtons())
        serialized_fields |= CMD_BUTTONS;
    if (hasAngle() || hasDeltaYaw())
        serialized_fields |= CMD_ANGLE;
    if (hasPitch() || hasDeltaPitch())
        serialized_fields |= CMD_PITCH;
    if (hasForwardMove())
        serialized_fields |= CMD_FORWARD;
    if (hasSideMove())
        serialized_fields |= CMD_SIDE;
    if (hasUpMove())
        serialized_fields |= CMD_UP;
    if (hasImpulse())
        serialized_fields |= CMD_IMPULSE;

    return serialized_fields;
}

VERSION_CONTROL(d_netcmd_cpp, "$Id: bbdd3a5002917c5a1c495bfa2cf8f2e4868a3389 $")
