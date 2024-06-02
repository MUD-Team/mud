

#pragma once

#include "r_defs.h"

// Called by startup code.
void R_Init();
void R_InitSprites();

// Retrieve column data for span blitting.
tallpost_t *R_GetPatchColumn(patch_t *patch, int32_t colnum);
tallpost_t *R_GetTextureColumn(texhandle_t texnum, int32_t colnum);

void R_PrecacheLevel(void);

extern std::vector<int32_t> originalLightLevels;

struct LocalView
{
    angle_t angle;
    bool    setangle;
    bool    skipangle;
    int32_t pitch;
    bool    setpitch;
    bool    skippitch;
};

//
// POV data.
//
extern fixed_t viewx;
extern fixed_t viewy;
extern fixed_t viewz;

extern angle_t   viewangle;
extern LocalView localview;
extern AActor   *camera; // [RH] camera instead of viewplayer

extern angle_t clipangle;

// extern fixed_t		finetangent[FINEANGLES/2];

extern visplane_t *floorplane;
extern visplane_t *ceilingplane;
extern visplane_t *skyplane;

// [AM] 4:3 Field of View
extern int32_t FieldOfView;
// [AM] Corrected (for widescreen) Field of View
extern int32_t CorrectFieldOfView;