// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 04d48ae09090f3250dbe62d9e63f21bf1bfa7387 $
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
//		Plats (i.e. elevator platforms) code, raising/lowering.
//
//-----------------------------------------------------------------------------

#include "m_random.h"
#include "mud_includes.h"
#include "p_lnspec.h"
#include "p_local.h"
#include "s_sound.h"

// From sv_main.cpp
void SV_BroadcastSector(int32_t sectornum);

extern bool predicting;

void P_SetPlatDestroy(DPlat *plat)
{
    if (!plat)
        return;

    plat->m_Status = DPlat::destroy;

    if (clientside && plat->m_Sector)
    {
        plat->m_Sector->floordata = NULL;
        plat->Destroy();
    }
}

IMPLEMENT_SERIAL(DPlat, DMovingFloor)

DPlat::DPlat() : m_Status(init)
{
}

void DPlat::Serialize(FArchive &arc)
{
    Super::Serialize(arc);
    if (arc.IsStoring())
    {
        arc << m_Speed << m_Low << m_High << m_Wait << m_Count << m_Status << m_OldStatus << m_Crush << m_Tag << m_Type
            << m_Height << m_Lip;
    }
    else
    {
        arc >> m_Speed >> m_Low >> m_High >> m_Wait >> m_Count >> m_Status >> m_OldStatus >> m_Crush >> m_Tag >>
            m_Type >> m_Height >> m_Lip;
    }
}

void DPlat::PlayPlatSound()
{
    if (predicting)
        return;

    const char *snd = NULL;
    switch (m_Status)
    {
    case midup:
    case middown:
        snd = "plats/pt1_mid";
        S_LoopedSound(m_Sector->soundorg, CHAN_BODY, snd, 1, ATTN_NORM);
        return;
    case up:
        snd = "plats/pt1_strt";
        break;
    case down:
        snd = "plats/pt1_strt";
        break;
    case waiting:
    case in_stasis:
    case finished:
        snd = "plats/pt1_stop";
        break;
    default:
        return;
    }

    S_Sound(m_Sector->soundorg, CHAN_BODY, snd, 1, ATTN_NORM);
}

//
// Move a plat up and down
//
void DPlat::RunThink()
{
    EResult res;

    switch (m_Status)
    {
    case midup:
    case up:
        res = MoveFloor(m_Speed, m_High, m_Crush ? DOOM_CRUSH : NO_CRUSH, 1, false);

        if (res == crushed && !m_Crush)
        {
            m_Count  = m_Wait;
            m_Status = down;
            PlayPlatSound();
        }
        else if (res == pastdest)
        {
            if (m_Type != platToggle)
            {
                m_Count  = m_Wait;
                m_Status = waiting;

                switch (m_Type)
                {
                case platDownWaitUpStay:
                case platRaiseAndStay:
                case platUpByValueStay:
                case platDownToNearestFloor:
                case platDownToLowestCeiling:
                case platRaiseAndStayLockout:
                case blazeDWUS:
                case raiseAndChange:
                case raiseToNearestAndChange:
                case genLift:
                    m_Status = finished;
                    break;

                default:
                    break;
                }
            }
            else
            {
                m_OldStatus = m_Status;  // jff 3/14/98 after action wait
                m_Status    = in_stasis; // for reactivation of toggle
            }
            PlayPlatSound();
        }
        break;

    case middown:
    case down:
        res = MoveFloor(m_Speed, m_Low, NO_CRUSH, -1, false);

        if (res == pastdest)
        {
            // if not an instant toggle, start waiting
            if (m_Type != platToggle) // jff 3/14/98 toggle up down
            {                         // is silent, instant, no waiting
                m_Count  = m_Wait;
                m_Status = waiting;

                switch (m_Type)
                {
                case platUpWaitDownStay:
                case platUpNearestWaitDownStay:
                case platUpByValue:
                case raiseAndChange:
                case raiseToNearestAndChange:
                    m_Status = finished;
                    break;

                default:
                    break;
                }
            }
            else
            {                            // instant toggles go into stasis awaiting next activation
                m_OldStatus = m_Status;  // jff 3/14/98 after action wait
                m_Status    = in_stasis; // for reactivation of toggle
            }

            PlayPlatSound();
        }
        // jff 1/26/98 remove the plat if it bounced so it can be tried again
        // only affects plats that raise and bounce

        // remove the plat if it's a pure raise type
        switch (m_Type)
        {
        case platUpByValueStay:
        case platRaiseAndStay:
        case platRaiseAndStayLockout:
        case raiseAndChange:
        case raiseToNearestAndChange:
            m_Status = finished;
            break;

        default:
            break;
        }

        break;

    case waiting:
        if (!--m_Count)
        {
            if (P_FloorHeight(m_Sector) <= m_Low)
                m_Status = up;
            else
                m_Status = down;

            PlayPlatSound();
        }
        break;
    case in_stasis:
        break;

    default:
        break;
    }

    if (m_Status == finished)
    {
        PlayPlatSound();
        if (!predicting)
            m_Status = destroy;
    }

    if (m_Status == destroy)
        P_SetPlatDestroy(this);
}

