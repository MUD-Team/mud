// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 10c7cd38ec9f16ce06b65ca1250a8ba4d3ac0227 $
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
//	Simple basic typedefs, isolated here to make it easier
//	 separating modules.
//
//-----------------------------------------------------------------------------

#pragma once

// Standard libc/STL includes we use in countless places
#include <stdint.h>

#include "errors.h"
#include "version.h"

#if defined(_MSC_VER)
#define forceinline __forceinline
#elif defined(__GNUC__)
#define forceinline inline __attribute__((always_inline))
#else
#define forceinline inline
#endif

#ifdef _MSC_VER
#define FORMAT_PRINTF(index, first_arg)
#else
#define FORMAT_PRINTF(index, first_arg) __attribute__((format(printf, index, first_arg)))
#endif

#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#define NORETURN __attribute__((noreturn))
#endif

#if defined(_MSC_VER) || defined(__WATCOMC__)
#define STACK_ARGS __cdecl
#else
#define STACK_ARGS
#endif

// Predefined with some OS.
#if !defined(UNIX) && !defined(_WIN32)
#include <float.h>
#endif

#ifdef UNIX
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#endif

typedef uint64_t dtime_t;

#ifdef _WIN32
#define PATHSEP         "\\"
#define PATHSEPCHAR     '\\'
#define PATHLISTSEP     ";"
#define PATHLISTSEPCHAR ';'
#else
#define PATHSEP         "/"
#define PATHSEPCHAR     '/'
#define PATHLISTSEP     ":"
#define PATHLISTSEPCHAR ':'
#endif

/**
 * @brief Returns a bitfield with a specific bit set.
 */
#define BIT(a) (1U << (a))

/**
 * @brief Returns a bitfield with a range of bits set from a to b, inclusive.
 *
 * @param a Low bit in the mask.
 * @param b High bit in the mask.
 */
static inline uint32_t BIT_MASK(uint32_t a, uint32_t b)
{
    return (static_cast<uint32_t>(-1) >> (31 - b)) & ~(BIT(a) - 1);
}

// [RH] This gets used all over; define it here:
FORMAT_PRINTF(1, 2) int32_t STACK_ARGS Printf(const char *format, ...);
FORMAT_PRINTF(2, 3) int32_t STACK_ARGS Printf(int32_t printlevel, const char *format, ...);
// [Russell] Prints a bold green message to the console
FORMAT_PRINTF(1, 2) int32_t STACK_ARGS Printf_Bold(const char *format, ...);
// [RH] Same here:
FORMAT_PRINTF(1, 2) int32_t STACK_ARGS DPrintf(const char *format, ...);

/**
 * @brief Print to all clients in a server, or to the local player offline.
 *
 * @note This could really use a new name, like "ServerPrintf".
 *
 * @param format printf-style format string.
 * @param ... printf-style arguments.
 */
void STACK_ARGS SV_BroadcastPrintf(const char *format, ...) FORMAT_PRINTF(1, 2);

/**
 * @brief Print to all clients in a server, or to the local player offline.
 *
 * @note This could really use a new name, like "ServerPrintf".
 *
 * @param printlevel PRINT_* constant designating what kind of print this is.
 * @param format printf-style format string.
 * @param ... printf-style arguments.
 */
void STACK_ARGS SV_BroadcastPrintf(int32_t printlevel, const char *format, ...) FORMAT_PRINTF(2, 3);

#ifdef SERVER_APP
void STACK_ARGS SV_BroadcastPrintfButPlayer(int32_t printlevel, int32_t player_id, const char *format, ...);
#endif

// game print flags
typedef enum
{
    PRINT_PICKUP,     // Pickup messages
    PRINT_OBITUARY,   // Death messages
    PRINT_HIGH,       // Regular messages

    PRINT_CHAT,       // Chat messages
    PRINT_TEAMCHAT,   // Chat messages from a teammate
    PRINT_SERVERCHAT, // Chat messages from the server

    PRINT_WARNING,    // Warning messages
    PRINT_ERROR,      // Fatal error messages

    PRINT_NORCON,     // Do NOT send the message to any rcon client.

    PRINT_FILTERCHAT, // Filter the message to not be displayed ingame, but only in the console (ugly hack)

    PRINT_MAXPRINT
} printlevel_t;

//
// MIN
//
// Returns the minimum of a and b.
//
#ifdef MIN
#undef MIN
#endif
template <class T> forceinline const T MIN(const T a, const T b)
{
    return a < b ? a : b;
}

//
// MAX
//
// Returns the maximum of a and b.
//
#ifdef MAX
#undef MAX
#endif
template <class T> forceinline const T MAX(const T a, const T b)
{
    return a > b ? a : b;
}

