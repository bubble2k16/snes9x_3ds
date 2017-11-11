
#include "snes9x.h"
#include "ppu.h"
#include "3dssettings.h"

#include <3ds.h>
#include "3dsgpu.h"
#include "3dsimpl.h"
#include "3dsimpl_gpu.h"

SGPU3DSExtended GPU3DSExt;
extern S9xSettings3DS settings3DS;

void gpu3dsSetMode7UpdateFrameCountUniform();

void gpu3dsInitializeMode7Vertex(int idx, int x, int y)
{
    int x0 = 0;
    int y0 = 0;

    if (x < 64)
    {
        x0 = x * 8;
        y0 = (y * 2 + 1) * 8;
    }
    else
    {
        x0 = (x - 64) * 8;
        y0 = (y * 2) * 8;
    }

    int x1 = x0 + 8;
    int y1 = y0 + 8;

    if (GPU3DS.isReal3DS)
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, -1};
        //m7vertices[1].Position = (SVector4i){x1, y1, 0, -1};

        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        //m7vertices[1].TexCoord = (STexCoord2i){8, 8};

    }
    else
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx * 6];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, -1};
        m7vertices[1].Position = (SVector4i){x1, y0, 0, -1};
        m7vertices[2].Position = (SVector4i){x0, y1, 0, -1};

        m7vertices[3].Position = (SVector4i){x1, y1, 0, -1};
        m7vertices[4].Position = (SVector4i){x0, y1, 0, -1};
        m7vertices[5].Position = (SVector4i){x1, y0, 0, -1};

        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        m7vertices[1].TexCoord = (STexCoord2i){8, 0};
        m7vertices[2].TexCoord = (STexCoord2i){0, 8};

        m7vertices[3].TexCoord = (STexCoord2i){8, 8};
        m7vertices[4].TexCoord = (STexCoord2i){0, 8};
        m7vertices[5].TexCoord = (STexCoord2i){8, 0};

    }
}

void gpu3dsInitializeMode7VertexForTile0(int idx, int x, int y)
{
    int x0 = x;
    int y0 = y;

    int x1 = x0 + 8;
    int y1 = y0 + 8;

    if (GPU3DS.isReal3DS)
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, 0x3fff};
        //m7vertices[1].Position = (SVector4i){x1, y1, 0, 0};

        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        //m7vertices[1].TexCoord = (STexCoord2i){8, 8};

    }
    else
    {
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DSExt.mode7TileVertexes.List) [idx * 6];

        m7vertices[0].Position = (SVector4i){x0, y0, 0, 0x3fff};
        m7vertices[1].Position = (SVector4i){x1, y0, 0, 0x3fff};
        m7vertices[2].Position = (SVector4i){x0, y1, 0, 0x3fff};

        m7vertices[3].Position = (SVector4i){x1, y1, 0, 0x3fff};
        m7vertices[4].Position = (SVector4i){x0, y1, 0, 0x3fff};
        m7vertices[5].Position = (SVector4i){x1, y0, 0, 0x3fff};

        m7vertices[0].TexCoord = (STexCoord2i){0, 0};
        m7vertices[1].TexCoord = (STexCoord2i){8, 0};
        m7vertices[2].TexCoord = (STexCoord2i){0, 8};

        m7vertices[3].TexCoord = (STexCoord2i){8, 8};
        m7vertices[4].TexCoord = (STexCoord2i){0, 8};
        m7vertices[5].TexCoord = (STexCoord2i){8, 0};

    }
}

void gpu3dsInitializeMode7Vertexes()
{
    GPU3DSExt.mode7FrameCount = 3;
    gpu3dsSetMode7UpdateFrameCountUniform();
    for (int f = 0; f < 2; f++)
    {
        int idx = 0;
        for (int section = 0; section < 4; section++)
        {
            for (int y = 0; y < 32; y++)
                for (int x = 0; x < 128; x++)
                    gpu3dsInitializeMode7Vertex(idx++, x, y);
        }

        gpu3dsInitializeMode7VertexForTile0(16384, 0, 0);
        gpu3dsInitializeMode7VertexForTile0(16385, 0, 8);
        gpu3dsInitializeMode7VertexForTile0(16386, 8, 0);
        gpu3dsInitializeMode7VertexForTile0(16387, 8, 8);

        gpu3dsSwapVertexListForNextFrame(&GPU3DSExt.mode7TileVertexes);
    }
}


void gpu3dsDrawRectangle(int x0, int y0, int x1, int y1, int depth, u32 color)
{
    gpu3dsAddRectangleVertexes (x0, y0, x1, y1, depth, color);
    gpu3dsDrawVertexList(&GPU3DSExt.rectangleVertexes, GPU_GEOMETRY_PRIM, false, -1, -1);
}


