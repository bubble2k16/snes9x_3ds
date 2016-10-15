
#ifndef _3DSGPU_H_
#define _3DSGPU_H_

#include <3ds.h>
#include "sf2d_private.h"
#include "3dssnes9x.h"
#include "gfx.h"

#define COMPOSE_HASH(vramAddr, pal)   ((vramAddr) << 4) + ((pal) & 0xf)

#define SUBSCREEN_Y         256

struct SVector3i
{
    int16 x, y, z;
};

struct SVector4i
{
    int16 x, y, z, w;
};


struct SVector3f
{
    float x, y, z;
};

struct STexCoord2i
{
    int16 u, v;
};

struct STexCoord2f
{
    float u, v;
};


struct STileVertex {
    struct SVector3i    Position;  
	struct STexCoord2i  TexCoord;  
};


struct SMode7TileVertex {
    struct SVector4i    Position;  
	struct STexCoord2i  TexCoord;  
};

struct SMode7LineVertex {
    struct SVector4i    Position;  
	struct STexCoord2f  TexCoord;  
};


struct SVertexColor {
    struct SVector4i    Position;  
	u32                 Color;
};


#define STILEVERTEX_ATTRIBFORMAT        GPU_ATTRIBFMT (0, 3, GPU_SHORT) | GPU_ATTRIBFMT (1, 2, GPU_SHORT)
#define SMODE7TILEVERTEX_ATTRIBFORMAT    GPU_ATTRIBFMT (0, 4, GPU_SHORT) | GPU_ATTRIBFMT (1, 2, GPU_SHORT)
#define SMODE7LINEVERTEX_ATTRIBFORMAT    GPU_ATTRIBFMT (0, 4, GPU_SHORT) | GPU_ATTRIBFMT (1, 2, GPU_FLOAT)
#define SVERTEXCOLOR_ATTRIBFORMAT       GPU_ATTRIBFMT(0, 4, GPU_SHORT) | GPU_ATTRIBFMT(1, 4, GPU_UNSIGNED_BYTE)


#define MAX_TEXTURE_POSITIONS		16383
#define MAX_HASH					(65536 * 16)


typedef struct
{
	int             Memory;                     // 0 = linear memory, 1 = VRAM
	GPU_TEXCOLOR    PixelFormat;  
	u32             Params;                
	int             Width;                 
	int             Height;                
	void            *PixelData;
    int             BufferSize;
	float           Projection[4*4];      /**< Orthographic projection matrix for this target */
    float           TextureScale[4];
} SGPUTexture;

typedef struct {
	DVLB_s 				*dvlb;
	shaderProgram_s 	shaderProgram;
	u32					projectionRegister;
} SGPUShader;


typedef struct
{
    u8              TotalAttributes = 0;
    u64             AttributeFormats = 0;
    int             VertexSize = 0;
    int             SizeInBytes = 0;
    int             FirstIndex = 0;
    int             Total = 0;
    int             Count = 0;
    void            *ListOriginal;
    void            *List;
    void            *ListBase;
    int             Flip = 0;

    void            *PrevList;
    u32             *PrevListOSAddr;
    int             PrevFirstIndex = 0;
    int             PrevCount = 0;

} SVertexList; 



typedef struct 
{
    GSPGPU_FramebufferFormats   screenFormat;
    GPU_TEXCOLOR                frameBufferFormat;
    
    u32                 *frameBuffer;
    u32                 *frameDepthBuffer;

    float               projectionTopScreen[16];
    float               projectionBottomScreen[16];

    SVertexList         quadVertexes;
    SVertexList         tileVertexes;
    SVertexList         mode7TileVertexes;
    SVertexList         mode7LineVertexes;
    SVertexList         rectangleVertexes;

    SGPUTexture         *currentTexture;
    SGPUTexture         *currentRenderTarget;
    SGPUTexture         *currentRenderTargetDepth;
    uint32              currentParams = 0;
    int                 targetDepthBufferSize = 0;
    void                *targetDepthBuffer;

    u32                 *currentAttributeBuffer = 0;
    u8                  currentTotalAttributes = 0;
    u32                 currentAttributeFormats = 0;

    SGPUShader          shaders[10];

    int                 mode7FrameCount = 0;
    float               mode7UpdateFrameCount[4];
    int                 currentShader = -1;

    // Memory Usage = 2.00 MB (for hashing of the texture position)
    uint16  vramCacheHashToTexturePosition[MAX_HASH + 1];               

    // Memory Usage = 0.06 MB
    int     vramCacheTexturePositionToHash[MAX_TEXTURE_POSITIONS];
  
    int     newCacheTexturePosition = 2;    

    bool    isReal3DS = false;
    bool    enableDebug = false;
    int     emulatorState = 0;
    
} SGPU3DS;


