// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: a7e1dcecd4fc1e9a74681825308f5bc97cb783f9 $
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

#ifdef UNIX
#include <dirent.h>
#endif

#include "d_event.h"
#include "d_ticcmd.h"

/*extern "C"
{
    extern byte CPUFamily, CPUModel, CPUStepping;
}*/

// Index values into the LanguageIDs array
enum
{
    LANGIDX_UserPreferred,
    LANGIDX_UserDefault,
    LANGIDX_SysPreferred,
    LANGIDX_SysDefault
};
extern uint32_t LanguageIDs[4];
extern void     SetLanguageIDs();

void I_BeginRead(void);
void I_EndRead(void);

// Called by DoomMain.
void I_Init(void);

// Called by startup code
// to get the amount of memory to malloc
// for the zone management.
void *I_ZoneBase(size_t *size);

dtime_t I_GetTime();
dtime_t I_ConvertTimeToMs(dtime_t value);
dtime_t I_ConvertTimeFromMs(dtime_t value);
void    I_Sleep(dtime_t sleep_time);

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
ticcmd_t *I_BaseTiccmd(void);

// Called by M_Responder when quit is selected.
// Clean exit, displays sell blurb.
void STACK_ARGS I_Quit(void);

void STACK_ARGS I_Error(const char *error, ...);

void addterm(void(STACK_ARGS *func)(void), const char *name);
#define atterm(t) addterm(t, #t)

// Print a console string
void I_PrintStr(int32_t x, const char *str, int32_t count, bool scroll);

// Set the title string of the startup window
void I_SetTitleString(const char *title);

std::string I_ConsoleInput(void);

// [RH] Returns millisecond-accurate time
dtime_t I_MSTime(void);

void I_Yield(void);

// [RH] Title string to display at bottom of console during startup
extern char DoomStartupTitle[256];

void I_FinishClockCalibration();