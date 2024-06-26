// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: e78b683435ada6907e853b4c46aa06477b668194 $
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
//	FARCHIVE
//
//-----------------------------------------------------------------------------

#include "farchive.h"

#include <algorithm>

#include "Poco/Buffer.h"
#include "Poco/ByteOrder.h"
#include "d_player.h"
#include "dobject.h"
#include "i_system.h"
#include "m_alloc.h"
#include "minilzo.h"
#include "mud_includes.h"
#include "physfs.h"

#ifdef __BIG_ENDIAN__
#define SWAP_WORD(x)
#define SWAP_DWORD(x)
#define SWAP_QWORD(x)
#define SWAP_SIZE(x, y)
#else
#define SWAP_WORD(x)                                                                                                   \
    {                                                                                                                  \
        x = (((x) << 8) | ((x) >> 8));                                                                                 \
    }
#define SWAP_DWORD(x)                                                                                                  \
    {                                                                                                                  \
        x = (((x) >> 24) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) | ((x) << 24));                             \
    }
// Swap any kind of data based on size - x = pointer to data, y = number of bytes
#define SWAP_SIZE(x, y)                                                                                                \
    {                                                                                                                  \
        std::reverse((uint8_t *)x, (uint8_t *)x + (size_t)y);                                                          \
    }
#define SWAP_QWORD(x)                                                                                                  \
    {                                                                                                                  \
        uint32_t *y = (uint32_t *)&x;                                                                                  \
        uint32_t  t = y[0];                                                                                            \
        y[0]        = y[1];                                                                                            \
        y[1]        = t;                                                                                               \
        SWAP_DWORD(y[0]);                                                                                              \
        SWAP_DWORD(y[1]);                                                                                              \
    }
#endif

#define MAX(a, b) ((a) < (b) ? (a) : (b))

static const char LZOSig[4] = {'F', 'L', 'Z', 'O'};

// Output buffer size for LZO compression, extra space in case uncompressable
static uint32_t MaxLZOCompressedLength(uint32_t input_len)
{
    return input_len + input_len / 16 + 64 + 3;
}

void FLZOFile::clear()
{
    m_Pos           = 0;
    m_BufferSize    = 0;
    m_MaxBufferSize = 0;
    m_Buffer        = NULL;
    m_File          = NULL;
    m_NoCompress    = false;
    m_Mode          = ENotOpen;
}

FLZOFile::FLZOFile()
{
    clear();
}

FLZOFile::FLZOFile(const char *name, EOpenMode mode, bool dontCompress)
{
    clear();
    m_NoCompress = dontCompress;

    Open(name, mode);
}

FLZOFile::FLZOFile(PHYSFS_File *file, EOpenMode mode, bool dontCompress)
{
    clear();
    m_Mode       = mode;
    m_File       = file;
    m_NoCompress = dontCompress;

    PostOpen();
}

FLZOFile::~FLZOFile()
{
    Close();
}

bool FLZOFile::Open(const char *name, EOpenMode mode)
{
    Close();
    if (name == NULL)
        return false;
    m_Mode = mode;
    if (mode == EReading)
        m_File = PHYSFS_openRead(name);
    else
        m_File = PHYSFS_openWrite(name);
    PostOpen();
    return !!m_File;
}

void FLZOFile::PostOpen()
{
    if (m_File && m_Mode == EReading)
    {
        char   sig[4];
        size_t readlen = PHYSFS_readBytes(m_File, sig, 4);
        if (readlen < 4)
        {
            printf("FLZOFile::PostOpen(): failed to read m_File\n");
        }
        if (sig[0] != LZOSig[0] || sig[1] != LZOSig[1] || sig[2] != LZOSig[2] || sig[3] != LZOSig[3])
        {
            PHYSFS_close(m_File);
            m_File = NULL;
        }
        else
        {
            uint32_t sizes[2];
            readlen = PHYSFS_readBytes(m_File, sizes, sizeof(uint32_t) * 2);
            if (readlen < sizeof(uint32_t) * 2)
            {
                printf("FLZOFile::PostOpen(): failed to read m_File\n");
            }
            SWAP_DWORD(sizes[0]);
            SWAP_DWORD(sizes[1]);

            uint32_t len = sizes[0] == 0 ? sizes[1] : sizes[0];
            m_Buffer     = (uint8_t *)Malloc(len + 8);

            readlen = PHYSFS_readBytes(m_File, m_Buffer + 8, len);
            if (readlen < len)
            {
                printf("FLZOFile::PostOpen(): failed to read m_File\n");
            }

            SWAP_DWORD(sizes[0]);
            SWAP_DWORD(sizes[1]);

            ((uint32_t *)m_Buffer)[0] = sizes[0];
            ((uint32_t *)m_Buffer)[1] = sizes[1];
            Explode();
        }
    }
}

