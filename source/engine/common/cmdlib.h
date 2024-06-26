// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: c7041837b4a819ebc3572594ce5e512c772da906 $
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
//	Command library (?)
//
//-----------------------------------------------------------------------------

#pragma once

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include <algorithm>
#include <string>
#include <vector>

#include "doomtype.h"

#ifdef _MSC_VER
#pragma warning(disable : 4244) // MIPS
#pragma warning(disable : 4136) // X86
#pragma warning(disable : 4051) // ALPHA

#pragma warning(disable : 4018) // signed/unsigned mismatch
#pragma warning(disable : 4305) // truncate from double to float
#endif

struct OTimespan
{
    int32_t csecs;
    int32_t tics;
    int32_t seconds;
    int32_t minutes;
    int32_t hours;
    OTimespan() : csecs(0), tics(0), seconds(0), minutes(0), hours(0)
    {
    }
};

int32_t ParseHex(const char *str);
int32_t ParseNum(const char *str);
bool    IsNum(const char *str); // [RH] added
bool    IsRealNum(const char *str);

// [Russell] Returns 0 if strings are the same, optional parameter for case
// sensitivity
bool iequals(const std::string &, const std::string &);

size_t StdStringFind(const std::string &haystack, const std::string &needle, size_t pos, size_t n, bool CIS);

size_t StdStringRFind(const std::string &haystack, const std::string &needle, size_t pos, size_t n, bool CIS);

std::string StdStringToLower(const std::string &, size_t n = std::string::npos);
std::string StdStringToLower(const char *, size_t n = std::string::npos);
std::string StdStringToUpper(const std::string &, size_t n = std::string::npos);
std::string StdStringToUpper(const char *, size_t n = std::string::npos);

std::string &TrimString(std::string &s);
std::string &TrimStringStart(std::string &s);
std::string &TrimStringEnd(std::string &s);

bool ValidString(const std::string &);
bool IsHexString(const std::string &str, const size_t len);

char *COM_Parse(char *data);

extern char com_token[8192];
extern bool com_eof;

char *copystring(const char *s);

std::vector<std::string> VectorArgs(size_t argc, char **argv);
std::string              JoinStrings(const std::vector<std::string> &pieces, const std::string &glue = "");

typedef std::vector<std::string> StringTokens;
StringTokens                     TokenizeString(const std::string &str, const std::string &delim);

FORMAT_PRINTF(2, 3) void STACK_ARGS StrFormat(std::string &out, const char *fmt, ...);
FORMAT_PRINTF(1, 2) std::string STACK_ARGS StrFormat(const char *fmt, ...);
void STACK_ARGS        VStrFormat(std::string &out, const char *fmt, va_list va);
std::string STACK_ARGS VStrFormat(const char *fmt, va_list va);

void StrFormatBytes(std::string &out, size_t bytes);

void TicsToTime(OTimespan &span, int32_t time, bool ceilsec = false);

bool CheckWildcards(const char *pattern, const char *text);
void ReplaceString(char **ptr, const char *str);

void StripColorCodes(std::string &str);

double   Remap(const double value, const double low1, const double high1, const double low2, const double high2);
uint32_t Log2(uint32_t n);
float    NextAfter(const float from, const float to);

/**
 * @brief Initialize an array with a specific value.
 *
 * @tparam A Array type to initialize.
 * @tparam T Value type to initialize with.
 * @param dst Array to initialize.
 * @param val Value to initialize with.
 */
template <typename A, typename T> static void ArrayInit(A &dst, const T &val)
{
    for (size_t i = 0; i < ARRAY_LENGTH(dst); i++)
        dst[i] = val;
}

/**
 * @brief Copy the complete contents of an array from one to the other.
 *
 * @detail Both params are templated in case the destination's type doesn't
 *         line up 100% with the source.
 *
 * @tparam A1 Destination array type.
 * @tparam A2 Source array type.
 * @param dst Destination array to write to.
 * @param src Source array to write from.
 */
template <typename A1, typename A2> static void ArrayCopy(A1 &dst, const A2 &src)
{
    for (size_t i = 0; i < ARRAY_LENGTH(src); i++)
        dst[i] = src[i];
}
