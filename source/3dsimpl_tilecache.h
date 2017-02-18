#include "3dssnes9x.h"
#include "3dsgpu.h"
#include "3dsimpl_gpu.h"

#ifndef _3DSIMPL_TILECACHE_H_
#define _3DSIMPL_TILECACHE_H_


//---------------------------------------------------------
// Initializes the Hash to Texture Position look-up (and
// the reverse look-up table as well)
//---------------------------------------------------------
void cache3dsInit();


//---------------------------------------------------------
// Computes and returns the texture position (Non-Mode 7) 
// given the tile number and 
//---------------------------------------------------------
inline int cache3dsGetTexturePositionFast(int tileAddr, int pal)
{
    tileAddr = tileAddr / 8;
    int hash = COMPOSE_HASH(tileAddr, pal);
    int pos = GPU3DSExt.vramCacheHashToTexturePosition[hash];

    if (pos == 0)
    {
        pos = GPU3DSExt.newCacheTexturePosition;

        //vramCacheFrameNumber[hash] = 0;

        GPU3DSExt.vramCacheTexturePositionToHash[GPU3DSExt.vramCacheHashToTexturePosition[hash] & 0xFFFE] = 0;

        GPU3DSExt.vramCacheHashToTexturePosition[GPU3DSExt.vramCacheTexturePositionToHash[pos]] = 0;

        GPU3DSExt.vramCacheHashToTexturePosition[hash] = pos;
        GPU3DSExt.vramCacheTexturePositionToHash[pos] = hash;

        GPU3DSExt.newCacheTexturePosition += 2;
        if (GPU3DSExt.newCacheTexturePosition >= MAX_TEXTURE_POSITIONS)
            GPU3DSExt.newCacheTexturePosition = 2;

        // Force this tile to re-decode. This fixes the tile corruption
        // problems when playing a game for too long.
        //
        GFX.VRAMPaletteFrame[tileAddr][pal] = 0;
    }

    return pos;
}


//---------------------------------------------------------
// Swaps the texture position for alternate frames. This
// is required to fix sprite flickering problems in games
// that updates the tile bitmaps mid-frame (for eg. in 
// DKC2)
//---------------------------------------------------------
inline int cacheGetSwapTexturePositionForAltFrameFast(int tileAddr, int pal)
{
    tileAddr = tileAddr / 8;
    int hash = COMPOSE_HASH(tileAddr, pal);
    int pos = GPU3DSExt.vramCacheHashToTexturePosition[hash] ^ 1;
    GPU3DSExt.vramCacheHashToTexturePosition[hash] = pos;
    return pos;
}


//---------------------------------------------------------
// Converts the tile in SNES bitplane format into 
// it's 5551 16-bit representation in 3DS texture format.
//---------------------------------------------------------
void cache3dsCacheSnesTileToTexturePosition(
    uint8 *snesTilePixels,
	uint16 *snesPalette,
    uint16 texturePosition);


//---------------------------------------------------------
// Converts the tile in SNES bitplane format into 
// it's 5551 16-bit representation in 3DS texture format.
//
// This is meant for the mode 7 tiles.
//---------------------------------------------------------
void cache3dsCacheSnesTileToMode7TexturePosition(
    uint8 *snesTilePixels,
	uint16 *snesPalette,
    uint16 texturePosition,
    uint32 *paletteMask);



#endif