#ifndef _3DSGPU_CPP_
extern SGPU3DS GPU3DS;
#else
SGPU3DS GPU3DS;


#endif

#define EMUSTATE_EMULATE        1
#define EMUSTATE_PAUSEMENU      2
#define EMUSTATE_END            3


void cacheInit();

int cacheGetTexturePosition(int hash);

inline int cacheGetTexturePositionFast(int tileAddr, int pal)
{
    int hash = COMPOSE_HASH(tileAddr, pal);
    int pos = GPU3DS.vramCacheHashToTexturePosition[hash];
    
    if (pos == 0)
    {
        pos = GPU3DS.newCacheTexturePosition;
        
        //vramCacheFrameNumber[hash] = 0;
        
        GPU3DS.vramCacheTexturePositionToHash[GPU3DS.vramCacheHashToTexturePosition[hash] & 0xFFFE] = 0;

        GPU3DS.vramCacheHashToTexturePosition[GPU3DS.vramCacheTexturePositionToHash[pos]] = 0;
        
        GPU3DS.vramCacheHashToTexturePosition[hash] = pos;
        GPU3DS.vramCacheTexturePositionToHash[pos] = hash;
        
        GPU3DS.newCacheTexturePosition += 2;
        if (GPU3DS.newCacheTexturePosition >= MAX_TEXTURE_POSITIONS)
            GPU3DS.newCacheTexturePosition = 2;

        // Force this tile to re-decode. This fixes the tile corruption
        // problems when playing a game for too long.
        //
        GFX.VRAMPaletteFrame[tileAddr][pal] = 0;
    }
    
    return pos;
}

// Bug fix: This fixing the sprite flickering problem in games
// that updates the tile bitmaps mid-frame. Solves the flickering
// sprites in DKC2.
//
inline int cacheGetSwapTexturePositionForAltFrameFast(int tileAddr, int pal)
{
    int hash = COMPOSE_HASH(tileAddr, pal);
    int pos = GPU3DS.vramCacheHashToTexturePosition[hash] ^ 1;
    GPU3DS.vramCacheHashToTexturePosition[hash] = pos;
    return pos;
}

inline int cacheGetMode7TexturePositionFast(int tileNumber);

void gpu3dsCacheToTexturePosition(uint8 *snesTilePixels, uint16 *snesPalette, uint16 texturePosition);
void gpu3dsCacheToMode7TexturePosition(uint8 *snesTilePixels, uint16 *snesPalette, uint16 texturePosition, uint32 *paletteMask);
void gpu3dsCacheToMode7Tile0TexturePosition(uint8 *snesTilePixels, uint16 *snesPalette, uint16 texturePosition, uint32 *paletteMask);
    
bool gpu3dsInitialize();
void gpu3dsInitializeMode7Vertexes();
void gpu3dsFinalize();

SGPUTexture *gpu3dsCreateTextureInLinearMemory(int width, int height, GPU_TEXCOLOR pixelFormat);
SGPUTexture *gpu3dsCreateTextureInVRAM(int width, int height, GPU_TEXCOLOR pixelFormat);
void gpu3dsDestroyTextureFromLinearMemory(SGPUTexture *texture);
void gpu3dsDestroyTextureFromVRAM(SGPUTexture *texture);

void gpu3dsStartNewFrame();

void gpu3dsCopyVRAMTilesIntoMode7TileVertexes(uint8 *VRAM);
void gpu3dsIncrementMode7UpdateFrameCount();


void gpu3dsResetState(); 
void gpu3dsLoadShader(int shaderIndex, u32 *shaderBinary, int size, int geometryShaderStride);
void gpu3dsUseShader(int shaderIndex);

void gpu3dsSetRenderTargetToTopFrameBuffer();
void gpu3dsSetRenderTargetToMainScreenTexture();
void gpu3dsSetRenderTargetToSubScreenTexture();
void gpu3dsSetRenderTargetToDepthTexture();
void gpu3dsSetRenderTargetToMode7FullTexture(int pixelOffset, int width, int height);
void gpu3dsSetRenderTargetToMode7Tile0Texture();
void gpu3dsSetRenderTargetToOBJLayer();

