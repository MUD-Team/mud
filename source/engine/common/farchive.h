// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 8c4c25654c5b56e226efb17776a26dcf7f45459d $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	FARCHIVE
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <stdio.h>

#include "dobject.h"
#include "doomtype.h"
#include "physfs.h"

#define FA_RESET (1 << 0)

class DObject;

class FFile
{
  public:
    enum EOpenMode
    {
        EReading,
        EWriting,
        ENotOpen
    };

    enum ESeekPos
    {
        ESeekSet,
        ESeekRelative,
        ESeekEnd
    };

    virtual ~FFile()
    {
    }

    virtual bool      Open(const char *name, EOpenMode mode) = 0;
    virtual void      Close()                                = 0;
    virtual void      Flush()                                = 0;
    virtual EOpenMode Mode() const                           = 0;
    virtual bool      IsPersistent() const                   = 0;
    virtual bool      IsOpen() const                         = 0;

    virtual FFile &Write(const void *, uint32_t) = 0;
    virtual FFile &Read(void *, uint32_t)        = 0;

    virtual uint32_t Tell() const        = 0;
    virtual FFile       &Seek(int32_t, ESeekPos) = 0;
    inline FFile        &Seek(uint32_t i, ESeekPos p)
    {
        return Seek((int32_t)i, p);
    }
};

class FLZOFile : public FFile
{
  public:
    FLZOFile();
    FLZOFile(const char *name, EOpenMode mode, bool dontcompress = false);
    FLZOFile(PHYSFS_File *file, EOpenMode mode, bool dontcompress = false);
    virtual ~FLZOFile();

    virtual bool      Open(const char *name, EOpenMode mode);
    virtual void      Close();
    virtual void      Flush();
    virtual EOpenMode Mode() const;
    virtual bool      IsPersistent() const
    {
        return true;
    }
    virtual bool IsOpen() const;

    virtual FFile       &Write(const void *, uint32_t);
    virtual FFile       &Read(void *, uint32_t);
    virtual uint32_t Tell() const;
    virtual FFile       &Seek(int32_t, ESeekPos);

  protected:
    uint32_t   m_Pos;
    uint32_t   m_BufferSize;
    uint32_t   m_MaxBufferSize;
    uint8_t *m_Buffer;
    bool           m_NoCompress;
    EOpenMode      m_Mode;
    PHYSFS_File   *m_File;

    virtual void Implode();
    virtual void Explode();
    virtual bool FreeOnExplode()
    {
        return true;
    }

  private:
    void clear();
    void PostOpen();
};

class FLZOMemFile : public FLZOFile
{
  public:
    FLZOMemFile();

    virtual ~FLZOMemFile();

    virtual bool Open(const char *name, EOpenMode mode); // Works for reading only
    virtual bool Open(void *memblock);                   // Open for reading only
    virtual bool Open();                                 // Open for writing only
    virtual bool Reopen();                               // Re-opens imploded file for reading only
    virtual void Close();
    virtual bool IsOpen() const;

    void Serialize(FArchive &arc);

    size_t Length() const;
    void   WriteToBuffer(void *buf, size_t length) const;

  protected:
    virtual bool FreeOnExplode()
    {
        return !m_SourceFromMem;
    }

  private:
    bool           m_SourceFromMem;
    uint8_t *m_ImplodedBuffer;
};

class FArchive
{
  public:
    FArchive(FFile &file, uint32_t flags = 0);
    virtual ~FArchive();

    inline bool IsLoading() const
    {
        return m_Loading;
    }
    inline bool IsStoring() const
    {
        return m_Storing;
    }
    inline bool IsPeristent() const
    {
        return m_Persistent;
    }
    inline bool IsReset() const
    {
        return m_Reset;
    }

    void SetHubTravel()
    {
        m_HubTravel = true;
    }

    void Close();

    virtual void Write(const void *mem, uint32_t len);
    virtual void Read(void *mem, uint32_t len);

    void  WriteCount(uint32_t count);
    uint32_t ReadCount();

    FArchive &operator<<(uint8_t c);
    FArchive &operator<<(uint16_t s);
    FArchive &operator<<(uint32_t i);
    FArchive &operator<<(uint64_t i);
    FArchive &operator<<(float f);
    FArchive &operator<<(double d);
    FArchive &operator<<(argb_t color);
    FArchive &operator<<(const char *str);
    FArchive &operator<<(DObject *obj);

