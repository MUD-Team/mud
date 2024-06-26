// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: da1fa1fff5f13decf7d8e66a6a28659289846444 $
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
//	Command-line variables
//
//-----------------------------------------------------------------------------

#pragma once

#include <cfloat>
#include <string>

#include "doomtype.h"
#include "tarray.h"

// Uncomment to allow for latency simulation - see sv_latency in sv_cvarlist.cpp
// Note: When compiling for linux you will have link against pthread manually
// #define SIMULATE_LATENCY

/*
==========================================================

CVARS (console variables)

==========================================================
*/

/**
 * [deathz0r] no special properties.
 */
#define CVAR_NULL 0

/**
 * Added to userinfo when changed.
 */
#define CVAR_USERINFO BIT(1)

/**
 * [Toke - todo] Changed the meaning of this flag, it now describes cvars that
 *               clients will be informed if changed.
 */
#define CVAR_SERVERINFO BIT(2)

/**
 * Don't allow change from console at all, but can be set from the command line.
 */
#define CVAR_NOSET BIT(3)

/**
 * Save changes until server restart.
 */
#define CVAR_LATCH BIT(4)

/**
 * Can unset this var from console.
 */
#define CVAR_UNSETTABLE BIT(5)

/**
 * Set each time the cvar_t is changed
 */
#define CVAR_MODIFIED BIT(7)

/**
 * Is cvar unchanged since creation?
 */
#define CVAR_ISDEFAULT BIT(8)

/**
 * Allocated, needs to be freed when destroyed.
 */
#define CVAR_AUTO BIT(9)

/**
 * [Nes] No substitution (0=disable, 1=enable)
 */
#define CVAR_NOENABLEDISABLE BIT(10)

/**
 * [Nes] Server version of CVAR_ARCHIVE
 */
#define CVAR_SERVERARCHIVE BIT(12)

/**
 * [Nes] Client version of CVAR_ARCHIVE
 */
#define CVAR_CLIENTARCHIVE BIT(13)

/**
 * [SL] CVAR_ARCHIVE enables both CVAR_CLIENTARCHIVE & CVAR_SERVERARCHIVE
 */
#define CVAR_ARCHIVE (CVAR_CLIENTARCHIVE | CVAR_SERVERARCHIVE)

// Hints for network code optimization
typedef enum
{
    CVARTYPE_NONE = 0 // Used for no sends

        ,
    CVARTYPE_BOOL,
    CVARTYPE_BYTE,
    CVARTYPE_WORD,
    CVARTYPE_INT,
    CVARTYPE_FLOAT,
    CVARTYPE_STRING

        ,
    CVARTYPE_MAX = 255
} cvartype_t;

class cvar_t
{
  public:
    cvar_t(const char *name, const char *def, const char *help, cvartype_t, uint32_t flags, float minval = -FLT_MAX,
           float maxval = FLT_MAX);
    cvar_t(const char *name, const char *def, const char *help, cvartype_t, uint32_t flags, void (*callback)(cvar_t &),
           float minval = -FLT_MAX, float maxval = FLT_MAX);
    virtual ~cvar_t();

    const char *cstring() const
    {
        return m_String.c_str();
    }
    const std::string &str() const
    {
        return m_String;
    }
    const char *name() const
    {
        return m_Name.c_str();
    }
    const char *helptext() const
    {
        return m_HelpText.c_str();
    }
    const char *latched() const
    {
        return m_LatchedString.c_str();
    }
    float value() const
    {
        return m_Value;
    }
    operator float() const
    {
        return m_Value;
    }
    operator const std::string &() const
    {
        return m_String;
    }
    uint32_t flags() const
    {
        return m_Flags;
    }
    cvartype_t type() const
    {
        return m_Type;
    }
    const std::string &getDefault() const
    {
        return m_Default;
    }
    float getMinValue() const
    {
        return m_MinValue;
    }
    float getMaxValue() const
    {
        return m_MaxValue;
    }

    // return m_Value as an int32_t, rounded to the nearest integer because
    // casting truncates instead of rounding
    int32_t asInt() const
    {
        return static_cast<int32_t>(m_Value >= 0.0f ? m_Value + 0.5f : m_Value - 0.5f);
    }

    inline void Callback()
    {
        if (m_Callback)
            m_Callback(*this);
    }

    void SetDefault(const char *value);
    void RestoreDefault();
    void Set(const char *value);
    void Set(float value);
    void ForceSet(const char *value);
    void ForceSet(float value);