void FLZOFile::Close()
{
    if (m_File)
    {
        if (m_Mode == EWriting)
        {
            Implode();
            PHYSFS_writeBytes(m_File, LZOSig, 4);
            PHYSFS_writeBytes(m_File, m_Buffer, m_BufferSize + 8);
        }
        PHYSFS_close(m_File);
        m_File = NULL;
    }

    M_Free(m_Buffer);

    clear();
}

void FLZOFile::Flush()
{
}

FFile::EOpenMode FLZOFile::Mode() const
{
    return m_Mode;
}

bool FLZOFile::IsOpen() const
{
    return !!m_File;
}

FFile &FLZOFile::Write(const void *mem, uint32_t len)
{
    if (m_Mode != EWriting)
    {
        I_Error("Tried to write to reading LZO file\n");
        return *this;
    }

    if (m_Pos + len > m_BufferSize)
    {
        do
        {
            m_BufferSize = m_MaxBufferSize = m_BufferSize ? m_BufferSize * 2 : 16384;
        } while (m_Pos + len > m_BufferSize);

        m_Buffer = (uint8_t *)Realloc(m_Buffer, m_BufferSize);
    }

    if (len == 1)
        m_Buffer[m_Pos] = *(uint8_t *)mem;
    else
        memcpy(m_Buffer + m_Pos, mem, len);

    m_Pos += len;
    if (m_Pos > m_BufferSize)
        m_BufferSize = m_Pos;

    return *this;
}

FFile &FLZOFile::Read(void *mem, uint32_t len)
{
    if (m_Mode != EReading)
    {
        I_Error("Tried to read from writing LZO file\n");
        return *this;
    }

    if (m_Pos + len > m_BufferSize)
    {
        I_Error("Attempt to read past end of LZO file\n");
        return *this;
    }

    if (len == 1)
        *(uint8_t *)mem = m_Buffer[m_Pos];
    else
        memcpy(mem, m_Buffer + m_Pos, len);

    m_Pos += len;

    return *this;
}

uint32_t FLZOFile::Tell() const
{
    return m_Pos;
}

FFile &FLZOFile::Seek(int32_t pos, ESeekPos ofs)
{
    if (ofs == ESeekRelative)
        pos += m_Pos;
    else if (ofs == ESeekEnd)
        pos = m_BufferSize - pos;

    if (pos < 0)
        m_Pos = 0;
    else if ((uint32_t)pos > m_BufferSize)
        m_Pos = m_BufferSize;
    else
        m_Pos = pos;

    return *this;
}