    inline FArchive &operator<<(char c)
    {
        return operator<<((uint8_t)c);
    }
    inline FArchive &operator<<(int8_t c)
    {
        return operator<<((uint8_t)c);
    }
    inline FArchive &operator<<(int16_t s)
    {
        return operator<<((uint16_t)s);
    }
    inline FArchive &operator<<(int32_t i)
    {
        return operator<<((uint32_t)i);
    }
    inline FArchive &operator<<(int64_t i)
    {
        return operator<<((uint64_t)i);
    }
    inline FArchive &operator<<(const uint8_t *str)
    {
        return operator<<((const char *)str);
    }
    inline FArchive &operator<<(const int8_t *str)
    {
        return operator<<((const char *)str);
    }
    inline FArchive &operator<<(bool b)
    {
        return operator<<((uint8_t)b);
    }

    FArchive &operator>>(uint8_t &c);
    FArchive &operator>>(uint16_t &s);
    FArchive &operator>>(uint32_t &i);
    FArchive &operator>>(uint64_t &i);
    FArchive &operator>>(float &f);
    FArchive &operator>>(double &d);
    FArchive &operator>>(argb_t &color);
    FArchive &operator>>(std::string &s);
    FArchive &ReadObject(DObject *&obj, TypeInfo *wanttype);

    inline FArchive &operator>>(char &c)
    {
        uint8_t in;
        operator>>(in);
        c = (char)in;
        return *this;
    }
    inline FArchive &operator>>(int8_t &c)
    {
        uint8_t in;
        operator>>(in);
        c = (int8_t)in;
        return *this;
    }
    inline FArchive &operator>>(int16_t &s)
    {
        uint16_t in;
        operator>>(in);
        s = (int16_t)in;
        return *this;
    }
    inline FArchive &operator>>(int32_t &i)
    {
        uint32_t in;
        operator>>(in);
        i = (int32_t)in;
        return *this;
    }
    inline FArchive &operator>>(int64_t &i)
    {
        uint64_t in;
        operator>>(in);
        i = (int64_t)in;
        return *this;
    }
    inline FArchive &operator>>(bool &b)
    {
        uint8_t in;
        operator>>(in);
        b = (in != 0);
        return *this;
    }
    inline FArchive &operator>>(DObject *&object)
    {
        return ReadObject(object, RUNTIME_CLASS(DObject));
    }

  protected:
    enum
    {
        EObjectHashSize = 137
    };

    uint32_t           FindObjectIndex(const DObject *obj) const;
    uint32_t           MapObject(const DObject *obj);
    uint32_t           WriteClass(const TypeInfo *info);
    const TypeInfo *ReadClass();
    const TypeInfo *ReadClass(const TypeInfo *wanttype);
    const TypeInfo *ReadStoredClass(const TypeInfo *wanttype);
    uint32_t           HashObject(const DObject *obj) const;

    bool   m_Persistent;  // meant for persistent storage (disk)?
    bool   m_Loading;     // extracting objects?
    bool   m_Storing;     // inserting objects?
    bool   m_HubTravel;   // travelling inside a hub?
    bool   m_Reset;       // reset state?
    FFile *m_File;        // unerlying file object
    uint32_t  m_ObjectCount; // # of objects currently serialized
    uint32_t  m_MaxObjectCount;
    uint32_t  m_ClassCount;  // # of unique classes currently serialized

    struct TypeMap
    {
        const TypeInfo *toCurrent; // maps archive type index to execution type index
        uint32_t           toArchive; // maps execution type index to archive type index

        enum
        {
            NO_INDEX = 0xffffffff
        };
    } *m_TypeMap;

    struct ObjectMap
    {
        const DObject *object;
        size_t         hashNext;
    }     *m_ObjectMap;
    size_t m_ObjectHash[EObjectHashSize];

  private:
    FArchive(const FArchive &src)
    {
    }
    void operator=(const FArchive &src)
    {
    }
};

class player_s;

FArchive &operator<<(FArchive &arc, player_s *p);
FArchive &operator>>(FArchive &arc, player_s *&p);
