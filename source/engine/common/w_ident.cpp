// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 815671388f73da7998e41dacaf22ab40089c2a94 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//
// Resource file identification
//
//-----------------------------------------------------------------------------

#include "w_ident.h"

#include "cmdlib.h"
#include "gi.h"
#include "hashtable.h"
#include "m_fileio.h"
#include "m_ostring.h"
#include "odamex.h"
#include "sarray.h"
#include "w_wad.h"

static const uint32_t IDENT_NONE       = 0;
static const uint32_t IDENT_COMMERCIAL = BIT(0);
static const uint32_t IDENT_IWAD       = BIT(1);
static const uint32_t IDENT_DEPRECATED = BIT(2);

#define FREEDOOM1_PREFIX "Freedoom: Phase 1"
#define FREEDOOM2_PREFIX "Freedoom: Phase 2"
#define FREEDM_PREFIX    "FreeDM"

// ============================================================================
//
// WadFileLumpFinder
//
// Opens a WAD file and checks for the existence of specified lumps.
//
// ============================================================================

class WadFileLumpFinder
{
  public:
    WadFileLumpFinder(const std::string &filename) : mNumLumps(0), mLumps(NULL)
    {
        PHYSFS_File *fp = PHYSFS_openRead(filename.c_str());
        if (fp)
        {
            wadinfo_t header;
            if (PHYSFS_readBytes(fp, &header, sizeof(header)) == sizeof(header))
            {
                header.identification = LELONG(header.identification);
                header.infotableofs   = LELONG(header.infotableofs);

                if (header.identification == IWAD_ID || header.identification == PWAD_ID)
                {
                    if (PHYSFS_seek(fp, header.infotableofs) != 0)
                    {
                        mNumLumps = LELONG(header.numlumps);
                        mLumps    = new filelump_t[mNumLumps];

                        if (PHYSFS_readBytes(fp, mLumps, mNumLumps * sizeof(*mLumps)) != mNumLumps * sizeof(*mLumps))
                            mNumLumps = 0;
                    }
                }
            }

            PHYSFS_close(fp);
        }
    }

    ~WadFileLumpFinder()
    {
        if (mLumps)
            delete[] mLumps;
    }

    bool exists(const std::string &lumpname)
    {
        for (size_t i = 0; i < mNumLumps; i++)
            if (iequals(lumpname, std::string(mLumps[i].name, 8)))
                return true;
        return false;
    }

  private:
    size_t      mNumLumps;
    filelump_t *mLumps;
};

// ============================================================================
//
// FileIdentificationManager
//
// Class to identify known IWAD/PWAD resource files
//
// ============================================================================

class FileIdentificationManager
{
  public:
    FileIdentificationManager() : mIdentifiers(64)
    {
    }

    //
    // FileIdentificationManager::addFile
    //
    // Adds identification information for a known file.
    //
    void addFile(const OString &idname, const OString &filename, const OString &crc32, const OString &md5,
                 const OString &group, bool commercial, const bool iwad, const bool deprecated, const int weight)
    {
        OCRC32Sum crc32Hash;
        OCRC32Sum::makeFromHexStr(crc32Hash, crc32);
        OMD5Hash md5Hash;
        OMD5Hash::makeFromHexStr(md5Hash, md5);

        IdType            id   = mIdentifiers.insert();
        fileIdentifier_t *file = &mIdentifiers.get(id);
        file->mIdName          = OStringToUpper(idname);
        file->mNiceName        = idname;
        file->mFilename        = OStringToUpper(filename);
        file->mIsIWAD          = iwad;

        // add the filename to the IWAD search list if it's not already in there
        if (std::find(mIWADSearchOrder.begin(), mIWADSearchOrder.end(), file->mFilename) == mIWADSearchOrder.end())
            mIWADSearchOrder.push_back(file->mFilename);
    }

    std::vector<OString> getFilenames() const
    {
        return mIWADSearchOrder;
    }

    bool isKnownIWADFilename(const std::string &filename) const
    {
        OString upper = StdStringToUpper(filename);
        for (IdentifierTable::const_iterator it = mIdentifiers.begin(); it != mIdentifiers.end(); ++it)
        {
            if (it->mIsIWAD && it->mFilename == upper)
                return true;
        }
        return false;
    }

