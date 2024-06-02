// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: a823b6d6df572205876051ba3727ed6bdb770269 $
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
//	Rendering of moving objects, sprites.
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_defs.h"
#include "r_sprites.h"

// [RH] Particle details
struct particle_s
{
    fixed_t  x, y, z;
    fixed_t  velx, vely, velz;
    fixed_t  accx, accy, accz;
    uint8_t  ttl;
    uint8_t  trans;
    uint8_t  size;
    uint8_t  fade;
    int32_t  color;
    uint16_t next;
    uint16_t nextinsubsector;
};
typedef struct particle_s particle_t;

extern int32_t          NumParticles;
extern int32_t          ActiveParticles;
extern int32_t          InactiveParticles;
extern particle_t      *Particles;
extern TArray<uint16_t> ParticlesInSubsec;

const uint16_t NO_PARTICLE = 0xffff;

#ifdef _MSC_VER
__inline particle_t *NewParticle()
{
    particle_t *result = NULL;
    if (InactiveParticles != NO_PARTICLE)
    {
        result            = Particles + InactiveParticles;
        InactiveParticles = result->next;
        result->next      = ActiveParticles;
        ActiveParticles   = result - Particles;
    }
    return result;
}
#else
particle_t *NewParticle();
#endif
void R_InitParticles();
void R_ClearParticles();
void R_DrawParticle(vissprite_t *);
void R_ProjectParticle(particle_t *, const sector_t *sector, int32_t fakeside);
void R_FindParticleSubsectors();

extern int32_t MaxVisSprites;

extern vissprite_t *vissprites;
extern vissprite_t *vissprite_p;
extern vissprite_t  vsprsortedhead;

// vars for R_DrawMaskedColumn
extern int32_t *mfloorclip;
extern int32_t *mceilingclip;
extern fixed_t  spryscale;
extern fixed_t  sprtopscreen;

void R_SortVisSprites();
void R_AddSprites(sector_t *sec, int32_t lightlevel, int32_t fakeside);
void R_ClearSprites();
void R_DrawMasked();