void gpu3dsFlush();
void gpu3dsWaitForPreviousFlush();
void gpu3dsFrameEnd();

void gpu3dsClearRenderTarget();
void gpu3dsClearAllRenderTargets();
void gpu3dsTransferToScreenBuffer();
void gpu3dsSwapScreenBuffers();

void gpu3dsEnableAlphaTestNotEqualsZero();
void gpu3dsEnableAlphaTestEquals(uint8 alpha);
void gpu3dsDisableAlphaTest();

void gpu3dsEnableDepthTestAndWriteColorAlphaOnly();
void gpu3dsEnableDepthTestAndWriteRedOnly();
void gpu3dsEnableDepthTest();
void gpu3dsDisableDepthTestAndWriteColorAlphaOnly();
void gpu3dsDisableDepthTestAndWriteRedOnly();
void gpu3dsDisableDepthTest();

void gpu3dsEnableStencilTest(GPU_TESTFUNC func, u8 ref, u8 input_mask);
void gpu3dsDisableStencilTest();

void gpu3dsClearTextureEnv(u8 num);
void gpu3dsSetTextureEnvironmentReplaceColor();
void gpu3dsSetTextureEnvironmentReplaceTexture0();
void gpu3dsSetTextureEnvironmentReplaceTexture0WithFullAlpha();
void gpu3dsSetTextureEnvironmentReplaceTexture0WithConstantAlpha(uint8 alpha);

void gpu3dsBindTexture(SGPUTexture *texture, GPU_TEXUNIT unit);
void gpu3dsBindTextureDepthForScreens(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesTileCacheForHires(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7Tile0CacheRepeat(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7FullRepeat(GPU_TEXUNIT unit);
void gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT unit);
void gpu3dsBindTextureSubScreen(GPU_TEXUNIT unit);
void gpu3dsBindTextureMainScreen(GPU_TEXUNIT unit);
void gpu3dsBindTextureOBJLayer(GPU_TEXUNIT unit);

void gpu3dsScissorTest(GPU_SCISSORMODE mode, uint32 x, uint32 y, uint32 w, uint32 h);

void gpu3dsEnableAlphaBlending();
void gpu3dsEnableAdditiveBlending();
void gpu3dsEnableSubtractiveBlending();
void gpu3dsEnableAdditiveDiv2Blending();
void gpu3dsEnableSubtractiveDiv2Blending();
void gpu3dsDisableAlphaBlending();

inline void __attribute__((always_inline)) gpu3dsAddQuadVertexes(
    int x0, int y0, int x1, int y1,
    int tx0, int ty0, int tx1, int ty1,
    int data)
{
    STileVertex *vertices = &((STileVertex *) GPU3DS.quadVertexes.List)[GPU3DS.quadVertexes.Count];

	vertices[0].Position = (SVector3i){x0, y0, data};
	vertices[1].Position = (SVector3i){x1, y0, data};
	vertices[2].Position = (SVector3i){x0, y1, data};

	vertices[3].Position = (SVector3i){x1, y1, data};
	vertices[4].Position = (SVector3i){x0, y1, data};
	vertices[5].Position = (SVector3i){x1, y0, data};

	vertices[0].TexCoord = (STexCoord2i){tx0, ty0};
	vertices[1].TexCoord = (STexCoord2i){tx1, ty0};
	vertices[2].TexCoord = (STexCoord2i){tx0, ty1};
    
	vertices[3].TexCoord = (STexCoord2i){tx1, ty1};
	vertices[4].TexCoord = (STexCoord2i){tx0, ty1};
	vertices[5].TexCoord = (STexCoord2i){tx1, ty0};
    
    //GPU3DS.vertexCount += 6;
    GPU3DS.quadVertexes.Count += 6;
}


inline void __attribute__((always_inline)) gpu3dsAddTileVertexes(
    int x0, int y0, int x1, int y1, 
    int tx0, int ty0, int tx1, int ty1,
    int data)
{
#ifndef RELEASE_SHADER
    if (GPU3DS.isReal3DS)
    {
#endif
        //STileVertex *vertices = &GPU3DS.tileList[GPU3DS.tileCount];
        STileVertex *vertices = &((STileVertex *) GPU3DS.tileVertexes.List)[GPU3DS.tileVertexes.Count];

        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};
        
        vertices[1].Position = (SVector3i){x1, y1, data};
        vertices[1].TexCoord = (STexCoord2i){tx1, ty1};
        
        //GPU3DS.tileCount += 2;
        GPU3DS.tileVertexes.Count += 2;

#ifndef RELEASE_SHADER        
    }
    else
    {        
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        //STileVertex *vertices = &GPU3DS.vertexList[GPU3DS.vertexCount];
        STileVertex *vertices = &((STileVertex *) GPU3DS.quadVertexes.List)[GPU3DS.quadVertexes.Count];
        
        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[1].Position = (SVector3i){x1, y0, data};
        vertices[2].Position = (SVector3i){x0, y1, data};

        vertices[3].Position = (SVector3i){x1, y1, data};
        vertices[4].Position = (SVector3i){x0, y1, data};
        vertices[5].Position = (SVector3i){x1, y0, data};

        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};
        vertices[1].TexCoord = (STexCoord2i){tx1, ty0};
        vertices[2].TexCoord = (STexCoord2i){tx0, ty1};
        
        vertices[3].TexCoord = (STexCoord2i){tx1, ty1};
        vertices[4].TexCoord = (STexCoord2i){tx0, ty1};
        vertices[5].TexCoord = (STexCoord2i){tx1, ty0};
        
        //GPU3DS.vertexCount += 6;
        GPU3DS.quadVertexes.Count += 6;
    }
#endif

}