DPlat::DPlat(sector_t *sector) : DMovingFloor(sector), m_Status(init)
{
}

void P_ActivateInStasis(int32_t tag)
{
    DPlat                  *scan;
    TThinkerIterator<DPlat> iterator;

    while ((scan = iterator.Next()))
    {
        if (scan->m_Tag == tag && scan->m_Status == DPlat::in_stasis)
            scan->Reactivate();
    }
}

DPlat::DPlat(sector_t *sec, DPlat::EPlatType type, fixed_t height, int32_t speed, int32_t delay, fixed_t lip)
    : DMovingFloor(sec), m_Status(init)
{
    m_Type   = type;
    m_Crush  = false;
    m_Speed  = speed;
    m_Wait   = delay;
    m_Height = height;
    m_Lip    = lip;

    // jff 1/26/98 Avoid raise plat bouncing a head off a ceiling and then
    // going down forever -- default lower to plat height when triggered
    m_Low = P_FloorHeight(sec);

    switch (type)
    {
    case DPlat::platRaiseAndStay:
        m_High   = P_FindNextHighestFloor(sec);
        m_Status = DPlat::midup;
        PlayPlatSound();
        break;

    case DPlat::platUpByValue:
    case DPlat::platUpByValueStay:
        m_High   = P_FloorHeight(sec) + height;
        m_Status = DPlat::midup;
        PlayPlatSound();
        break;

    case DPlat::platDownByValue:
        m_Low    = P_FloorHeight(sec) - height;
        m_Status = DPlat::middown;
        PlayPlatSound();
        break;

    case DPlat::platDownWaitUpStay:
    case DPlat::platDownWaitUpStayStone:
        m_Low = P_FindLowestFloorSurrounding(sec) + lip;

        if (m_Low > P_FloorHeight(sec))
            m_Low = P_FloorHeight(sec);

        m_High   = P_FloorHeight(sec);
        m_Status = DPlat::down;
        PlayPlatSound();
        break;

    case DPlat::platUpNearestWaitDownStay:
        m_High   = P_FindNextHighestFloor(sec);
        m_Status = DPlat::up;
        PlayPlatSound();
        break;

    case DPlat::platUpWaitDownStay:
        m_High = P_FindHighestFloorSurrounding(sec);

        if (m_High < P_FloorHeight(sec))
            m_High = P_FloorHeight(sec);

        m_Status = DPlat::up;
        PlayPlatSound();
        break;

    case DPlat::platPerpetualRaise:
        m_Low = P_FindLowestFloorSurrounding(sec) + lip;

        if (m_Low > P_FloorHeight(sec))
            m_Low = P_FloorHeight(sec);

        m_High = P_FindHighestFloorSurrounding(sec);

        if (m_High < P_FloorHeight(sec))
            m_High = P_FloorHeight(sec);

        m_Status = P_Random() & 1 ? DPlat::down : DPlat::up;

        PlayPlatSound();
        break;

    case DPlat::platToggle: // jff 3/14/98 add new type to support instant toggle
        m_Crush = false;    // jff 3/14/98 crush anything in the way

        // set up toggling between ceiling, floor inclusive
        m_Low    = P_CeilingHeight(sec);
        m_High   = P_FloorHeight(sec);
        m_Status = DPlat::down;
        // 			SN_StartSequence (sec, "Silence");
        break;

    case DPlat::platDownToNearestFloor:
        m_Low    = P_FindNextLowestFloor(sec) + lip;
        m_Status = DPlat::down;
        m_High   = P_FloorHeight(sec);
        PlayPlatSound();
        break;

    case DPlat::platDownToLowestCeiling:
        m_Low  = P_FindLowestCeilingSurrounding(sec);
        m_High = P_FloorHeight(sec);

        if (m_Low > P_FloorHeight(sec))
            m_Low = P_FloorHeight(sec);

        m_Status = DPlat::down;
        PlayPlatSound();
        break;

    default:
        break;
    }
}

