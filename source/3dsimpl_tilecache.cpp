
#include "3dsimpl.h"
#include "3dsimpl_tilecache.h"


//---------------------------------------------------------
// Initializes the Hash to Texture Position look-up (and
// the reverse look-up table as well)
//---------------------------------------------------------
void cache3dsInit()
{
    //printf ("Cache %8x\n", &vramCacheFrameNumber);
    //memset(&vramCacheFrameNumber, 0, MAX_HASH * 4);
    memset(&GPU3DSExt.vramCacheHashToTexturePosition, 0, (MAX_HASH + 1) * 2);

    // Fixes issue in Thunder Spirits where the tileaddr + pal
    // gives the texturePos of 0, but gets overwritten by other tiles
    //
    GPU3DSExt.vramCacheTexturePositionToHash[0] = 0;
    for (int i = 1; i < MAX_TEXTURE_POSITIONS; i++)
        GPU3DSExt.vramCacheTexturePositionToHash[i] = MAX_HASH;
    GPU3DSExt.newCacheTexturePosition = 2;
}


//---------------------------------------------------------
// Converts the tile in SNES bitplane format into 
// it's 5551 16-bit representation in 3DS texture format.
//---------------------------------------------------------
void cache3dsCacheSnesTileToTexturePosition(
    uint8 *snesTilePixels,
	uint16 *snesPalette,
    uint16 texturePosition)
{
    int tx = texturePosition % 128;
    int ty = (texturePosition / 128) & 0x7f;
    texturePosition = (127 - ty) * 128 + tx;    // flip vertically.
    uint32 base = texturePosition * 64;

    uint16 *tileTexture = (uint16 *)snesTileCacheTexture->PixelData;

    #define GET_TILE_PIXEL(x)   (snesTilePixels[x] == 0 ? 0 : snesPalette[snesTilePixels[x]])
    tileTexture [base + 0] = GET_TILE_PIXEL(56);
    tileTexture [base + 1] = GET_TILE_PIXEL(57);
    tileTexture [base + 4] = GET_TILE_PIXEL(58);
    tileTexture [base + 5] = GET_TILE_PIXEL(59);
    tileTexture [base + 16] = GET_TILE_PIXEL(60);
    tileTexture [base + 17] = GET_TILE_PIXEL(61);
    tileTexture [base + 20] = GET_TILE_PIXEL(62);
    tileTexture [base + 21] = GET_TILE_PIXEL(63);

    tileTexture [base + 2] = GET_TILE_PIXEL(48);
    tileTexture [base + 3] = GET_TILE_PIXEL(49);
    tileTexture [base + 6] = GET_TILE_PIXEL(50);
    tileTexture [base + 7] = GET_TILE_PIXEL(51);
    tileTexture [base + 18] = GET_TILE_PIXEL(52);
    tileTexture [base + 19] = GET_TILE_PIXEL(53);
    tileTexture [base + 22] = GET_TILE_PIXEL(54);
    tileTexture [base + 23] = GET_TILE_PIXEL(55);

    tileTexture [base + 8] = GET_TILE_PIXEL(40);
    tileTexture [base + 9] = GET_TILE_PIXEL(41);
    tileTexture [base + 12] = GET_TILE_PIXEL(42);
    tileTexture [base + 13] = GET_TILE_PIXEL(43);
    tileTexture [base + 24] = GET_TILE_PIXEL(44);
    tileTexture [base + 25] = GET_TILE_PIXEL(45);
    tileTexture [base + 28] = GET_TILE_PIXEL(46);
    tileTexture [base + 29] = GET_TILE_PIXEL(47);

    tileTexture [base + 10] = GET_TILE_PIXEL(32);
    tileTexture [base + 11] = GET_TILE_PIXEL(33);
    tileTexture [base + 14] = GET_TILE_PIXEL(34);
    tileTexture [base + 15] = GET_TILE_PIXEL(35);
    tileTexture [base + 26] = GET_TILE_PIXEL(36);
    tileTexture [base + 27] = GET_TILE_PIXEL(37);
    tileTexture [base + 30] = GET_TILE_PIXEL(38);
    tileTexture [base + 31] = GET_TILE_PIXEL(39);

    tileTexture [base + 32] = GET_TILE_PIXEL(24);
    tileTexture [base + 33] = GET_TILE_PIXEL(25);
    tileTexture [base + 36] = GET_TILE_PIXEL(26);
    tileTexture [base + 37] = GET_TILE_PIXEL(27);
    tileTexture [base + 48] = GET_TILE_PIXEL(28);
    tileTexture [base + 49] = GET_TILE_PIXEL(29);
    tileTexture [base + 52] = GET_TILE_PIXEL(30);
    tileTexture [base + 53] = GET_TILE_PIXEL(31);

    tileTexture [base + 34] = GET_TILE_PIXEL(16);
    tileTexture [base + 35] = GET_TILE_PIXEL(17);
    tileTexture [base + 38] = GET_TILE_PIXEL(18);
    tileTexture [base + 39] = GET_TILE_PIXEL(19);
    tileTexture [base + 50] = GET_TILE_PIXEL(20);
    tileTexture [base + 51] = GET_TILE_PIXEL(21);
    tileTexture [base + 54] = GET_TILE_PIXEL(22);
    tileTexture [base + 55] = GET_TILE_PIXEL(23);

    tileTexture [base + 40] = GET_TILE_PIXEL(8);
    tileTexture [base + 41] = GET_TILE_PIXEL(9);
    tileTexture [base + 44] = GET_TILE_PIXEL(10);
    tileTexture [base + 45] = GET_TILE_PIXEL(11);
    tileTexture [base + 56] = GET_TILE_PIXEL(12);
    tileTexture [base + 57] = GET_TILE_PIXEL(13);
    tileTexture [base + 60] = GET_TILE_PIXEL(14);
    tileTexture [base + 61] = GET_TILE_PIXEL(15);

    tileTexture [base + 42] = GET_TILE_PIXEL(0);
    tileTexture [base + 43] = GET_TILE_PIXEL(1);
    tileTexture [base + 46] = GET_TILE_PIXEL(2);
    tileTexture [base + 47] = GET_TILE_PIXEL(3);
    tileTexture [base + 58] = GET_TILE_PIXEL(4);
    tileTexture [base + 59] = GET_TILE_PIXEL(5);
    tileTexture [base + 62] = GET_TILE_PIXEL(6);
    tileTexture [base + 63] = GET_TILE_PIXEL(7);

}


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
    uint32 *paletteMask)
{
    int tx = texturePosition % 16;              // should never be >= 16
    int ty = (texturePosition / 16) & 0xf;      // should never be >= 16
    texturePosition = (15 - ty) * 16 + tx;      // flip vertically.
    uint32 base = texturePosition * 64;

    uint16 *tileTexture = (uint16 *)snesMode7TileCacheTexture->PixelData;
	uint32 charPaletteMask = 0;

    #define GET_TILE_PIXEL(x)   (snesTilePixels[x * 2] == 0 ? 0 : snesPalette[snesTilePixels[x * 2]]); charPaletteMask |= (1 << (snesTilePixels[x * 2] >> 3));
    tileTexture [base + 0] = GET_TILE_PIXEL(56);
    tileTexture [base + 1] = GET_TILE_PIXEL(57);
    tileTexture [base + 4] = GET_TILE_PIXEL(58);
    tileTexture [base + 5] = GET_TILE_PIXEL(59);
    tileTexture [base + 16] = GET_TILE_PIXEL(60);
    tileTexture [base + 17] = GET_TILE_PIXEL(61);
    tileTexture [base + 20] = GET_TILE_PIXEL(62);
    tileTexture [base + 21] = GET_TILE_PIXEL(63);

    tileTexture [base + 2] = GET_TILE_PIXEL(48);
    tileTexture [base + 3] = GET_TILE_PIXEL(49);
    tileTexture [base + 6] = GET_TILE_PIXEL(50);
    tileTexture [base + 7] = GET_TILE_PIXEL(51);
    tileTexture [base + 18] = GET_TILE_PIXEL(52);
    tileTexture [base + 19] = GET_TILE_PIXEL(53);
    tileTexture [base + 22] = GET_TILE_PIXEL(54);
    tileTexture [base + 23] = GET_TILE_PIXEL(55);

    tileTexture [base + 8] = GET_TILE_PIXEL(40);
    tileTexture [base + 9] = GET_TILE_PIXEL(41);
    tileTexture [base + 12] = GET_TILE_PIXEL(42);
    tileTexture [base + 13] = GET_TILE_PIXEL(43);
    tileTexture [base + 24] = GET_TILE_PIXEL(44);
    tileTexture [base + 25] = GET_TILE_PIXEL(45);
    tileTexture [base + 28] = GET_TILE_PIXEL(46);
    tileTexture [base + 29] = GET_TILE_PIXEL(47);

    tileTexture [base + 10] = GET_TILE_PIXEL(32);
    tileTexture [base + 11] = GET_TILE_PIXEL(33);
    tileTexture [base + 14] = GET_TILE_PIXEL(34);
    tileTexture [base + 15] = GET_TILE_PIXEL(35);
    tileTexture [base + 26] = GET_TILE_PIXEL(36);
    tileTexture [base + 27] = GET_TILE_PIXEL(37);
    tileTexture [base + 30] = GET_TILE_PIXEL(38);
    tileTexture [base + 31] = GET_TILE_PIXEL(39);

    tileTexture [base + 32] = GET_TILE_PIXEL(24);
    tileTexture [base + 33] = GET_TILE_PIXEL(25);
    tileTexture [base + 36] = GET_TILE_PIXEL(26);
    tileTexture [base + 37] = GET_TILE_PIXEL(27);
    tileTexture [base + 48] = GET_TILE_PIXEL(28);
    tileTexture [base + 49] = GET_TILE_PIXEL(29);
    tileTexture [base + 52] = GET_TILE_PIXEL(30);
    tileTexture [base + 53] = GET_TILE_PIXEL(31);

    tileTexture [base + 34] = GET_TILE_PIXEL(16);
    tileTexture [base + 35] = GET_TILE_PIXEL(17);
    tileTexture [base + 38] = GET_TILE_PIXEL(18);
    tileTexture [base + 39] = GET_TILE_PIXEL(19);
    tileTexture [base + 50] = GET_TILE_PIXEL(20);
    tileTexture [base + 51] = GET_TILE_PIXEL(21);
    tileTexture [base + 54] = GET_TILE_PIXEL(22);
    tileTexture [base + 55] = GET_TILE_PIXEL(23);

    tileTexture [base + 40] = GET_TILE_PIXEL(8);
    tileTexture [base + 41] = GET_TILE_PIXEL(9);
    tileTexture [base + 44] = GET_TILE_PIXEL(10);
    tileTexture [base + 45] = GET_TILE_PIXEL(11);
    tileTexture [base + 56] = GET_TILE_PIXEL(12);
    tileTexture [base + 57] = GET_TILE_PIXEL(13);
    tileTexture [base + 60] = GET_TILE_PIXEL(14);
    tileTexture [base + 61] = GET_TILE_PIXEL(15);

    tileTexture [base + 42] = GET_TILE_PIXEL(0);
    tileTexture [base + 43] = GET_TILE_PIXEL(1);
    tileTexture [base + 46] = GET_TILE_PIXEL(2);
    tileTexture [base + 47] = GET_TILE_PIXEL(3);
    tileTexture [base + 58] = GET_TILE_PIXEL(4);
    tileTexture [base + 59] = GET_TILE_PIXEL(5);
    tileTexture [base + 62] = GET_TILE_PIXEL(6);
    tileTexture [base + 63] = GET_TILE_PIXEL(7);

    *paletteMask = charPaletteMask;
}