void FLZOFile::Implode()
{
    uint32_t input_len      = m_BufferSize;
    lzo_uint compressed_len = 0;
    uint8_t *compressed     = NULL;

    uint8_t *oldbuf = m_Buffer;

    if (!m_NoCompress)
    {
        compressed = new lzo_byte[MaxLZOCompressedLength(input_len)];

        lzo_byte *wrkmem = new lzo_byte[LZO1X_1_MEM_COMPRESS];
        int32_t   res    = lzo1x_1_compress(m_Buffer, input_len, compressed, &compressed_len, wrkmem);
        delete[] wrkmem;

        // If the data could not be compressed, store it as-is.
        if (res != LZO_E_OK || compressed_len > input_len)
        {
            DPrintf("LZOFile could not be imploded\n");
            compressed_len = 0;
        }
        else
        {
            // A comment inside LZO says "lzo_uint must match size_t".
            DPrintf("LZOFile shrunk from %u to %zu bytes\n", input_len, compressed_len);
        }
    }

    if (compressed_len == 0 || !compressed)
        m_BufferSize = m_MaxBufferSize = input_len;
    else
        m_BufferSize = m_MaxBufferSize = compressed_len;

    m_Buffer = (uint8_t *)Malloc(m_BufferSize + 8);
    m_Pos    = 0;

    ((uint32_t *)m_Buffer)[0] = Poco::ByteOrder::fromBigEndian((uint32_t)compressed_len);
    ((uint32_t *)m_Buffer)[1] = Poco::ByteOrder::fromBigEndian((uint32_t)input_len);

    if (compressed_len == 0 || !compressed)
        memcpy(m_Buffer + 8, oldbuf, input_len);
    else
        memcpy(m_Buffer + 8, compressed, compressed_len);

    delete[] compressed;
    compressed = NULL;

    M_Free(oldbuf);
}

void FLZOFile::Explode()
{
    if (m_Buffer)
    {
        uint32_t compressed_len = Poco::ByteOrder::fromBigEndian(((uint32_t *)m_Buffer)[0]);
        uint32_t expanded_len   = Poco::ByteOrder::fromBigEndian(((uint32_t *)m_Buffer)[1]);

        uint8_t *expanded_buffer = (uint8_t *)Malloc(expanded_len);

        if (compressed_len != 0)
        {
            lzo_uint newlen = expanded_len;

            int32_t res = lzo1x_decompress_safe(m_Buffer + 8, compressed_len, expanded_buffer, &newlen, NULL);
            if (res != LZO_E_OK || newlen != expanded_len)
            {
                M_Free(expanded_buffer);
                I_Error("Could not decompress LZO file");
            }
        }
        else
        {
            memcpy(expanded_buffer, m_Buffer + 8, expanded_len);
        }

        if (FreeOnExplode())
        {
            M_Free(m_Buffer);
        }

        m_Buffer     = expanded_buffer;
        m_BufferSize = expanded_len;
    }
}

FLZOMemFile::FLZOMemFile() : FLZOFile()
{
    m_SourceFromMem  = false;
    m_ImplodedBuffer = NULL;
}

FLZOMemFile::~FLZOMemFile()
{
}

bool FLZOMemFile::Open(const char *name, EOpenMode mode)
{
    if (mode == EWriting)
    {
        if (name)
            I_Error("FLZOMemFile cannot write to disk");
        else
            return Open();
    }
    else
    {
        bool res = FLZOFile::Open(name, EReading);
        if (res)
        {
            PHYSFS_close(m_File);
            m_File = NULL;
        }
        return res;
    }
    return false;
}

bool FLZOMemFile::Open(void *memblock)
{
    // [SL] TODO: what is m_BufferSize?!?
    Close();
    m_Mode          = EReading;
    m_Buffer        = (uint8_t *)memblock;
    m_SourceFromMem = true;
    Explode();
    m_SourceFromMem = false;
    return !!m_Buffer;
}

bool FLZOMemFile::Open()
{
    Close();
    m_Mode          = EWriting;
    m_BufferSize    = 0;
    m_MaxBufferSize = 16384;
    m_Buffer        = (uint8_t *)Malloc(16384);
    m_Pos           = 0;
    return true;
}

bool FLZOMemFile::Reopen()
{
    if (m_Buffer == NULL && m_ImplodedBuffer)
    {
        m_Mode          = EReading;
        m_Buffer        = m_ImplodedBuffer;
        m_SourceFromMem = true;
        Explode();
        m_SourceFromMem = false;
        return true;
    }
    return false;
}

void FLZOMemFile::Close()
{
    if (m_Mode == EWriting)
    {
        FLZOFile::Implode();
        m_ImplodedBuffer = m_Buffer;
        m_Buffer         = NULL;
    }
}