    const OString identify(const OResFile &file)
    {
        // This function for now is severely crippled and will work under the
        // assumption that we are only loading Freedoom 1 or 2 - Dasho

        static const int  NUM_CHECKLUMPS                = 2;
        static const char checklumps[NUM_CHECKLUMPS][8] = {
            {'E', '1', 'M', '1'},                     // 0
            {'M', 'A', 'P', '0', '1'},                // 1
        };

        bool lumpsfound[NUM_CHECKLUMPS] = {0};

        WadFileLumpFinder lumps(file.getBasename());
        for (int i = 0; i < NUM_CHECKLUMPS; i++)
            if (lumps.exists(std::string(checklumps[i], 8)))
                lumpsfound[i] = true;

        if (lumpsfound[0]) // E1M1
            return OStringToUpper(OString(FREEDOOM1_PREFIX));
        else if (lumpsfound[1]) // MAP01
            return OStringToUpper(OString(FREEDOOM2_PREFIX));
        else
            return "UNKNOWN";
    }

    void dump() const
    {
        for (IdentifierTable::const_iterator it = mIdentifiers.begin(); it != mIdentifiers.end(); ++it)
        {
            Printf(PRINT_HIGH, "%s\n", it->mFilename.c_str());
        }
    }

  private:
    typedef uint32_t IdType;

    typedef SArray<fileIdentifier_t> IdentifierTable;
    IdentifierTable                  mIdentifiers;

    typedef OHashTable<OCRC32Sum, IdType> CRC32SumLookupTable;
    CRC32SumLookupTable                   mCRC32SumLookup;

    typedef OHashTable<OMD5Hash, IdType> Md5SumLookupTable;
    Md5SumLookupTable                    mMd5SumLookup;

    typedef std::vector<OString> FilenameArray;
    FilenameArray                mIWADSearchOrder;
};

static FileIdentificationManager identtab;

//
// W_ConfigureGameInfo
//
// Queries the FileIdentityManager to identify the given IWAD file based
// on its MD5Sum. The appropriate values are then set for the globals
// gameinfo, gamemode, and gamemission.
//
// gamemode will be set to undetermined if the file is not a valid IWAD.
//
//
void W_ConfigureGameInfo(const OResFile &iwad)
{
    extern gameinfo_t SharewareGameInfo;
    extern gameinfo_t RegisteredGameInfo;
    extern gameinfo_t RetailGameInfo;
    extern gameinfo_t CommercialGameInfo;
    extern gameinfo_t RetailBFGGameInfo;
    extern gameinfo_t CommercialBFGGameInfo;

    const OString idname = identtab.identify(iwad);

    if (idname.find(OStringToUpper(OString(FREEDOOM1_PREFIX))) == 0)
    {
        gamemode    = retail;
        gameinfo    = RetailGameInfo;
        gamemission = retail_freedoom;
    }
    else if (idname.find(OStringToUpper(OString(FREEDOOM2_PREFIX))) == 0)
    {
        gamemode    = commercial;
        gameinfo    = CommercialGameInfo;
        gamemission = commercial_freedoom;
    }
    else if (idname.find(OStringToUpper(OString(FREEDM_PREFIX))) == 0)
    {
        gamemode    = commercial;
        gameinfo    = CommercialGameInfo;
        gamemission = commercial_freedoom;
    }
    else
    {
        gamemode    = undetermined;
        gameinfo    = SharewareGameInfo;
        gamemission = doom;
    }
}

//
// W_IsKnownIWAD
//
// Returns true if the given file is a known IWAD file.
//
bool W_IsKnownIWAD(const OWantFile &file)
{
    if (::identtab.isKnownIWADFilename(file.getBasename()))
        return true;

    return false;
}

//
// W_IsIWAD
//
// Returns true if the given file is an IWAD file.
//
// Right now we only want to test with a singular IWAD
// while we migrate from the WAD format altogether. 
// Assume anything ending in .wad is the IWAD - Dasho
bool W_IsIWAD(const OResFile &file)
{
    std::string ext;
    if (!M_ExtractFileExtension(file.getBasename(), ext))
        return false;
    if (ext != "wad")
        return false;
    return true;
}

std::vector<OString> W_GetIWADFilenames()
{
    return identtab.getFilenames();
}

VERSION_CONTROL(w_ident_cpp, "$Id: 815671388f73da7998e41dacaf22ab40089c2a94 $")