//
// clamp
//
// Clamps the value of in to the range min, max
//
#ifdef clamp
#undef clamp
#endif
template <class T> forceinline T clamp(const T in, const T min, const T max)
{
    return in <= min ? min : in >= max ? max : in;
}

//
// ARRAY_LENGTH
//
// Safely counts the number of items in an C array.
//
// https://www.drdobbs.com/cpp/counting-array-elements-at-compile-time/197800525?pgno=1
//
#define ARRAY_LENGTH(arr)                                                                                              \
    (0 * sizeof(reinterpret_cast<const ::Bad_arg_to_ARRAY_LENGTH *>(arr)) +                                            \
     0 * sizeof(::Bad_arg_to_ARRAY_LENGTH::check_type((arr), &(arr))) + sizeof(arr) / sizeof((arr)[0]))

struct Bad_arg_to_ARRAY_LENGTH
{
    class Is_pointer; // incomplete
    class Is_array
    {
    };
    template <typename T> static Is_pointer check_type(const T *, const T *const *);
    static Is_array                         check_type(const void *, const void *);
};

// ----------------------------------------------------------------------------
//
// Color Management Types
//
// ----------------------------------------------------------------------------

// 8-bit palette index
typedef uint8_t palindex_t;

//
// argb_t class
//
// Allows ARGB8888 values to be accessed as a packed 32-bit integer or accessed
// by its individual 8-bit color and alpha channels.
//
class argb_t
{
  public:
    argb_t()
    {
    }
    argb_t(uint32_t _value) : value(_value)
    {
    }

    argb_t(uint8_t _r, uint8_t _g, uint8_t _b)
    {
        seta(255);
        setr(_r);
        setg(_g);
        setb(_b);
    }

    argb_t(uint8_t _a, uint8_t _r, uint8_t _g, uint8_t _b)
    {
        seta(_a);
        setr(_r);
        setg(_g);
        setb(_b);
    }

    inline operator uint32_t() const
    {
        return value;
    }

    inline uint8_t geta() const
    {
        return channels[a_num];
    }

    inline uint8_t getr() const
    {
        return channels[r_num];
    }

    inline uint8_t getg() const
    {
        return channels[g_num];
    }

    inline uint8_t getb() const
    {
        return channels[b_num];
    }

    inline void seta(uint8_t n)
    {
        channels[a_num] = n;
    }

    inline void setr(uint8_t n)
    {
        channels[r_num] = n;
    }

    inline void setg(uint8_t n)
    {
        channels[g_num] = n;
    }

    inline void setb(uint8_t n)
    {
        channels[b_num] = n;
    }

    static void setChannels(uint8_t _a, uint8_t _r, uint8_t _g, uint8_t _b)
    {
        a_num = _a;
        r_num = _r;
        g_num = _g;
        b_num = _b;
    }

  private:
    static uint8_t a_num, r_num, g_num, b_num;

    union {
        uint32_t value;
        uint8_t  channels[4];
    };
};

//
// fargb_t class
//
// Stores ARGB color channels as four floats. This is ideal for color
// manipulation where precision is important.
//
class fargb_t
{
  public:
    fargb_t()
    {
    }
    fargb_t(const argb_t color)
    {
        seta(color.geta() / 255.0f);
        setr(color.getr() / 255.0f);
        setg(color.getg() / 255.0f);
        setb(color.getb() / 255.0f);
    }

    fargb_t(float _r, float _g, float _b)
    {
        seta(1.0f);
        setr(_r);
        setg(_g);
        setb(_b);
    }

    fargb_t(float _a, float _r, float _g, float _b)
    {
        seta(_a);
        setr(_r);
        setg(_g);
        setb(_b);
    }

    inline operator argb_t() const
    {
        return argb_t((uint8_t)(a * 255.0f), (uint8_t)(r * 255.0f), (uint8_t)(g * 255.0f), (uint8_t)(b * 255.0f));
    }

    inline float geta() const
    {
        return a;
    }

    inline float getr() const
    {
        return r;
    }

    inline float getg() const
    {
        return g;
    }

    inline float getb() const
    {
        return b;
    }

    inline void seta(float n)
    {
        a = n;
    }

    inline void setr(float n)
    {
        r = n;
    }

    inline void setg(float n)
    {
        g = n;
    }

    inline void setb(float n)
    {
        b = n;
    }

  private:
    float a, r, g, b;
};

//
// fahsv_t class
//
// Stores AHSV color channels as four floats.
//
class fahsv_t
{
  public:
    fahsv_t()
    {
    }

    fahsv_t(float _h, float _s, float _v)
    {
        seta(1.0f);
        seth(_h);
        sets(_s);
        setv(_v);
    }

    fahsv_t(float _a, float _h, float _s, float _v)
    {
        seta(_a);
        seth(_h);
        sets(_s);
        setv(_v);
    }

    inline float geta() const
    {
        return a;
    }