    static void Transfer(const char *fromname, const char *toname);

    static void EnableNoSet(); // enable the honoring of CVAR_NOSET
    static void EnableCallbacks();

    uint32_t m_Flags;

    // Writes all cvars that could effect demo sync to *demo_p. These are
    // cvars that have either CVAR_SERVERINFO or CVAR_DEMOSAVE set.
    static void C_WriteCVars(uint8_t **demo_p, uint32_t filter, bool compact = false);

    // Read all cvars from *demo_p and set them appropriately.
    static void C_ReadCVars(uint8_t **demo_p);

    // Backup cvars for restoration later. Called before connecting to a server
    // or a demo starts playing to save all cvars which could be changed while
    // by the server or by playing a demo.
    // [SL] bitflag can be used to filter which cvars are set to default.
    // The default value for bitflag is 0xFFFFFFFF, which effectively disables
    // the filtering.
    static void C_BackupCVars(uint32_t bitflag = 0xFFFFFFFF);

    // Restore demo cvars. Called after demo playback to restore all cvars
    // that might possibly have been changed during the course of demo playback.
    static void C_RestoreCVars(void);

    // Finds a named cvar
    static cvar_t *FindCVar(const char *var_name, cvar_t **prev);

    // Called from G_InitNew()
    static void UnlatchCVars(void);

    // archive cvars to PHYSFS_File f
    static void C_ArchiveCVars(void *f);

    // Initialize cvars to default values after they are created.
    // [SL] bitflag can be used to filter which cvars are set to default.
    // The default value for bitflag is 0xFFFFFFFF, which effectively disables
    // the filtering.
    static void C_SetCVarsToDefaults(uint32_t bitflag = 0xFFFFFFFF);

    static bool SetServerVar(const char *name, const char *value);

    static void FilterCompactCVars(TArray<cvar_t *> &cvars, uint32_t filter);

    // console variable interaction
    static cvar_t *cvar_set(const char *var_name, const char *value);
    static cvar_t *cvar_forceset(const char *var_name, const char *value);

    // list all console variables
    static void cvarlist();

    cvar_t &operator=(float other)
    {
        ForceSet(other);
        return *this;
    }
    cvar_t &operator=(const char *other)
    {
        ForceSet(other);
        return *this;
    }

    cvar_t *GetNext()
    {
        return m_Next;
    }

  private:
    cvar_t(const cvar_t &var)
    {
    }

    void InitSelf(const char *name, const char *def, const char *help, cvartype_t, uint32_t flags,
                  void (*callback)(cvar_t &), float minval = -FLT_MAX, float maxval = FLT_MAX);

    void (*m_Callback)(cvar_t &);
    cvar_t *m_Next;

    cvartype_t m_Type;

    std::string m_Name, m_String;
    std::string m_HelpText;

    float m_Value;
    float m_MinValue, m_MaxValue;

    std::string m_LatchedString, m_Default;

    static bool m_UseCallback;
    static bool m_DoNoSet;

  protected:
    cvar_t()
        : m_Flags(0), m_Callback(NULL), m_Next(NULL), m_Type(CVARTYPE_NONE), m_Value(0.f), m_MinValue(-FLT_MAX),
          m_MaxValue(FLT_MAX)
    {
    }
};

cvar_t *GetFirstCvar(void);

// Maximum number of cvars that can be saved.
#define MAX_BACKUPCVARS 512

#define CVAR(name, def, help, type, flags) cvar_t name(#name, def, help, type, flags);

#define CVAR_RANGE(name, def, help, type, flags, minval, maxval)                                                       \
    cvar_t name(#name, def, help, type, flags, minval, maxval);

#define EXTERN_CVAR(name) extern cvar_t name;

#define CVAR_FUNC_DECL(name, def, help, type, flags)                                                                   \
    extern void cvarfunc_##name(cvar_t &);                                                                             \
    cvar_t      name(#name, def, help, type, flags, cvarfunc_##name);

#define CVAR_RANGE_FUNC_DECL(name, def, help, type, flags, minval, maxval)                                             \
    extern void cvarfunc_##name(cvar_t &);                                                                             \
    cvar_t      name(#name, def, help, type, flags, cvarfunc_##name, minval, maxval);

#define CVAR_FUNC_IMPL(name)                                                                                           \
    EXTERN_CVAR(name)                                                                                                  \
    void cvarfunc_##name(cvar_t &var)
