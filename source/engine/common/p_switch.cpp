// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: efa54b9274a0458223dc204261199d3d7053c75b $
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
//	Switches, buttons. Two-state animation. Exits.
//
//-----------------------------------------------------------------------------

#include <map>

#include "gi.h"
#include "i_system.h"
#include "mud_includes.h"
#include "m_fileio.h"
#include "p_lnspec.h"
#include "p_local.h"
#include "p_mapformat.h"
#include "r_state.h"
#include "s_sound.h"
#include "svc_message.h"
#include "w_wad.h"
#include "z_zone.h"

extern int32_t numtextures;

//
// CHANGE THE TEXTURE OF A WALL SWITCH TO ITS OPPOSITE
//

class DActiveButton : public DThinker
{
    DECLARE_SERIAL(DActiveButton, DThinker);

  public:
    enum EWhere
    {
        BUTTON_Top,
        BUTTON_Middle,
        BUTTON_Bottom,
        BUTTON_Nowhere
    };

    DActiveButton();
    DActiveButton(line_t *, EWhere, texhandle_t tex, int32_t time, fixed_t x, fixed_t y);

    void RunThink();

    line_t     *m_Line;
    EWhere      m_Where;
    texhandle_t m_Texture;
    int32_t      m_Timer;
    fixed_t     m_X, m_Y; // Location of timer sound

    friend FArchive &operator<<(FArchive &arc, EWhere where)
    {
        return arc << (uint8_t)where;
    }
    friend FArchive &operator>>(FArchive &arc, EWhere &out)
    {
        uint8_t in;
        arc >> in;
        out = (EWhere)in;
        return arc;
    }
};

static int32_t *switchlist;
static int32_t  numswitches;

//
// P_InitSwitchList
// Only called at game initialization.
//
// [RH] Rewritten to use a BOOM-style SWITCHES lump and remove the
//		MAXSWITCHES limit.
void P_InitSwitchList(void)
{
    if (!M_FileExists("lumps/SWITCHES.lmp"))
        I_FatalError("Missing lumps/SWITCHES.lmp");    

    PHYSFS_File *rawswitches = PHYSFS_openRead("lumps/SWITCHES.lmp");

    if (rawswitches == NULL)
    {
        I_FatalError("Error opening lumps/SWITCHES.lmp"); 
    }

    uint8_t *alphSwitchList = new uint8_t[PHYSFS_fileLength(rawswitches)];

    if (PHYSFS_readBytes(rawswitches, alphSwitchList, PHYSFS_fileLength(rawswitches)) != PHYSFS_fileLength(rawswitches))
    {
        PHYSFS_close(rawswitches);
        delete[] alphSwitchList;
        I_FatalError("Error reading lumps/SWITCHES.lmp"); 
    }
    
    PHYSFS_close(rawswitches);

    uint8_t *list_p;
    int32_t   i;

    for (i = 0, list_p = alphSwitchList; list_p[18] || list_p[19]; list_p += 20, i++)
        ;

    if (i == 0)
    {
        switchlist  = (int32_t *)Z_Malloc(sizeof(*switchlist), PU_STATIC, 0);
        *switchlist = TextureManager::NOT_FOUND_TEXTURE_HANDLE;
        numswitches = 0;
    }
    else
    {
        switchlist = (int32_t *)Z_Malloc(sizeof(*switchlist) * (i * 2 + 1), PU_STATIC, 0);

        for (i = 0, list_p = alphSwitchList; list_p[18] || list_p[19]; list_p += 20)
        {
            if (((gameinfo.maxSwitch & 15) >= (list_p[18] & 15)) && ((gameinfo.maxSwitch & ~15) == (list_p[18] & ~15)))
            {
                texhandle_t switchtex = texturemanager.getHandle((const char *)list_p, Texture::TEX_TEXTURE);
                // [RH] Skip this switch if it can't be found.
                if (switchtex == TextureManager::NOT_FOUND_TEXTURE_HANDLE)
                    continue;

                switchlist[i++] = switchtex;
                switchlist[i++] = texturemanager.getHandle((const char *)(list_p+9), Texture::TEX_TEXTURE);
            }
        }
        numswitches   = i / 2;
        switchlist[i] = TextureManager::NOT_FOUND_TEXTURE_HANDLE;
    }

    delete[] alphSwitchList;
}