DPlat::DPlat(sector_t *sec, int32_t target, int32_t delay, int32_t speed, int32_t trigger)
    : DMovingFloor(sec), m_Status(init)
{
    m_Crush  = false;
    m_Type   = genLift;
    m_Status = DPlat::down;
    m_Height = 0;
    m_Lip    = 0;
    m_High   = sec->floorheight;

    // setup the target destination height
    switch (target)
    {
    case F2LnF:
        m_Low = P_FindLowestFloorSurrounding(sec);
        if (m_Low > sec->floorheight)
            m_Low = sec->floorheight;
        break;
    case F2NnF:
        m_Low = P_FindNextLowestFloor(sec);
        break;
    case F2LnC:
        m_Low = P_FindLowestCeilingSurrounding(sec);
        if (m_Low > sec->floorheight)
            m_Low = sec->floorheight;
        break;
    case LnF2HnF:
        m_Type = genPerpetual;
        m_Low  = P_FindLowestFloorSurrounding(sec);
        if (m_Low > sec->floorheight)
            m_Low = sec->floorheight;
        m_High = P_FindHighestFloorSurrounding(sec);
        if (m_High < sec->floorheight)
            m_High = sec->floorheight;
        m_Status = (EPlatState)(P_Random() & 1 ? DPlat::down : DPlat::up);
        break;
    default:
        break;
    }

    // setup the speed of motion
    switch (speed)
    {
    case SpeedSlow:
        m_Speed = PLATSPEED * 2;
        break;
    case SpeedNormal:
        m_Speed = PLATSPEED * 4;
        break;
    case SpeedFast:
        m_Speed = PLATSPEED * 8;
        break;
    case SpeedTurbo:
        m_Speed = PLATSPEED * 16;
        break;
    default:
        break;
    }

    // setup the delay time before the floor returns
    switch (delay)
    {
    case 0:
        m_Wait = 1 * 35;
        break;
    case 1:
        m_Wait = 3 * 35;
        break;
    case 2:
        m_Wait = 5 * 35;
        break;
    case 3:
        m_Wait = 10 * 35;
        break;
    }
    PlayPlatSound();
}

// Clones a DPlat and returns a pointer to that clone.
//
// The caller owns the pointer, and it must be deleted with `delete`.
DPlat *DPlat::Clone(sector_t *sec) const
{
    DPlat *plat = new DPlat(*this);

    plat->Orphan();
    plat->m_Sector = sec;

    return plat;
}