inline void __attribute__((always_inline)) gpu3dsAddMode7LineVertexes(
    int x0, int y0, int x1, int y1, 
    float tx0, float ty0, float tx1, float ty1)
{
#ifndef RELEASE_SHADER
    if (GPU3DS.isReal3DS)
    {
#endif
        SMode7LineVertex *vertices = &((SMode7LineVertex *) GPU3DS.mode7LineVertexes.List)[GPU3DS.mode7LineVertexes.Count];

        vertices[0].Position = (SVector4i){x0, y0, 0, 1};
        vertices[0].TexCoord = (STexCoord2f){tx0, ty0};
        
        // yes we will use a special value for the geometry shader to detect detect mode 7
        vertices[1].Position = (SVector4i){x1, -16384, 0, 1};
        vertices[1].TexCoord = (STexCoord2f){tx1, ty1};
        
        GPU3DS.mode7LineVertexes.Count += 2;

#ifndef RELEASE_SHADER        
    }
    else
    {        
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        SMode7LineVertex *vertices = &((SMode7LineVertex *) GPU3DS.mode7LineVertexes.List)[GPU3DS.mode7LineVertexes.Count];
        
        vertices[0].Position = (SVector4i){x0, y0, 0, 1};
        vertices[0].TexCoord = (STexCoord2f){tx0, ty0};

        vertices[1].Position = (SVector4i){x1, y0, 0, 1};
        vertices[1].TexCoord = (STexCoord2f){tx1, ty1};

        vertices[2].Position = (SVector4i){x0, y1, 0, 1};
        vertices[2].TexCoord = (STexCoord2f){tx0, ty0};
        

        vertices[3].Position = (SVector4i){x1, y0, 0, 1};
        vertices[3].TexCoord = (STexCoord2f){tx1, ty1};

        vertices[4].Position = (SVector4i){x1, y1, 0, 1};
        vertices[4].TexCoord = (STexCoord2f){tx1, ty1};

        vertices[5].Position = (SVector4i){x0, y1, 0, 1};
        vertices[5].TexCoord = (STexCoord2f){tx0, ty0};
        
        GPU3DS.mode7LineVertexes.Count += 6;
    }
#endif

}




inline void __attribute__((always_inline)) gpu3dsSetMode7TileTexturePos(int idx, int data)
{
#ifndef RELEASE_SHADER
    if (GPU3DS.isReal3DS)
    {
#endif
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.mode7TileVertexes.List) [idx];

        m7vertices[0].Position.z = data;
        //m7vertices[1].Position.z = data;
#ifndef RELEASE_SHADER        
    }
    else
    {        
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.mode7TileVertexes.List) [idx * 6];
        
        m7vertices[0].Position.z = data;
        m7vertices[1].Position.z = data;
        m7vertices[2].Position.z = data;
        m7vertices[3].Position.z = data;
        m7vertices[4].Position.z = data;
        m7vertices[5].Position.z = data;
    }
#endif
}



