

#pragma once

#include "r_defs.h"

// Retrieve column data for span blitting.
tallpost_t *R_GetPatchColumn(patch_t *patch, int32_t colnum);
tallpost_t *R_GetTextureColumn(texhandle_t texnum, int32_t colnum);

void R_PrecacheLevel(void);