//
// Do Platforms
//	[RH] Changed amount to height and added delay,
//		 lip, change, tag, and speed parameters.
//
bool EV_DoPlat(int32_t tag, line_t *line, DPlat::EPlatType type, fixed_t height, int32_t speed, int32_t delay,
               fixed_t lip, int32_t change)
{
    DPlat    *plat;
    int32_t   secnum;
    sector_t *sec;
    int32_t   rtn    = false;
    bool      manual = false;

    // [RH] If tag is zero, use the sector on the back side
    //		of the activating line (if any).
    if (tag == 0)
    {
        if (!line || !(sec = line->backsector))
            return false;
        secnum = sec - sectors;
        manual = true;
        goto manual_plat;
    }

    //	Activate all <type> plats that are in_stasis
    switch (type)
    {
    case DPlat::platToggle:
        rtn = true;
    case DPlat::platPerpetualRaise:
        P_ActivateInStasis(tag);
        break;

    default:
        break;
    }

    secnum = -1;
    while ((secnum = P_FindSectorFromTag(tag, secnum)) >= 0)
    {
        sec = &sectors[secnum];

    manual_plat:
        if (sec->floordata)
        {
            if (manual)
                return false;
            else
                continue;
        }

        // Find lowest & highest floors around sector
        rtn  = true;
        plat = new DPlat(sec, type, height, speed, delay, lip);
        P_AddMovingFloor(sec);

        plat->m_Tag = tag;

        if (change)
        {
            if (line)
                sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
            if (change == 1)
                sec->special = 0; // Stop damage and other stuff, if any
            if (serverside)
                SV_BroadcastSector(secnum);
        }

        if (manual)
            return rtn;
    }
    return rtn;
}

bool EV_DoGenLift(line_t *line)
{
    DPlat    *plat;
    int32_t   secnum;
    sector_t *sec;
    bool      rtn    = false;
    bool      manual = false;
    uint32_t  value  = (uint32_t)line->special - GenLiftBase;

    int32_t Targ = (value & LiftTarget) >> LiftTargetShift;
    int32_t Dely = (value & LiftDelay) >> LiftDelayShift;
    int32_t Sped = (value & LiftSpeed) >> LiftSpeedShift;
    int32_t Trig = (value & TriggerType) >> TriggerTypeShift;

    // Activate all <type> plats that are in_stasis

    if (Targ == LnF2HnF)
        P_ActivateInStasis(line->id);

    if (Trig == PushOnce || Trig == PushMany)
    {
        if (!line || !(sec = line->backsector))
            return false;
        secnum = sec - sectors;
        manual = true;
        goto manual_genplat;
    }

    secnum = -1;
    while ((secnum = P_FindSectorFromTagOrLine(line->id, line, secnum)) >= 0)
    {
    manual_genplat:
        sec = &sectors[secnum];
        if (sec->floordata)
        {
            if (manual)
                return false;
            else
                continue;
        }

        // Find lowest & highest floors around sector
        rtn  = true;
        plat = new DPlat(sec, Targ, Dely, Sped, Trig);

        plat->m_Tag = line->id;

        P_AddMovingFloor(sec);

        if (manual)
            return rtn;
    }
    return rtn;
}

void DPlat::Reactivate()
{
    if (m_Type == platToggle) // jff 3/14/98 reactivate toggle type
        m_Status = m_OldStatus == up ? down : up;
    else
        m_Status = m_OldStatus;
}

void DPlat::Stop()
{
    m_OldStatus = m_Status;
    m_Status    = in_stasis;
}

void EV_StopPlat(int32_t tag)
{
    DPlat                  *scan;
    TThinkerIterator<DPlat> iterator;

    while ((scan = iterator.Next()))
    {
        if (scan->m_Status != DPlat::in_stasis && scan->m_Tag == tag)
            scan->Stop();
    }
}

VERSION_CONTROL(p_plats_cpp, "$Id: 04d48ae09090f3250dbe62d9e63f21bf1bfa7387 $")