inline void __attribute__((always_inline)) gpu3dsSetMode7TileModifiedFlag(int idx)
{
    int updateFrame = GPU3DS.mode7FrameCount;

#ifndef RELEASE_SHADER
    if (GPU3DS.isReal3DS)
    {
#endif
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.mode7TileVertexes.List) [idx];

        m7vertices[0].Position.w = updateFrame;
        //m7vertices[1].Position.w = updateFrame;
#ifndef RELEASE_SHADER        
    }
    else 
    {        
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.mode7TileVertexes.List) [idx * 6];
        
        m7vertices[0].Position.w = updateFrame;
        m7vertices[1].Position.w = updateFrame;
        m7vertices[2].Position.w = updateFrame;
        m7vertices[3].Position.w = updateFrame;
        m7vertices[4].Position.w = updateFrame;
        m7vertices[5].Position.w = updateFrame;
    }
#endif
}


inline void __attribute__((always_inline)) gpu3dsSetMode7TileModifiedFlag(int idx, int updateFrame)
{
#ifndef RELEASE_SHADER
    if (GPU3DS.isReal3DS)
    {
#endif
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.mode7TileVertexes.List) [idx];

        m7vertices[0].Position.w = updateFrame;
        //m7vertices[1].Position.w = updateFrame;
#ifndef RELEASE_SHADER        
    }
    else 
    {        
        // This is used for testing in Citra, since Citra doesn't implement
        // the geometry shader required in the tile renderer
        //
        SMode7TileVertex *m7vertices = &((SMode7TileVertex *)GPU3DS.mode7TileVertexes.List) [idx * 6];
        
        m7vertices[0].Position.w = updateFrame;
        m7vertices[1].Position.w = updateFrame;
        m7vertices[2].Position.w = updateFrame;
        m7vertices[3].Position.w = updateFrame;
        m7vertices[4].Position.w = updateFrame;
        m7vertices[5].Position.w = updateFrame;
    }
#endif
}

inline void __attribute__((always_inline)) gpu3dsAddMode7ScanlineVertexes(
    int x0, int y0, int x1, int y1, 
    int tx0, int ty0, int tx1, int ty1,
    int data)
{
#ifndef RELEASE_SHADER    
    if (GPU3DS.isReal3DS)
    {
#endif        
        STileVertex *vertices = &((STileVertex *) GPU3DS.tileVertexes.List)[GPU3DS.tileVertexes.Count];

        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};

        // yes we will use a special value for the geometry shader to detect detect mode 7
        vertices[1].Position = (SVector3i){x1, -16384, data};      
        vertices[1].TexCoord = (STexCoord2i){tx1, ty1};
        
        //GPU3DS.tileCount += 2;
        GPU3DS.tileVertexes.Count += 2;

#ifndef RELEASE_SHADER
    }
    else
    {
        //STileVertex *vertices = &GPU3DS.vertexList[GPU3DS.vertexCount];
        STileVertex *vertices = &((STileVertex *) GPU3DS.quadVertexes.List)[GPU3DS.quadVertexes.Count];

        vertices[0].Position = (SVector3i){x0, y0, data};
        vertices[0].TexCoord = (STexCoord2i){tx0, ty0};

        vertices[1].Position = (SVector3i){x1, y0, data};
        vertices[1].TexCoord = (STexCoord2i){tx1, ty1};

        vertices[2].Position = (SVector3i){x0, y1, data};
        vertices[2].TexCoord = (STexCoord2i){tx0, ty0};
        

        vertices[3].Position = (SVector3i){x1, y0, data};
        vertices[3].TexCoord = (STexCoord2i){tx1, ty1};

        vertices[4].Position = (SVector3i){x1, y1, data};
        vertices[4].TexCoord = (STexCoord2i){tx1, ty1};

        vertices[5].Position = (SVector3i){x0, y1, data};
        vertices[5].TexCoord = (STexCoord2i){tx0, ty0};
        
        //GPU3DS.vertexCount += 6;
        GPU3DS.quadVertexes.Count += 6;
    }
#endif
}


void gpu3dsAddRectangleVertexes(int x0, int y0, int x1, int y1, int depth, u32 color);
void gpu3dsDrawVertexes(bool repeatLastDraw = false);
void gpu3dsDrawMode7Vertexes(int fromIndex, int tileCount);
void gpu3dsDrawMode7LineVertexes(bool repeatLastDraw = false);

void gpu3dsDrawRectangle(int x0, int y0, int x1, int y1, int depth, u32 color);

#endif