void FLZOMemFile::Serialize(FArchive &arc)
{
    if (arc.IsStoring())
    {
        if (m_ImplodedBuffer == NULL)
        {
            I_Error("FLZOMemFile must be imploded before storing\n");
            // Q: How do we get here without closing FLZOMemFile first?
            Close();
        }
        arc.Write(LZOSig, 4);

        uint32_t sizes[2];
        sizes[0] = ((uint32_t *)m_ImplodedBuffer)[0];
        sizes[1] = ((uint32_t *)m_ImplodedBuffer)[1];
        SWAP_DWORD(sizes[0]);
        SWAP_DWORD(sizes[1]);
        arc.Write(m_ImplodedBuffer, (sizes[0] ? sizes[0] : sizes[1]) + 8);
    }
    else
    {
        Close();
        m_Mode = EReading;

        char     sig[4];
        uint32_t sizes[2];

        arc.Read(sig, 4);

        if (sig[0] != LZOSig[0] || sig[1] != LZOSig[1] || sig[2] != LZOSig[2] || sig[3] != LZOSig[3])
            I_Error("Expected to extract an LZO-compressed file\n");

        arc >> sizes[0] >> sizes[1];
        uint32_t len = sizes[0] == 0 ? sizes[1] : sizes[0];

        m_Buffer = (uint8_t *)Malloc(len + 8);
        SWAP_DWORD(sizes[0]);
        SWAP_DWORD(sizes[1]);
        ((uint32_t *)m_Buffer)[0] = sizes[0];
        ((uint32_t *)m_Buffer)[1] = sizes[1];
        arc.Read(m_Buffer + 8, len);
        m_ImplodedBuffer = m_Buffer;
        m_Buffer         = NULL;
        m_Mode           = EWriting;
    }
}

bool FLZOMemFile::IsOpen() const
{
    return !!m_Buffer;
}

size_t FLZOMemFile::Length() const
{
    return m_BufferSize + 8;
}

void FLZOMemFile::WriteToBuffer(void *buf, size_t length) const
{
    length = length < (m_BufferSize + 8) ? length : (m_BufferSize + 8);

    if (m_ImplodedBuffer)
        memcpy(buf, m_ImplodedBuffer, length);
    else
        memcpy(buf, m_Buffer, length);
}

//============================================
//
// FArchive
//
//============================================

FArchive::FArchive(FFile &file, uint32_t flags)
{
    int32_t i;

    m_Reset          = flags & FA_RESET;
    m_HubTravel      = false;
    m_File           = &file;
    m_MaxObjectCount = m_ObjectCount = 0;
    m_ObjectMap                      = NULL;

    if (file.Mode() == FFile::EReading)
    {
        m_Loading = true;
        m_Storing = false;
    }
    else
    {
        m_Loading = false;
        m_Storing = true;
    }

    m_Persistent = file.IsPersistent();

    m_TypeMap = new TypeMap[TypeInfo::m_NumTypes];
    for (i = 0; i < TypeInfo::m_NumTypes; i++)
    {
        m_TypeMap[i].toArchive = ~0;
        m_TypeMap[i].toCurrent = NULL;
    }

    m_ClassCount = 0;

    for (i = 0; i < EObjectHashSize; i++)
        m_ObjectHash[i] = ~0;
}

FArchive::~FArchive()
{
    Close();

    delete[] m_TypeMap;
    m_TypeMap = NULL;

    if (m_ObjectMap)
    {
        M_Free(m_ObjectMap);
    }
}

void FArchive::Write(const void *mem, uint32_t len)
{
    m_File->Write(mem, len);
}

void FArchive::Read(void *mem, uint32_t len)
{
    m_File->Read(mem, len);
}

void FArchive::Close()
{
    if (m_File)
    {
        m_File->Close();
        m_File = NULL;
    }
}

void FArchive::WriteCount(uint32_t count)
{
    // [AM] Hoisted out of loop due to MSVC/ASan detecting as
    //      use-after-scope.
    uint8_t out = 0;
    do
    {
        out = count & 0x7f;
        if (count >= 0x80)
            out |= 0x80;
        Write(&out, sizeof(uint8_t));
        count >>= 7;
    } while (count);
}

uint32_t FArchive::ReadCount()
{
    uint8_t  in;
    uint32_t count = 0;
    int32_t  ofs   = 0;

    do
    {
        Read(&in, sizeof(uint8_t));
        count |= (in & 0x7f) << ofs;
        ofs += 7;
    } while (in & 0x80);

    return count;
}

