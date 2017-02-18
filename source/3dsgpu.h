
#ifndef _3DSGPU_H_
#define _3DSGPU_H_

#include <3ds.h>
#include "gpulib.h"
#include "3dsmatrix.h"
#include "3dssnes9x.h"
#include "gfx.h"


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
    u8              TotalAttributes = 0;
    u64             AttributeFormats = 0;
    void            *List;
    int             Count = 0;
    GPU_Primitive_t Type;
} SStoredVertexList;


typedef struct
{
    GSPGPU_FramebufferFormats   screenFormat;
    GPU_TEXCOLOR                frameBufferFormat;

    u32                 *frameBuffer;
    u32                 *frameDepthBuffer;

    float               projectionTopScreen[16];
    float               projectionBottomScreen[16];
    float               textureOffset[4];

    SStoredVertexList   vertexesStored[4][10];

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

    int                 currentShader = -1;

    bool                isReal3DS = false;
    bool                enableDebug = false;
    int                 emulatorState = 0;

} SGPU3DS;


extern SGPU3DS GPU3DS;


#define EMUSTATE_EMULATE        1
#define EMUSTATE_PAUSEMENU      2
#define EMUSTATE_END            3



bool gpu3dsInitialize();
void gpu3dsFinalize();

void gpu3dsSetParallaxBarrier(bool enable);
void gpu3dsCheckSlider();

void gpu3dsAllocVertexList(SVertexList *list, int sizeInBytes, int vertexSize,
    u8 totalAttributes, u64 attributeFormats);
void gpu3dsDeallocVertexList(SVertexList *list);

SGPUTexture *gpu3dsCreateTextureInLinearMemory(int width, int height, GPU_TEXCOLOR pixelFormat);
SGPUTexture *gpu3dsCreateTextureInVRAM(int width, int height, GPU_TEXCOLOR pixelFormat);
void gpu3dsDestroyTextureFromLinearMemory(SGPUTexture *texture);
void gpu3dsDestroyTextureFromVRAM(SGPUTexture *texture);

int gpu3dsGetPixelSize(GPU_TEXCOLOR pixelFormat);

void gpu3dsStartNewFrame();

void gpu3dsCopyVRAMTilesIntoMode7TileVertexes(uint8 *VRAM);
void gpu3dsIncrementMode7UpdateFrameCount();

void gpu3dsResetState();

void gpu3dsInitializeShaderRegistersForRenderTarget(int vertexShaderRegister, int geometryShaderRegister);
void gpu3dsInitializeShaderRegistersForTexture(int vertexShaderRegister, int geometryShaderRegister);
void gpu3dsInitializeShaderRegistersForTextureOffset(int vertexShaderRegister);

void gpu3dsLoadShader(int shaderIndex, u32 *shaderBinary, int size, int geometryShaderStride);
void gpu3dsUseShader(int shaderIndex);

void gpu3dsSetRenderTargetToTopFrameBuffer();
void gpu3dsSetRenderTargetToTexture(SGPUTexture *texture, SGPUTexture *depthTexture);
void gpu3dsSetRenderTargetToTextureSpecific(SGPUTexture *texture, SGPUTexture *depthTexture, int addressOffset, int width, int height);

void gpu3dsFlush();
void gpu3dsWaitForPreviousFlush();
void gpu3dsFrameEnd();

void gpu3dsClearRenderTarget();
void gpu3dsTransferToScreenBuffer();
void gpu3dsSwapVertexListForNextFrame(SVertexList *list);
void gpu3dsSwapScreenBuffers();

void gpu3dsEnableAlphaTestNotEqualsZero();
void gpu3dsEnableAlphaTestEqualsOne();
void gpu3dsEnableAlphaTestEquals(uint8 alpha);
void gpu3dsEnableAlphaTestGreaterThanEquals(uint8 alpha);
void gpu3dsDisableAlphaTest();

void gpu3dsEnableDepthTestAndWriteColorAlphaOnly();
void gpu3dsEnableDepthTestAndWriteRedOnly();
void gpu3dsEnableDepthTest();
void gpu3dsDisableDepthTestAndWriteColorAlphaOnly();
void gpu3dsDisableDepthTestAndWriteColorOnly();
void gpu3dsDisableDepthTestAndWriteRedOnly();
void gpu3dsDisableDepthTest();

void gpu3dsEnableStencilTest(GPU_TESTFUNC func, u8 ref, u8 input_mask);
void gpu3dsDisableStencilTest();

void gpu3dsClearTextureEnv(u8 num);
void gpu3dsSetTextureEnvironmentReplaceColor();
void gpu3dsSetTextureEnvironmentReplaceColorButKeepAlpha();
void gpu3dsSetTextureEnvironmentReplaceTexture0();
void gpu3dsSetTextureEnvironmentReplaceTexture0WithFullAlpha();
void gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();
void gpu3dsSetTextureEnvironmentReplaceTexture0WithConstantAlpha(uint8 alpha);

void gpu3dsBindTexture(SGPUTexture *texture, GPU_TEXUNIT unit);
void gpu3dsBindTextureWithParams(SGPUTexture *texture, GPU_TEXUNIT unit, u32 param);

void gpu3dsScissorTest(GPU_SCISSORMODE mode, uint32 x, uint32 y, uint32 w, uint32 h);

void gpu3dsEnableAlphaBlending();
void gpu3dsEnableAdditiveBlending();
void gpu3dsEnableSubtractiveBlending();
void gpu3dsEnableAdditiveDiv2Blending();
void gpu3dsEnableSubtractiveDiv2Blending();
void gpu3dsDisableAlphaBlending();
void gpu3dsDisableAlphaBlendingKeepDestAlpha();

void gpu3dsSetTextureOffset(float u, float v);

void gpu3dsDrawVertexList(SVertexList *list, GPU_Primitive_t type, bool repeatLastDraw, int storeVertexListIndex, int storeIndex);
void gpu3dsDrawVertexList(SVertexList *list, GPU_Primitive_t type, int fromIndex, int tileCount);


#endif
