
#pragma once

#include "map_defs.h"

// A vissprite_t is a thing
//	that will be drawn during a refresh.
// I.e. a sprite object that is partly visible.
struct vissprite_s
{
    int32_t x1;
    int32_t x2;
    int32_t y1;
    int32_t y2;

    // for line side calculation
    fixed_t gx;
    fixed_t gy;

    // global bottom / top for silhouette clipping
    fixed_t gzb;
    fixed_t gzt;

    // horizontal position of x1
    fixed_t startfrac;

    fixed_t xscale, yscale;

    // negative if flipped
    fixed_t xiscale;

    fixed_t     depth;
    fixed_t     texturemid;
    texhandle_t tex_id;
    patch_t    *tex_patch = NULL;

    // for color translation and shadow draw,
    //  maxbright frames as well
    shaderef_t colormap;

    int32_t mobjflags;
    bool    spectator;   // [Blair] Mark if this visprite belongs to a spectator.

    sector_t *heightsec; // killough 3/27/98: height sector for underwater/fake ceiling
    fixed_t   translucency;
    uint8_t   FakeFlat;  // [RH] which side of fake/floor ceiling sprite is on

    AActor *mo;
};
typedef vissprite_s vissprite_t;

//
// The infamous visplane
//
struct visplane_s
{
    visplane_s *next; // Next visplane in hash chain -- killough

    plane_t secplane;

    texhandle_t picnum;
    int32_t     lightlevel;
    fixed_t     xoffs, yoffs; // killough 2/28/98: Support scrolling flats
    int32_t     minx;
    int32_t     maxx;

    shaderef_t colormap;       // [RH] Support multiple colormaps
    fixed_t    xscale, yscale; // [RH] Support flat scaling
    angle_t    angle;          // [RH] Support flat rotation

    uint32_t *bottom;          // [RH] bottom and top arrays are dynamically
    uint32_t  pad;             //		allocated immediately after the
    uint32_t  top[3];          //		visplane.
};
typedef visplane_s visplane_t;

extern int32_t MaxVisSprites;
extern vissprite_t *vissprites;
extern vissprite_t *lastvissprite;