FArchive &FArchive::operator<<(const char *str)
{
    if (str == NULL)
    {
        WriteCount(0);
    }
    else
    {
        uint32_t size = strlen(str) + 1;
        WriteCount(size);
        Write(str, size - 1);
    }
    return *this;
}

FArchive &FArchive::operator>>(std::string &s)
{
    uint32_t size = ReadCount();
    if (size == 0)
        s = "";
    else
    {
        Poco::Buffer<char> cstr(size);
        size--;
        Read(cstr.begin(), size);
        cstr[size] = 0;
        s          = cstr.begin();
    }
    return *this;
}

FArchive &FArchive::operator<<(uint8_t c)
{
    Write(&c, sizeof(uint8_t));
    return *this;
}

FArchive &FArchive::operator>>(uint8_t &c)
{
    Read(&c, sizeof(uint8_t));
    return *this;
}

FArchive &FArchive::operator<<(uint16_t w)
{
    SWAP_WORD(w);
    Write(&w, sizeof(uint16_t));
    return *this;
}

FArchive &FArchive::operator>>(uint16_t &w)
{
    Read(&w, sizeof(uint16_t));
    SWAP_WORD(w);
    return *this;
}

FArchive &FArchive::operator<<(uint32_t w)
{
    SWAP_DWORD(w);
    Write(&w, sizeof(uint32_t));
    return *this;
}

FArchive &FArchive::operator>>(uint32_t &w)
{
    Read(&w, sizeof(uint32_t));
    SWAP_DWORD(w);
    return *this;
}

FArchive &FArchive::operator<<(uint64_t w)
{
    SWAP_QWORD(w);
    Write(&w, sizeof(uint64_t));
    return *this;
}

FArchive &FArchive::operator>>(uint64_t &w)
{
    Read(&w, sizeof(uint64_t));
    SWAP_QWORD(w);
    return *this;
}

FArchive &FArchive::operator<<(float w)
{
    SWAP_SIZE(&w, sizeof(float));
    Write(&w, sizeof(float));
    return *this;
}

FArchive &FArchive::operator>>(float &w)
{
    Read(&w, sizeof(float));
    SWAP_SIZE(&w, sizeof(float));
    return *this;
}

FArchive &FArchive::operator<<(double w)
{
    SWAP_SIZE(&w, sizeof(double));
    Write(&w, sizeof(double));
    return *this;
}

FArchive &FArchive::operator>>(double &w)
{
    Read(&w, sizeof(double));
    SWAP_SIZE(&w, sizeof(double));
    return *this;
}

FArchive &FArchive::operator<<(argb_t color)
{
    uint8_t a = color.geta(), r = color.getr(), g = color.getg(), b = color.getb();
    Write(&b, 1);
    Write(&g, 1);
    Write(&r, 1);
    Write(&a, 1);
    return *this;
}

FArchive &FArchive::operator>>(argb_t &color)
{
    uint8_t a, r, g, b;
    Read(&b, 1);
    Read(&g, 1);
    Read(&r, 1);
    Read(&a, 1);
    color = argb_t(a, r, g, b);
    return *this;
}

#define NEW_OBJ          ((uint8_t)1)
#define NEW_CLS_OBJ      ((uint8_t)2)
#define OLD_OBJ          ((uint8_t)3)
#define NULL_OBJ         ((uint8_t)4)
#define NEW_PLYR_OBJ     ((uint8_t)5)
#define NEW_PLYR_CLS_OBJ ((uint8_t)6)