void gpu3dsAddRectangleVertexes(int x0, int y0, int x1, int y1, int depth, u32 color)
{
    if (GPU3DS.isReal3DS)
    {
        SVertexColor *vertices = &((SVertexColor *) GPU3DSExt.rectangleVertexes.List)[GPU3DSExt.rectangleVertexes.Count];

        vertices[0].Position = (SVector4i){x0, y0, depth, 1};
        vertices[1].Position = (SVector4i){x1, y1, depth, 1};

        u32 swappedColor = ((color & 0xff) << 24) | ((color & 0xff00) << 8) | ((color & 0xff0000) >> 8) | ((color & 0xff000000) >> 24);
        vertices[0].Color = swappedColor;
        vertices[1].Color = swappedColor;

        GPU3DSExt.rectangleVertexes.Count += 2;
    }
    else
    {
        SVertexColor *vertices = &((SVertexColor *) GPU3DSExt.rectangleVertexes.List)[GPU3DSExt.rectangleVertexes.Count];

        vertices[0].Position = (SVector4i){x0, y0, depth, 1};
        vertices[1].Position = (SVector4i){x1, y0, depth, 1};
        vertices[2].Position = (SVector4i){x0, y1, depth, 1};
        vertices[3].Position = (SVector4i){x1, y1, depth, 1};
        vertices[4].Position = (SVector4i){x1, y0, depth, 1};
        vertices[5].Position = (SVector4i){x0, y1, depth, 1};

        u32 swappedColor = ((color & 0xff) << 24) | ((color & 0xff00) << 8) | ((color & 0xff0000) >> 8) | ((color & 0xff000000) >> 24);
        vertices[0].Color = swappedColor;
        vertices[1].Color = swappedColor;
        vertices[2].Color = swappedColor;
        vertices[3].Color = swappedColor;
        vertices[4].Color = swappedColor;
        vertices[5].Color = swappedColor;

        GPU3DSExt.rectangleVertexes.Count += 6;
    }
}


void gpu3dsDrawVertexes(bool repeatLastDraw, int storeIndex)
{
    gpu3dsDrawVertexList(&GPU3DSExt.quadVertexes, GPU_TRIANGLES, repeatLastDraw, 0, storeIndex);
    gpu3dsDrawVertexList(&GPU3DSExt.tileVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 1, storeIndex);
    gpu3dsDrawVertexList(&GPU3DSExt.rectangleVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 2, storeIndex);
}


void gpu3dsDrawMode7Vertexes(int fromIndex, int tileCount)
{
    if (GPU3DS.isReal3DS)
        gpu3dsDrawVertexList(&GPU3DSExt.mode7TileVertexes, GPU_GEOMETRY_PRIM, fromIndex, tileCount);
    else
        gpu3dsDrawVertexList(&GPU3DSExt.mode7TileVertexes, GPU_TRIANGLES, fromIndex, tileCount);

}

void gpu3dsDrawMode7LineVertexes(bool repeatLastDraw, int storeIndex)
{
    if (GPU3DS.isReal3DS)
        gpu3dsDrawVertexList(&GPU3DSExt.mode7LineVertexes, GPU_GEOMETRY_PRIM, repeatLastDraw, 3, storeIndex);
    else
        gpu3dsDrawVertexList(&GPU3DSExt.mode7LineVertexes, GPU_TRIANGLES, repeatLastDraw, 3, storeIndex);
}


// Changes the texture pixel format (but it must be the same 
// size as the original pixel format). No errors will be thrown
// if the format is incorrect.
//
void gpu3dsSetMode7TexturesPixelFormatToRGB5551()
{
	snesMode7FullTexture->PixelFormat = GPU_RGBA5551;
    snesMode7Tile0Texture->PixelFormat = GPU_RGBA5551;
    snesMode7TileCacheTexture->PixelFormat = GPU_RGBA5551;
}

void gpu3dsSetMode7TexturesPixelFormatToRGB4444()
{
	snesMode7FullTexture->PixelFormat = GPU_RGBA4;
    snesMode7Tile0Texture->PixelFormat = GPU_RGBA4;
    snesMode7TileCacheTexture->PixelFormat = GPU_RGBA4;
}

void gpu3dsSetRenderTargetToMainScreenTexture()
{
    gpu3dsSetRenderTargetToTexture(snesMainScreenTarget, snesDepthForScreens);
}

