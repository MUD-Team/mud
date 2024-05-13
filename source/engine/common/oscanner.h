// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 8b998efbc955f6c0494c36cf600539345bc5bb63 $
//
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
//   A better text lump parser, with no global state.
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>

#include "doomtype.h"

struct OScannerConfig
{
    const char *lumpName;
    bool        semiComments;
    bool        cComments;
};

class OScanner
{
    OScannerConfig m_config;
    const char    *m_scriptStart;
    const char    *m_scriptEnd;
    const char    *m_position;
    int32_t            m_lineNumber;
    std::string    m_token;
    bool           m_unScan;
    bool           m_removeEscapeCharacter;
    bool           m_isQuotedString;
    bool           m_crossed;

    bool checkPair(char a, char b);
    void skipWhitespace();
    void skipToNextLine();
    void skipPastPair(char a, char b);
    bool munchQuotedString();
    void munchString();
    void pushToken(const char *string, size_t length);
    void pushToken(const std::string &string);

  public:
    OScanner(const OScannerConfig &config)
        : m_config(config), m_scriptStart(NULL), m_scriptEnd(NULL), m_position(NULL), m_lineNumber(0), m_token(""),
          m_unScan(false), m_removeEscapeCharacter(false), m_isQuotedString(false), m_crossed(false)
    {
    }

    static OScanner openBuffer(const OScannerConfig &config, const char *start, const char *end);

    bool scan();
    void mustScan();
    void mustScanInt();
    void mustScanFloat();
    void mustScanBool();
    void unScan();

    std::string getToken() const;
    int32_t         getTokenInt() const;
    float       getTokenFloat() const;
    bool        getTokenBool() const;

    bool           &crossed();
    bool            isQuotedString() const;
    void            assertTokenIs(const char *string) const;
    bool            compareToken(const char *string) const;
    bool            compareTokenNoCase(const char *string) const;
    void STACK_ARGS warning(const char *message, ...) const;
    void STACK_ARGS error(const char *message, ...) const;
};