    inline float geth() const
    {
        return h;
    }

    inline float gets() const
    {
        return s;
    }

    inline float getv() const
    {
        return v;
    }

    inline void seta(float n)
    {
        a = n;
    }

    inline void seth(float n)
    {
        h = n;
    }

    inline void sets(float n)
    {
        s = n;
    }

    inline void setv(float n)
    {
        v = n;
    }

  private:
    float a, h, s, v;
};

// ----------------------------------------------------------------------------
//
// Color Mapping classes
//
// ----------------------------------------------------------------------------

typedef struct
{
    palindex_t *colormap;  // Colormap for 8-bit
    argb_t     *shademap;  // ARGB8888 values for 32-bit
    uint8_t     ramp[256]; // Light fall-off as a function of distance
                           // Light levels: 0 = black, 255 = full bright.
                           // Distance:     0 = near,  255 = far.
} shademap_t;

struct dyncolormap_s;
typedef struct dyncolormap_s dyncolormap_t;

// This represents a clean reference to a map of both 8-bit colors and 32-bit shades.
struct shaderef_t
{
  private:
    const shademap_t *m_colors;                 // The color/shade map to use
    int32_t           m_mapnum;                 // Which index into the color/shade map to use

  public:
    mutable const palindex_t    *m_colormap;    // Computed colormap pointer
    mutable const argb_t        *m_shademap;    // Computed shademap pointer
    mutable const dyncolormap_t *m_dyncolormap; // Auto-discovered dynamic colormap

  public:
    shaderef_t();
    shaderef_t(const shaderef_t &other);
    shaderef_t(const shademap_t *const colors, const int32_t mapnum);

    // Determines if m_colors is NULL
    bool isValid() const;

    shaderef_t with(const int32_t mapnum) const;

    palindex_t        index(const uint8_t c) const;
    argb_t            shade(const uint8_t c) const;
    const shademap_t *map() const;
    int32_t           mapnum() const;
    uint8_t           ramp() const;

    bool operator==(const shaderef_t &other) const;
};

forceinline bool shaderef_t::isValid() const
{
    return m_colors != NULL;
}

forceinline shaderef_t shaderef_t::with(const int32_t mapnum) const
{
    return shaderef_t(m_colors, m_mapnum + mapnum);
}

forceinline palindex_t shaderef_t::index(const palindex_t c) const
{
#if ODAMEX_DEBUG
    if (m_colors == NULL)
        throw CDoomError("shaderef_t::index(): Bad shaderef_t");
    if (m_colors->colormap == NULL)
        throw CDoomError("shaderef_t::index(): colormap == NULL!");
#endif

    return m_colormap[c];
}

forceinline argb_t shaderef_t::shade(const palindex_t c) const
{
#if ODAMEX_DEBUG
    if (m_colors == NULL)
        throw CDoomError("shaderef_t::shade(): Bad shaderef_t");
    if (m_colors->shademap == NULL)
        throw CDoomError("shaderef_t::shade(): shademap == NULL!");
#endif

    return m_shademap[c];
}

forceinline const shademap_t *shaderef_t::map() const
{
    return m_colors;
}

forceinline int32_t shaderef_t::mapnum() const
{
    return m_mapnum;
}

forceinline bool shaderef_t::operator==(const shaderef_t &other) const
{
    return m_colors == other.m_colors && m_mapnum == other.m_mapnum;
}

// NOTE(jsd): Rest of shaderef_t and translationref_t functions implemented in "r_main.h"
// NOTE(jsd): Constructors are implemented in "v_palette.cpp"

// rt_rawcolor does no color mapping and only uses the default palette.
template <typename pixel_t> static forceinline pixel_t rt_rawcolor(const shaderef_t &pal, const uint8_t c);

// rt_mapcolor does color mapping.
template <typename pixel_t> static forceinline pixel_t rt_mapcolor(const shaderef_t &pal, const uint8_t c);

// rt_blend2 does alpha blending between two colors.
template <typename pixel_t>
static forceinline pixel_t rt_blend2(const pixel_t bg, const int32_t bga, const pixel_t fg, const int32_t fga);

template <> forceinline palindex_t rt_rawcolor<palindex_t>(const shaderef_t &pal, const uint8_t c)
{
    // NOTE(jsd): For rawcolor we do no index.
    return (c);
}

template <> forceinline argb_t rt_rawcolor<argb_t>(const shaderef_t &pal, const uint8_t c)
{
    return pal.shade(c);
}

template <> forceinline palindex_t rt_mapcolor<palindex_t>(const shaderef_t &pal, const uint8_t c)
{
    return pal.index(c);
}

template <> forceinline argb_t rt_mapcolor<argb_t>(const shaderef_t &pal, const uint8_t c)
{
    return pal.shade(c);
}