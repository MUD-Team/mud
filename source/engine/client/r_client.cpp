

#include "r_client.h"

#include "Poco/ByteOrder.h"
#include "r_sky.h"
#include "r_sprites.h"

//
// R_GetPatchColumn
//
tallpost_t *R_GetPatchColumn(patch_t *patch, int32_t colnum)
{
    return (tallpost_t *)((uint8_t *)patch + Poco::ByteOrder::fromLittleEndian(patch->columnofs[colnum]));
}

//
// R_GetTextureColumn
//
tallpost_t *R_GetTextureColumn(texhandle_t texnum, int32_t colnum)
{
    const Texture *tex = texturemanager.getTexture(texnum);
    colnum &= tex->getWidthMask();
    patch_t *texpatch = (patch_t *)tex->getData();

    return (tallpost_t *)((uint8_t *)texpatch + Poco::ByteOrder::fromLittleEndian(texpatch->columnofs[colnum]));
}

//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//
// [RH] Rewrote this using Lee Killough's code in BOOM as an example.

void R_PrecacheLevel(void)
{
    int32_t i;

    // Precache flats.
    for (i = numsectors - 1; i >= 0; i--)
    {
        texturemanager.getTexture(sectors[i].floorpic);
        texturemanager.getTexture(sectors[i].ceilingpic);
    }

    // Precache textures.
    for (i = numsides - 1; i >= 0; i--)
    {
        texturemanager.getTexture(sides[i].toptexture);
        texturemanager.getTexture(sides[i].midtexture);
        texturemanager.getTexture(sides[i].bottomtexture);
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //	indicate a sky floor/ceiling as a flat,
    //	while the sky texture is stored like
    //	a wall texture, with an episode dependend
    //	name.
    //
    // [RH] Possibly two sky textures now.
    // [ML] 5/11/06 - Not anymore!

    texturemanager.getTexture(sky1texture);
    texturemanager.getTexture(sky2texture);

    // Precache sprites
    AActor                  *actor;
    TThinkerIterator<AActor> iterator;

    while ((actor = iterator.Next()))
        R_CacheSprite(sprites + actor->sprite);
}

//
// GAME FUNCTIONS
//
int32_t      MaxVisSprites;
vissprite_t *vissprites;
vissprite_t *lastvissprite;

//
// R_InitSprites
// Called at program start.
//
void R_InitSprites()
{
    MaxVisSprites = 128; // [RH] This is the initial default value. It grows as needed.

    M_Free(vissprites);

    vissprites    = (vissprite_t *)Malloc(MaxVisSprites * sizeof(vissprite_t));
    lastvissprite = &vissprites[MaxVisSprites];
}
