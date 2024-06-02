// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: fdc10b510d4603a8dcc28f6babf338eaf8b9289e $
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
//	Data objects (?)
//
//-----------------------------------------------------------------------------

#include "dobject.h"

#include "d_player.h" // See p_user.cpp to find out why this doesn't work.
#include "m_alloc.h"  // Ideally, DObjects can be used independant of Doom.
#include "mud_includes.h"
#include "z_zone.h"

ClassInit::ClassInit(TypeInfo *type)
{
    type->RegisterType();
}

TypeInfo **TypeInfo::m_Types;
uint16_t   TypeInfo::m_NumTypes;
uint16_t   TypeInfo::m_MaxTypes;

void TypeInfo::RegisterType()
{
    if (m_NumTypes == m_MaxTypes)
    {
        m_MaxTypes = m_MaxTypes ? m_MaxTypes * 2 : 32;
        m_Types    = (TypeInfo **)Realloc(m_Types, m_MaxTypes * sizeof(*m_Types));
    }
    m_Types[m_NumTypes] = this;
    TypeIndex           = m_NumTypes;
    m_NumTypes++;
}

const TypeInfo *TypeInfo::FindType(const char *name)
{
    uint16_t i;

    for (i = 0; i != m_NumTypes; i++)
        if (!strcmp(name, m_Types[i]->Name))
            return m_Types[i];

    return NULL;
}

TypeInfo DObject::_StaticType("DObject", NULL, sizeof(DObject));

TArray<DObject *> DObject::Objects;
TArray<size_t>    DObject::FreeIndices;
TArray<DObject *> DObject::ToDestroy;
bool              DObject::Inactive;

DObject::DObject()
{
    ObjectFlags = 0;
    if (FreeIndices.Pop(Index))
        Objects[Index] = this;
    else
        Index = Objects.Push(this);
}

DObject::~DObject()
{
    if (!Inactive)
    {
        if (!(ObjectFlags & OF_MassDestruction))
        {
            RemoveFromArray();
        }
        else if (!(ObjectFlags & OF_Cleanup))
        {
            // object is queued for deletion, but is not being deleted
            // by the destruction process, so remove it from the
            // ToDestroy array and do other necessary stuff.
            int32_t i;

            for (i = ToDestroy.Size() - 1; i >= 0; i--)
            {
                if (ToDestroy[i] == this)
                {
                    ToDestroy[i] = NULL;
                    break;
                }
            }
        }
    }
}

void DObject::Destroy()
{
    if (!Inactive)
    {
        if (!(ObjectFlags & OF_MassDestruction))
        {
            RemoveFromArray();
            ObjectFlags |= OF_MassDestruction;
            ToDestroy.Push(this);
        }
    }
    else
        delete this;
}

void DObject::BeginFrame()
{
}

void DObject::EndFrame()
{
    DObject *obj;

    if (ToDestroy.Size())
    {
        // Printf (PRINT_HIGH, "Destroyed %d objects\n", ToDestroy.Size());

        while (ToDestroy.Pop(obj))
        {
            if (obj)
            {
                obj->ObjectFlags |= OF_Cleanup;
                delete obj;
            }
        }
    }
}

void DObject::RemoveFromArray()
{
    // denis - our array is static, so are some of the objects (eg DArgs)
    // so there's really no telling which is destroyed first, better to bail
    if (Inactive)
        return;

    if (Objects.Size() == Index + 1)
    {
        DObject *dummy;
        Objects.Pop(dummy);
    }
    else if (Objects.Size() > Index + 1)
    {
        Objects[Index] = NULL;
        FreeIndices.Push(Index);
    }
}

void DObject::StaticShutdown()
{
    Inactive = true;

    // denis - thinkers should be destroyed, but possibly not here?
    DThinker::DestroyAllThinkers();
}

VERSION_CONTROL(dobject_cpp, "$Id: fdc10b510d4603a8dcc28f6babf338eaf8b9289e $")