void P_DestroyButtonThinkers()
{
    DActiveButton                  *button;
    TThinkerIterator<DActiveButton> iterator;

    while ((button = iterator.Next()))
        button->Destroy();
}

//
// Start a button counting down till it turns off.
// [RH] Rewritten to remove MAXBUTTONS limit and use temporary soundorgs.
//
static void P_StartButton(line_t *line, DActiveButton::EWhere w, int32_t texture, int32_t time, fixed_t x, fixed_t y)
{
    DActiveButton                  *button;
    TThinkerIterator<DActiveButton> iterator;

    // See if button is already pressed
    while ((button = iterator.Next()))
    {
        if (button->m_Line == line)
            return;
    }

    new DActiveButton(line, w, texture, time, x, y);
}

texhandle_t *P_GetButtonTexturePtr(line_t *line, texhandle_t *&altTexture, DActiveButton::EWhere &where)
{
    if (!line->sidenum[0])
        return NULL;

    texhandle_t texTop = sides[line->sidenum[0]].toptexture;
    texhandle_t texMid = sides[line->sidenum[0]].midtexture;
    texhandle_t texBot = sides[line->sidenum[0]].bottomtexture;
    where      = (DActiveButton::EWhere)0;
    altTexture = NULL;

    for (int32_t i = 0; i < numswitches * 2; i++)
    {
        if (switchlist[i] == texTop)
        {
            altTexture = (texhandle_t *)&switchlist[i ^ 1];
            where      = DActiveButton::BUTTON_Top;
            return &sides[line->sidenum[0]].toptexture;
        }
        else if (switchlist[i] == texBot)
        {
            altTexture = (texhandle_t *)&switchlist[i ^ 1];
            where      = DActiveButton::BUTTON_Bottom;
            return &sides[line->sidenum[0]].bottomtexture;
        }
        else if (switchlist[i] == texMid)
        {
            altTexture = (texhandle_t *)&switchlist[i ^ 1];
            where      = DActiveButton::BUTTON_Middle;
            return &sides[line->sidenum[0]].midtexture;
        }
    }

    return NULL;
}

texhandle_t P_GetButtonTexture(line_t *line)
{
    DActiveButton::EWhere twhere;
    texhandle_t                *alt;
    texhandle_t                *texture = P_GetButtonTexturePtr(line, alt, twhere);

    if (texture)
        return *texture;

    return TextureManager::NO_TEXTURE_HANDLE;
}

void P_SetButtonTexture(line_t *line, texhandle_t texture)
{
    if (texture != TextureManager::NO_TEXTURE_HANDLE)
    {
        DActiveButton::EWhere twhere;
        texhandle_t                *alt;
        texhandle_t                *findTexture = P_GetButtonTexturePtr(line, alt, twhere);

        if (findTexture)
            *findTexture = texture;
    }
}

// denis - query button
bool P_GetButtonInfo(line_t *line, uint32_t &state, uint32_t &time)
{
    DActiveButton                  *button;
    TThinkerIterator<DActiveButton> iterator;

    // See if button is already pressed
    while ((button = iterator.Next()))
    {
        if (button->m_Line == line)
        {
            state = button->m_Where;
            time  = button->m_Timer;
            return true;
        }
    }

    return false;
}

bool P_SetButtonInfo(line_t *line, uint32_t state, uint32_t time)
{
    DActiveButton                  *button;
    TThinkerIterator<DActiveButton> iterator;

    // See if button is already pressed
    while ((button = iterator.Next()))
    {
        if (button->m_Line == line)
        {
            button->m_Where = (DActiveButton::EWhere)state;
            button->m_Timer = time;
            return true;
        }
    }

    return false;
}

