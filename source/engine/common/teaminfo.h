// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 41c46311469a278b978c8778515441d207f3eb55 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//   Team Information header.
//
//-----------------------------------------------------------------------------

#pragma once

#include <vector>

#include "actor.h"
#include "doomdata.h"
#include "info.h"

enum team_t
{
    TEAM_BLUE,
    TEAM_RED,
    TEAM_GREEN,

    NUMTEAMS,

    TEAM_NONE
};

struct TeamInfo
{
    team_t      Team;
    std::string ColorStringUpper;
    std::string ColorString;
    argb_t      Color;
    std::string TextColor;
    std::string ToastColor;
    int32_t     TransColor;

    int32_t FountainColorArg;

    int32_t                  TeamSpawnThingNum;
    std::vector<mapthing2_t> Starts;

    int32_t Points;
    int32_t RoundWins;

    const std::string ColorizedTeamName();
    int32_t           LivesPool();
};

/**
 * @brief A collection of pointers to teams, commonly called a "view".
 */
typedef std::vector<TeamInfo *> TeamsView;

class TeamQuery
{
    enum SortTypes
    {
        SORT_NONE,
        SORT_LIVES,
        SORT_SCORE,
        SORT_WINS,
    };

    enum SortFilters
    {
        SFILTER_NONE,
        SFILTER_MAX,
        SFILTER_NOT_MAX,
    };

    SortTypes   m_sort;
    SortFilters m_sortFilter;

  public:
    TeamQuery() : m_sort(SORT_NONE), m_sortFilter(SFILTER_NONE)
    {
    }

    /**
     * @brief Return teams sorted by greatest number of total lives.
     *
     * @return A mutated TeamQuery to chain off of.
     */
    TeamQuery &sortLives()
    {
        m_sort = SORT_LIVES;
        return *this;
    }

    /**
     * @brief Return teams sorted by highest score.
     *
     * @return A mutated TeamQuery to chain off of.
     */
    TeamQuery &sortScore()
    {
        m_sort = SORT_SCORE;
        return *this;
    }

    /**
     * @brief Return teams sorted by highest wins.
     *
     * @return A mutated TeamQuery to chain off of.
     */
    TeamQuery &sortWins()
    {
        m_sort = SORT_WINS;
        return *this;
    }

    /**
     * @brief Given a sort, filter so only the top item remains.  In the case
     *        of a tie, multiple items are returned.
     *
     * @return A mutated TeamQuery to chain off of.
     */
    TeamQuery &filterSortMax()
    {
        m_sortFilter = SFILTER_MAX;
        return *this;
    }

    /**
     * @brief Given a sort, filter so only things other than the top item
     *        remains.
     *
     * @return A mutated TeamQuery to chain off of.
     */
    TeamQuery &filterSortNotMax()
    {
        m_sortFilter = SFILTER_NOT_MAX;
        return *this;
    }

    TeamsView execute();
};

void        InitTeamInfo();
void        TeamInfo_ResetScores(bool fullreset = true);
TeamInfo   *GetTeamInfo(team_t team);
std::string V_GetTeamColor(TeamInfo *team);
std::string V_GetTeamColor(team_t ateam);
