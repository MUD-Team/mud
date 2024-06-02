// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 4f43625aba84e5e6dd10cfff91fbcf664503e5be $
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
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------

#pragma once

#if TARGET_CPU_X86 || TARGET_CPU_X86_64
#ifdef __BIG_ENDIAN__
#undef __BIG_ENDIAN__
#endif
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif
#endif

#if TARGET_CPU_PPC
#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__
#endif
#ifdef __LITTLE_ENDIAN__
#undef __LITTLE_ENDIAN__
#endif
#endif

// Endianess handling.
// WAD files are stored little endian.
#ifdef __BIG_ENDIAN__

// Swap 16bit, that is, MSB and LSB byte.
// No masking with 0xFF should be necessary.

inline static uint16_t LESHORT(uint16_t x)
{
    return (uint16_t)((x >> 8) | (x << 8));
}

inline static int16_t LESHORT(int16_t x)
{
    return (int16_t)((((uint16_t)x) >> 8) | (((uint16_t)x) << 8));
}

// Swapping 32bit.
inline static uint32_t LELONG(uint32_t x)
{
    return (uint32_t)((x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24));
}

inline static int32_t LELONG(int32_t x)
{
    return (int32_t)((((uint32_t)x) >> 24) | ((((uint32_t)x) >> 8) & 0xff00) | ((((uint32_t)x) << 8) & 0xff0000) |
                     (((uint32_t)x) << 24));
}

inline static uint16_t BESHORT(uint16_t x)
{
    return x;
}

inline static int16_t BESHORT(int16_t x)
{
    return x;
}

inline static uint32_t BELONG(uint32_t x)
{
    return x;
}

inline static int32_t BELONG(int32_t x)
{
    return x;
}

#else

inline static uint16_t LESHORT(uint16_t x)
{
    return x;
}

inline static int16_t LESHORT(int16_t x)
{
    return x;
}

inline static uint32_t LELONG(uint32_t x)
{
    return x;
}

inline static int32_t LELONG(int32_t x)
{
    return x;
}

inline static uint16_t BESHORT(uint16_t x)
{
    return (uint16_t)((x >> 8) | (x << 8));
}

inline static int16_t BESHORT(int16_t x)
{
    return (int16_t)((((uint16_t)x) >> 8) | (((uint16_t)x) << 8));
}

inline static uint32_t BELONG(uint32_t x)
{
    return (uint32_t)((x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24));
}

inline static int32_t BELONG(int32_t x)
{
    return (int32_t)((((uint32_t)x) >> 24) | ((((uint32_t)x) >> 8) & 0xff00) | ((((uint32_t)x) << 8) & 0xff0000) |
                     (((uint32_t)x) << 24));
}

#endif