FArchive &FArchive::operator<<(DObject *obj)
{
    player_t *player;

    if (obj == NULL)
    {
        operator<<(NULL_OBJ);
    }
    else
    {
        const TypeInfo *type = RUNTIME_TYPE(obj);

        if (type == RUNTIME_CLASS(DObject))
        {
            // I_Error ("Tried to save an instance of DObject.\n"
            //		 "This should not happen.\n");
            operator<<(NULL_OBJ);
        }
        else if (m_TypeMap[type->TypeIndex].toArchive == (uint32_t)~0)
        {
            // No instances of this class have been written out yet.
            // Write out the class, then write out the object. If this
            // is an actor controlled by a player, make note of that
            // so that it can be overridden when moving around in a hub.
            if (obj->IsKindOf(RUNTIME_CLASS(AActor)) && (player = static_cast<AActor *>(obj)->player) &&
                player->mo == obj)
            {
                operator<<(NEW_PLYR_CLS_OBJ);
                operator<<((uint8_t)(player->id));
            }
            else
            {
                operator<<(NEW_CLS_OBJ);
            }
            WriteClass(type);
            MapObject(obj);
            obj->Serialize(*this);
        }
        else
        {
            // An instance of this class has already been saved. If
            // this object has already been written, save a reference
            // to the saved object. Otherwise, save a reference to the
            // class, then save the object. Again, if this is a player-
            // controlled actor, remember that.
            uint32_t index = FindObjectIndex(obj);

            if (index == (uint32_t)~0)
            {
                if (obj->IsKindOf(RUNTIME_CLASS(AActor)) && (player = static_cast<AActor *>(obj)->player) &&
                    player->mo == obj)
                {
                    operator<<(NEW_PLYR_OBJ);
                    operator<<((uint8_t)(player->id));
                }
                else
                {
                    operator<<(NEW_OBJ);
                }
                WriteCount(m_TypeMap[type->TypeIndex].toArchive);
                MapObject(obj);
                obj->Serialize(*this);
            }
            else
            {
                operator<<(OLD_OBJ);
                WriteCount(index);
            }
        }
    }
    return *this;
}

FArchive &FArchive::ReadObject(DObject *&obj, TypeInfo *wanttype)
{
    uint8_t         objHead;
    const TypeInfo *type;
    uint8_t         playerNum;
    uint32_t        index;

    operator>>(objHead);

    switch (objHead)
    {
    case NULL_OBJ:
        obj = NULL;
        break;

    case OLD_OBJ:
        index = ReadCount();
        if (index >= m_ObjectCount)
        {
            I_Error("Object reference too high (%u; max is %u)\n", index, m_ObjectCount);
        }
        obj = (DObject *)m_ObjectMap[index].object;
        break;

    case NEW_PLYR_CLS_OBJ:
        operator>>(playerNum);
        if (m_HubTravel)
        {
            // If travelling inside a hub, use the existing player actor
            type = ReadClass(wanttype);
            idplayer(playerNum).mo.init(new AActor());
            obj = idplayer(playerNum).mo;
            MapObject(obj);

            // But also create a new one so that we can get past the one
            // stored in the archive.
            DObject *tempobj = type->CreateNew();
            tempobj->Serialize(*this);
            tempobj->Destroy();
            break;
        }
        /* fallthrough */
    case NEW_CLS_OBJ:
        type = ReadClass(wanttype);
        obj  = type->CreateNew();
        MapObject(obj);
        obj->Serialize(*this);
        break;

    case NEW_PLYR_OBJ:
        operator>>(playerNum);
        if (m_HubTravel)
        {
            type = ReadStoredClass(wanttype);
            idplayer(playerNum).mo.init(new AActor());
            obj = idplayer(playerNum).mo;
            MapObject(obj);

            DObject *tempobj = type->CreateNew();
            tempobj->Serialize(*this);
            tempobj->Destroy();
            break;
        }
        /* fallthrough */
    case NEW_OBJ:
        type = ReadStoredClass(wanttype);
        obj  = type->CreateNew();
        MapObject(obj);
        obj->Serialize(*this);
        break;

    default:
        I_Error("Unknown object code (%d) in archive\n", objHead);
    }
    return *this;
}

uint32_t FArchive::WriteClass(const TypeInfo *info)
{
    if (m_ClassCount >= TypeInfo::m_NumTypes)
    {
        I_Error("Too many unique classes have been written.\nOnly %u were registered\n", TypeInfo::m_NumTypes);
    }
    if (m_TypeMap[info->TypeIndex].toArchive != (uint32_t)~0)
    {
        I_Error("Attempt to write '%s' twice.\n", info->Name);
    }
    m_TypeMap[info->TypeIndex].toArchive = m_ClassCount;
    m_TypeMap[m_ClassCount].toCurrent    = info;
    operator<<(info->Name);
    return m_ClassCount++;
}