void gpu3dsSetRenderTargetToSubScreenTexture()
{
    gpu3dsSetRenderTargetToTexture(snesSubScreenTarget, snesDepthForScreens);
}

void gpu3dsSetRenderTargetToDepthTexture()
{
    gpu3dsSetRenderTargetToTexture(snesDepthForScreens, snesDepthForOtherTextures);
}

void gpu3dsSetRenderTargetToMode7FullTexture(int pixelOffset, int width, int height)
{
    gpu3dsSetRenderTargetToTextureSpecific(snesMode7FullTexture, snesDepthForOtherTextures,
        pixelOffset * gpu3dsGetPixelSize(snesMode7FullTexture->PixelFormat), width, height);
}

void gpu3dsSetRenderTargetToMode7Tile0Texture()
{
    gpu3dsSetRenderTargetToTexture(snesMode7Tile0Texture, snesDepthForOtherTextures);
}

void gpu3dsSetMode7UpdateFrameCountUniform()
{
    int updateFrame = GPU3DSExt.mode7FrameCount;
    GPU3DSExt.mode7UpdateFrameCount[0] = ((float)updateFrame) - 0.5f;      // set 'w' to updateFrame

    GPU_SetFloatUniform(GPU_VERTEX_SHADER, 5, (u32 *)GPU3DSExt.mode7UpdateFrameCount, 1);
    GPU_SetFloatUniform(GPU_GEOMETRY_SHADER, 15, (u32 *)GPU3DSExt.mode7UpdateFrameCount, 1);
}


void gpu3dsCopyVRAMTilesIntoMode7TileVertexes(uint8 *VRAM)
{
    for (int i = 0; i < 16384; i++)
    {
        gpu3dsSetMode7TileTexturePos(i, VRAM[i * 2]);
        gpu3dsSetMode7TileModifiedFlag(i);
    }
    IPPU.Mode7CharDirtyFlagCount = 1;
    for (int i = 0; i < 256; i++)
    {
        IPPU.Mode7CharDirtyFlag[i] = 2;
    }
}

void gpu3dsIncrementMode7UpdateFrameCount()
{
    gpu3dsSwapVertexListForNextFrame(&GPU3DSExt.mode7TileVertexes);
    GPU3DSExt.mode7FrameCount ++;

    if (GPU3DSExt.mode7FrameCount == 0x3fff)
    {
        GPU3DSExt.mode7FrameCount = 1;
    }

    // Bug fix: Clears the updateFrameCount of both sets
    // of mode7TileVertexes!
    //
    if (GPU3DSExt.mode7FrameCount <= 2)
    {
        for (int i = 0; i < 16384; )
        {
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);

            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
            gpu3dsSetMode7TileModifiedFlag(i++, -1);
        }
    }

    gpu3dsSetMode7UpdateFrameCountUniform();
}


void gpu3dsBindTextureDepthForScreens(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesDepthForScreens, unit);
}


void gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesMode7TileCacheTexture, unit);
}

void gpu3dsBindTextureSnesMode7Tile0CacheRepeat(GPU_TEXUNIT unit)
{
    gpu3dsBindTextureWithParams(snesMode7Tile0Texture, unit,
        GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_WRAP_S(GPU_REPEAT)
        | GPU_TEXTURE_WRAP_T(GPU_REPEAT));
}

void gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT unit)
{
    gpu3dsBindTextureWithParams(snesMode7FullTexture, unit,
        GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_BORDER)
        | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_BORDER));
}

void gpu3dsBindTextureSnesMode7FullRepeat(GPU_TEXUNIT unit)
{
    gpu3dsBindTextureWithParams(snesMode7FullTexture, unit,
        GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_WRAP_S(GPU_REPEAT)
        | GPU_TEXTURE_WRAP_T(GPU_REPEAT));
}


void gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesTileCacheTexture, unit);
}

void gpu3dsBindTextureSnesTileCacheForHires(GPU_TEXUNIT unit)
{
    gpu3dsBindTextureWithParams(snesTileCacheTexture, unit,
	    GPU_TEXTURE_MAG_FILTER(GPU_NEAREST)
        | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST)
		| GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_BORDER)
		| GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_BORDER)
    );
}

void gpu3dsBindTextureMainScreen(GPU_TEXUNIT unit)
{
    gpu3dsBindTextureWithParams(snesMainScreenTarget, unit,
        GPU_TEXTURE_MAG_FILTER(GPU_LINEAR)
        | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR)
        | GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_BORDER)
        | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_BORDER));
}

void gpu3dsBindTextureSubScreen(GPU_TEXUNIT unit)
{
    gpu3dsBindTexture(snesSubScreenTarget, unit);
}
