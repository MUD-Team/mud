// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 06bde90b3f7fc7f2bf3f8f4d154b989331b80ac1 $
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
//  Sound Sequences header
//
//-----------------------------------------------------------------------------

#pragma once

#include "actor.h"
#include "r_defs.h"
#include "s_sound.h"

typedef enum
{
    SEQ_PLATFORM,
    SEQ_DOOR,
    SEQ_ENVIRONMENT,
    SEQ_NUMSEQTYPES,
    SEQ_NOTRANS,
    MAXSEQUENCES
} seqtype_t;

struct sector_s;

void      S_ParseSndSeq(void);
void      SN_StartSequence(AActor *mobj, int32_t sequence, seqtype_t type);
void      SN_StartSequence(AActor *mobj, const char *name);
void      SN_StartSequence(struct sector_s *sector, int32_t sequence, seqtype_t type);
void      SN_StartSequence(struct sector_s *sector, const char *name);
void      SN_StartSequence(fixed_t spot[3], int32_t sequence, seqtype_t type);
void      SN_StartSequence(fixed_t spot[3], const char *name);
void      SN_StopSequence(AActor *mobj);
void      SN_StopSequence(sector_t *sector);
void      SN_StopSequence(fixed_t spot[3]);
void      SN_UpdateActiveSequences(void);
void      SN_StopAllSequences(void);
ptrdiff_t SN_GetSequenceOffset(int32_t sequence, uint32_t *sequencePtr);
void SN_ChangeNodeData(int32_t nodeNum, int32_t seqOffset, int32_t delayTics, float volume, int32_t currentSoundID);

class DSeqNode : public DObject
{
    DECLARE_SERIAL(DSeqNode, DObject)
  public:
    virtual ~DSeqNode();
    virtual void MakeSound()
    {
    }
    virtual void MakeLoopedSound()
    {
    }
    virtual void *Source()
    {
        return NULL;
    }
    virtual bool IsPlaying()
    {
        return false;
    }
    void                    RunThink();
    inline static DSeqNode *FirstSequence()
    {
        return SequenceListHead;
    }
    inline DSeqNode *NextSequence() const
    {
        return m_Next;
    }
    void ChangeData(int32_t seqOffset, int32_t delayTics, float volume, int32_t currentSoundID);

    static void SerializeSequences(FArchive &arc);

  protected:
    DSeqNode();
    DSeqNode(int32_t sequence);

    uint32_t *m_SequencePtr;
    int32_t   m_Sequence;

    int32_t m_CurrentSoundID;
    int32_t m_DelayTics;
    float   m_Volume;
    int32_t m_StopSound;
    int32_t m_Atten;

  private:
    static DSeqNode *SequenceListHead;
    DSeqNode        *m_Next, *m_Prev;

    void ActivateSequence(int32_t sequence);

    friend void SN_StopAllSequences(void);
};

typedef struct
{
    char     name[MAX_SNDNAME + 1];
    int32_t  stopsound;
    uint32_t script[1]; // + more until end of sequence script
} sndseq_t;

void      SN_StartSequence(AActor *mobj, int32_t sequence, seqtype_t type);
void      SN_StartSequence(AActor *mobj, const char *name);
void      SN_StartSequence(struct sector_s *sector, int32_t sequence, seqtype_t type);
void      SN_StartSequence(struct sector_s *sector, const char *name);
void      SN_StartSequence(polyobj_t *poly, int32_t sequence, seqtype_t type);
void      SN_StartSequence(polyobj_t *poly, const char *name);
void      SN_StopSequence(AActor *mobj);
void      SN_StopSequence(sector_t *sector);
void      SN_StopSequence(polyobj_t *poly);
void      SN_UpdateActiveSequences(void);
ptrdiff_t SN_GetSequenceOffset(int32_t sequence, uint32_t *sequencePtr);
void      SN_DoStop(void *);
void SN_ChangeNodeData(int32_t nodeNum, int32_t seqOffset, int32_t delayTics, float volume, int32_t currentSoundID);

extern sndseq_t **Sequences;
extern int32_t    ActiveSequences;
extern int32_t    NumSequences;