const TypeInfo *FArchive::ReadClass()
{
    std::string typeName;
    int32_t     i;

    if (m_ClassCount >= TypeInfo::m_NumTypes)
    {
        I_Error("Too many unique classes have been read.\nOnly %u were registered\n", TypeInfo::m_NumTypes);
    }
    operator>>(typeName);
    for (i = 0; i < TypeInfo::m_NumTypes; i++)
    {
        if (!strcmp(TypeInfo::m_Types[i]->Name, typeName.c_str()))
        {
            m_TypeMap[i].toArchive            = m_ClassCount;
            m_TypeMap[m_ClassCount].toCurrent = TypeInfo::m_Types[i];
            m_ClassCount++;
            return TypeInfo::m_Types[i];
        }
    }
    if (typeName.length())
        I_Error("Unknown class '%s'\n", typeName.c_str());
    else
        I_Error("Unknown class\n");
    return NULL;
}

const TypeInfo *FArchive::ReadClass(const TypeInfo *wanttype)
{
    const TypeInfo *type = ReadClass();
    if (!type->IsDescendantOf(wanttype))
    {
        I_Error("Expected to extract an object of type '%s'.\n"
                "Found one of type '%s' instead.\n",
                wanttype->Name, type->Name);
    }
    return type;
}

const TypeInfo *FArchive::ReadStoredClass(const TypeInfo *wanttype)
{
    uint32_t index = ReadCount();
    if (index >= m_ClassCount)
    {
        I_Error("Class reference too high (%u; max is %u)\n", index, m_ClassCount);
    }
    const TypeInfo *type = m_TypeMap[index].toCurrent;
    if (!type->IsDescendantOf(wanttype))
    {
        I_Error("Expected to extract an object of type '%s'.\n"
                "Found one of type '%s' instead.\n",
                wanttype->Name, type->Name);
    }
    return type;
}

uint32_t FArchive::MapObject(const DObject *obj)
{
    uint32_t i;

    if (m_ObjectCount >= m_MaxObjectCount)
    {
        m_MaxObjectCount = m_MaxObjectCount ? m_MaxObjectCount * 2 : 1024;
        m_ObjectMap      = (ObjectMap *)Realloc(m_ObjectMap, sizeof(ObjectMap) * m_MaxObjectCount);
        for (i = m_ObjectCount; i < m_MaxObjectCount; i++)
        {
            m_ObjectMap[i].hashNext = (uint32_t)~0;
            m_ObjectMap[i].object   = NULL;
        }
    }

    uint32_t index = m_ObjectCount++;
    uint32_t hash  = HashObject(obj);

    m_ObjectMap[index].object   = obj;
    m_ObjectMap[index].hashNext = m_ObjectHash[hash];
    m_ObjectHash[hash]          = index;

    return index;
}

uint32_t FArchive::HashObject(const DObject *obj) const
{
    return (uint32_t)((size_t)obj % EObjectHashSize);
}

uint32_t FArchive::FindObjectIndex(const DObject *obj) const
{
    if (!m_ObjectMap)
        return ~0;
    uint32_t index = m_ObjectHash[HashObject(obj)];
    while (index != (uint32_t)~0 && m_ObjectMap[index].object != obj)
    {
        index = m_ObjectMap[index].hashNext;
    }
    return index;
}

FArchive &operator<<(FArchive &arc, player_s *p)
{
    if (p)
        return arc << (uint8_t)(p->id);
    else
        return arc << (uint8_t)0xff;
}

FArchive &operator>>(FArchive &arc, player_s *&p)
{
    uint8_t ofs;
    arc >> ofs;
    if (ofs == 0xff)
        p = NULL;
    else
    {
        if (validplayer(idplayer(ofs)))
            p = &idplayer(ofs);
        else
            p = NULL;
    }
    return arc;
}

VERSION_CONTROL(farchive_cpp, "$Id: e78b683435ada6907e853b4c46aa06477b668194 $")