void P_UpdateButtons(client_t *cl)
{
    DActiveButton                  *button;
    TThinkerIterator<DActiveButton> iterator;
    std::map<uint32_t, bool>        actedlines;

    // See if button is already pressed
    while ((button = iterator.Next()))
    {
        if (button->m_Line == NULL)
            continue;

        uint32_t l     = button->m_Line - lines;
        uint32_t state = 0, timer = 0;

        state = button->m_Where;
        timer = button->m_Timer;

        // record that we acted on this line:
        actedlines[l] = true;

        MSG_WriteSVC(&cl->reliablebuf, SVC_Switch(lines[l], state, timer));
    }

    for (int32_t l = 0; l < numlines; l++)
    {
        // update all button state except those that have actors assigned:
        if (!actedlines[l] && lines[l].wastoggled)
        {
            MSG_WriteSVC(&cl->reliablebuf, SVC_Switch(lines[l], 0, 0));
        }
    }
}

//
// Function that changes wall texture.
// Tell it if switch is ok to use again (1=yes, it's a button).
//
void P_ChangeSwitchTexture(line_t *line, int32_t useAgain, bool playsound)
{
    const char *sound;

    // EXIT SWITCH?
    if (P_IsExitLine(line->special))
    {
        sound = "switches/exitbutn";
    }
    else
    {
        sound = "switches/normbutn";
    }

    if (!useAgain && P_HandleSpecialRepeat(line))
        line->special = 0;

    DActiveButton::EWhere twhere;
    texhandle_t                *altTexture;
    texhandle_t                *texture = P_GetButtonTexturePtr(line, altTexture, twhere);

    if (texture)
    {
        // [RH] The original code played the sound at buttonlist->soundorg,
        //		which wasn't necessarily anywhere near the switch if
        //		it was facing a big sector.
        fixed_t x = line->v1->x + (line->dx >> 1);
        fixed_t y = line->v1->y + (line->dy >> 1);

        if (playsound)
        {
            // [SL] 2011-05-27 - Play at a normal volume in the center
            // of the switch's linedef
            S_Sound(x, y, CHAN_BODY, sound, 1, ATTN_NORM);
        }

        if (useAgain)
            P_StartButton(line, twhere, *texture, BUTTONTIME, x, y);
        *texture           = *altTexture;
        line->switchactive = true;
    }

    line->wastoggled = true;
}

IMPLEMENT_SERIAL(DActiveButton, DThinker)

DActiveButton::DActiveButton()
{
    m_Line    = NULL;
    m_Where   = BUTTON_Nowhere;
    m_Texture = 0;
    m_Timer   = 0;
    m_X       = 0;
    m_Y       = 0;
}

DActiveButton::DActiveButton(line_t *line, EWhere where, texhandle_t texture, int32_t time, fixed_t x, fixed_t y)
{
    m_Line    = line;
    m_Where   = where;
    m_Texture = texture;
    m_Timer   = time;
    m_X       = x;
    m_Y       = y;
}

void DActiveButton::Serialize(FArchive &arc)
{
    Super::Serialize(arc);
    if (arc.IsStoring())
    {
        arc << m_Line << m_Where << m_Texture << m_Timer << m_X << m_Y;
    }
    else
    {
        arc >> m_Line >> m_Where >> m_Texture >> m_Timer >> m_X >> m_Y;
    }
}

void DActiveButton::RunThink()
{
    if (0 >= --m_Timer)
    {
        switch (m_Where)
        {
        case BUTTON_Top:
            sides[m_Line->sidenum[0]].toptexture = m_Texture;
            break;

        case BUTTON_Middle:
            sides[m_Line->sidenum[0]].midtexture = m_Texture;
            break;

        case BUTTON_Bottom:
            sides[m_Line->sidenum[0]].bottomtexture = m_Texture;
            break;

        default:
            break;
        }

        // [SL] 2011-05-27 - Play at a normal volume in the center of the
        //  switch's linedef
        fixed_t x = m_Line->v1->x + (m_Line->dx >> 1);
        fixed_t y = m_Line->v1->y + (m_Line->dy >> 1);
        S_Sound(x, y, CHAN_BODY, "switches/normbutn", 1, ATTN_NORM);
        
        Destroy();
        m_Line->switchactive = false;
    }
}

VERSION_CONTROL(p_switch_cpp, "$Id: efa54b9274a0458223dc204261199d3d7053c75b $")
