//=============================================================================
// Contains all the hooks and interfaces between the emulator interface
// and the main emulator core.
//=============================================================================

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <3ds.h>

#include <dirent.h>
#include "snes9x.h"
#include "memmap.h"
#include "apu.h"
#include "gfx.h"
#include "snapshot.h"
#include "cheats.h"
#include "movie.h"
#include "display.h"
#include "soundux.h"

#include "3dssnes9x.h"
#include "3dsexit.h"
#include "3dsgpu.h"
#include "3dssound.h"
#include "3dsui.h"
#include "3dsinput.h"
#include "3dssettings.h"
#include "3dsimpl.h"
#include "3dsimpl_tilecache.h"
#include "3dsimpl_gpu.h"

#include "blargsnes_spc700/dsp.h"
#include "blargsnes_spc700/spc700.h"

// Compiled shaders
//
#include "shaderfast_shbin.h"
#include "shaderfast2_shbin.h"
#include "shaderfast3_shbin.h"
#include "shaderfastm7_shbin.h"

#include "shaderslow_shbin.h"
#include "shaderslow2_shbin.h"
#include "shaderslow3_shbin.h"
#include "shaderslowm7_shbin.h"


//------------------------------------------------------------------------
// Memory Usage = 0.26 MB   for 4-point rectangle (triangle strip) vertex buffer
#define RECTANGLE_BUFFER_SIZE           0x40000

//------------------------------------------------------------------------
// Memory Usage = 8.00 MB   for 6-point quad vertex buffer (Citra only)
#define CITRA_VERTEX_BUFFER_SIZE        0x800000

// Memory Usage = Not used (Real 3DS only)
#define CITRA_TILE_BUFFER_SIZE          0x200

// Memory usage = 2.00 MB   for 6-point full texture mode 7 update buffer
#define CITRA_M7_BUFFER_SIZE            0x200000

// Memory usage = 0.39 MB   for 2-point mode 7 scanline draw
#define CITRA_MODE7_LINE_BUFFER_SIZE    0x60000


//------------------------------------------------------------------------
// Memory Usage = 0.06 MB   for 6-point quad vertex buffer (Real 3DS only)
#define REAL3DS_VERTEX_BUFFER_SIZE      0x1000

// Memory Usage = 3.00 MB   for 2-point rectangle vertex buffer (Real 3DS only)
#define REAL3DS_TILE_BUFFER_SIZE        0x300000

// Memory usage = 0.78 MB   for 2-point full texture mode 7 update buffer
#define REAL3DS_M7_BUFFER_SIZE          0xC0000

// Memory usage = 0.13 MB   for 2-point mode 7 scanline draw
#define REAL3DS_MODE7_LINE_BUFFER_SIZE  0x20000


//---------------------------------------------------------
// Our textures
//---------------------------------------------------------
SGPUTexture *snesMainScreenTarget;
SGPUTexture *snesSubScreenTarget;

SGPUTexture *snesTileCacheTexture;
SGPUTexture *snesMode7FullTexture;
SGPUTexture *snesMode7TileCacheTexture;
SGPUTexture *snesMode7Tile0Texture;

SGPUTexture *snesDepthForScreens;
SGPUTexture *snesDepthForOtherTextures;


// Settings related to the emulator.
//
extern S9xSettings3DS settings3DS;


//---------------------------------------------------------
// Initializes the emulator core.
//
// You must call snd3dsSetSampleRate here to set 
// the CSND's sampling rate.
//---------------------------------------------------------
bool impl3dsInitializeCore()
{
	// Initialize our CSND engine.
	//
	snd3dsSetSampleRate(true, 32000, 125, true, 4, 8);
	S9xSetPlaybackRate(32000);
	DspReset();
	IAPU.DSPReplayIndex = 0;
	IAPU.DSPWriteIndex = 0;
	
	// Initialize our tile cache engine.
	//
    cache3dsInit();

	// Initialize our GPU.
	// Load up and initialize any shaders
	//
    if (GPU3DS.isReal3DS)
    {
        gpu3dsLoadShader(0, (u32 *)shaderfast_shbin, shaderfast_shbin_size, 6);
    	gpu3dsLoadShader(1, (u32 *)shaderslow_shbin, shaderslow_shbin_size, 0);     // copy to screen
    	gpu3dsLoadShader(2, (u32 *)shaderfast2_shbin, shaderfast2_shbin_size, 6);   // draw tiles
        gpu3dsLoadShader(3, (u32 *)shaderfastm7_shbin, shaderfastm7_shbin_size, 3); // mode 7 shader
    }
    else
    {
    	gpu3dsLoadShader(0, (u32 *)shaderslow_shbin, shaderslow_shbin_size, 0);
    	gpu3dsLoadShader(1, (u32 *)shaderslow_shbin, shaderslow_shbin_size, 0);     // copy to screen
        gpu3dsLoadShader(2, (u32 *)shaderslow2_shbin, shaderslow2_shbin_size, 0);   // draw tiles
        gpu3dsLoadShader(3, (u32 *)shaderslowm7_shbin, shaderslowm7_shbin_size, 0); // mode 7 shader
    }

	gpu3dsInitializeShaderRegistersForRenderTarget(0, 10);
	gpu3dsInitializeShaderRegistersForTexture(4, 14);
	gpu3dsInitializeShaderRegistersForTextureOffset(6);
	gpu3dsUseShader(0);

    // Create all the necessary textures
    //
    snesTileCacheTexture = gpu3dsCreateTextureInLinearMemory(1024, 1024, GPU_RGBA5551);
    snesMode7TileCacheTexture = gpu3dsCreateTextureInLinearMemory(128, 128, GPU_RGBA4);

    // This requires 16x16 texture as a minimum
    snesMode7Tile0Texture = gpu3dsCreateTextureInVRAM(16, 16, GPU_RGBA4);    //
    snesMode7FullTexture = gpu3dsCreateTextureInVRAM(1024, 1024, GPU_RGBA4); // 2.000 MB

    // Main screen requires 8-bit alpha, otherwise alpha blending will not work well
    snesMainScreenTarget = gpu3dsCreateTextureInVRAM(256, 256, GPU_RGBA8);      // 0.250 MB
    snesSubScreenTarget = gpu3dsCreateTextureInVRAM(256, 256, GPU_RGBA8);       // 0.250 MB

    // Depth texture for the sub / main screens.
    // Performance: Create depth buffers in VRAM improves GPU performance!
    //              Games like Axelay, F-Zero (EUR) now run close to full speed!
    //
    snesDepthForScreens = gpu3dsCreateTextureInVRAM(256, 256, GPU_RGBA8);       // 0.250 MB
    snesDepthForOtherTextures = gpu3dsCreateTextureInVRAM(512, 512, GPU_RGBA8); // 1.000 MB

    if (snesTileCacheTexture == NULL || snesMode7FullTexture == NULL ||
        snesMode7TileCacheTexture == NULL || snesMode7Tile0Texture == NULL ||
        snesMainScreenTarget == NULL || snesSubScreenTarget == NULL ||
        snesDepthForScreens == NULL  || snesDepthForOtherTextures == NULL)
    {
        printf ("Unable to allocate textures\n");
        return false;
    }

    if (GPU3DS.isReal3DS)
    {
        gpu3dsAllocVertexList(&GPU3DSExt.rectangleVertexes, RECTANGLE_BUFFER_SIZE, sizeof(SVertexColor), 2, SVERTEXCOLOR_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DSExt.mode7TileVertexes, sizeof(SMode7TileVertex) * 16400 * 1 * 2 + 0x200, sizeof(SMode7TileVertex), 2, SMODE7TILEVERTEX_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DSExt.quadVertexes, REAL3DS_VERTEX_BUFFER_SIZE, sizeof(STileVertex), 2, STILEVERTEX_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DSExt.tileVertexes, REAL3DS_TILE_BUFFER_SIZE, sizeof(STileVertex), 2, STILEVERTEX_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DSExt.mode7LineVertexes, REAL3DS_MODE7_LINE_BUFFER_SIZE, sizeof(SMode7LineVertex), 2, SMODE7LINEVERTEX_ATTRIBFORMAT);
    }
    else
    {
        gpu3dsAllocVertexList(&GPU3DSExt.rectangleVertexes, RECTANGLE_BUFFER_SIZE, sizeof(SVertexColor), 2, SVERTEXCOLOR_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DSExt.mode7TileVertexes, sizeof(SMode7TileVertex) * 16400 * 6 * 2 + 0x200, sizeof(SMode7TileVertex), 2, SMODE7TILEVERTEX_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DSExt.quadVertexes, CITRA_VERTEX_BUFFER_SIZE, sizeof(STileVertex), 2, STILEVERTEX_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DSExt.tileVertexes, CITRA_TILE_BUFFER_SIZE, sizeof(STileVertex), 2, STILEVERTEX_ATTRIBFORMAT);
        gpu3dsAllocVertexList(&GPU3DSExt.mode7LineVertexes, CITRA_MODE7_LINE_BUFFER_SIZE, sizeof(SMode7LineVertex), 2, SMODE7LINEVERTEX_ATTRIBFORMAT);
    }

    if (GPU3DSExt.quadVertexes.ListBase == NULL ||
        GPU3DSExt.tileVertexes.ListBase == NULL ||
        GPU3DSExt.rectangleVertexes.ListBase == NULL ||
        GPU3DSExt.mode7TileVertexes.ListBase == NULL ||
        GPU3DSExt.mode7LineVertexes.ListBase == NULL)
    {
        printf ("Unable to allocate vertex list buffers \n");
        return false;
    }

	// Initialize the vertex list for mode 7.
	//
    gpu3dsInitializeMode7Vertexes();



	// Initialize our SNES core
	//
    memset(&Settings, 0, sizeof(Settings));
    Settings.Paused = false;
    Settings.BGLayering = TRUE;
    Settings.SoundBufferSize = 0;
    Settings.CyclesPercentage = 100;
    Settings.APUEnabled = Settings.NextAPUEnabled = TRUE;
    Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
    Settings.SkipFrames = 0;
    Settings.ShutdownMaster = TRUE;
    Settings.FrameTimePAL = 20000;
    Settings.FrameTimeNTSC = 16667;
    Settings.FrameTime = Settings.FrameTimeNTSC;
    Settings.DisableSampleCaching = FALSE;
    Settings.DisableMasterVolume = FALSE;
    Settings.Mouse = FALSE;
    Settings.SuperScope = FALSE;
    Settings.MultiPlayer5 = FALSE;
    Settings.ControllerOption = SNES_JOYPAD;
    Settings.SupportHiRes = FALSE;
    Settings.NetPlay = FALSE;
    Settings.ServerName [0] = 0;
    Settings.ThreadSound = FALSE;
    Settings.AutoSaveDelay = 60;         // Bug fix to save SRAM within 60 frames (1 second instead of 30 seconds)
#ifdef _NETPLAY_SUPPORT
    Settings.Port = NP_DEFAULT_PORT;
#endif
    Settings.ApplyCheats = TRUE;
    Settings.TurboMode = FALSE;
    Settings.TurboSkipFrames = 15;

    Settings.Transparency = FALSE;
    Settings.SixteenBit = TRUE;
    Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;

    // Sound related settings.
    Settings.DisableSoundEcho = FALSE;
    Settings.SixteenBitSound = TRUE;
    Settings.SoundPlaybackRate = 32000;
    Settings.Stereo = TRUE;
    Settings.SoundBufferSize = 0;
    Settings.APUEnabled = Settings.NextAPUEnabled = TRUE;
    Settings.InterpolatedSound = TRUE;
    Settings.AltSampleDecode = 0;
    Settings.SoundEnvelopeHeightReading = 1;
	Settings.UseFastDSPCore = 1;

    if(!Memory.Init())
    {
        printf ("Unable to initialize memory.\n");
        return false;
    }

    if(!S9xInitAPU())
    {
        printf ("Unable to initialize APU.\n");
        return false;
    }
	SPC_RAM = IAPU.RAM;

    if(!S9xGraphicsInit())
    {
        printf ("Unable to initialize graphics.\n");
        return false;
    }


    if(!S9xInitSound (
        7, Settings.Stereo,
        Settings.SoundBufferSize))
    {
        printf ("Unable to initialize sound.\n");
        return false;
    }
    so.playback_rate = Settings.SoundPlaybackRate;
    so.stereo = Settings.Stereo;
    so.sixteen_bit = Settings.SixteenBitSound;
    so.buffer_size = 32768;
    so.encoded = FALSE;


    return true;
}


//---------------------------------------------------------
// Finalizes and frees up any resources.
//---------------------------------------------------------
void impl3dsFinalize()
{
	// Frees up all vertex lists
	//
    gpu3dsDeallocVertexList(&GPU3DSExt.mode7TileVertexes);
    gpu3dsDeallocVertexList(&GPU3DSExt.rectangleVertexes);
    gpu3dsDeallocVertexList(&GPU3DSExt.quadVertexes);
    gpu3dsDeallocVertexList(&GPU3DSExt.tileVertexes);
    gpu3dsDeallocVertexList(&GPU3DSExt.mode7LineVertexes);
	
	// Frees up all textures.
	//
    gpu3dsDestroyTextureFromLinearMemory(snesTileCacheTexture);
    gpu3dsDestroyTextureFromLinearMemory(snesMode7TileCacheTexture);

    gpu3dsDestroyTextureFromVRAM(snesMode7Tile0Texture);
    gpu3dsDestroyTextureFromVRAM(snesMode7FullTexture);
    gpu3dsDestroyTextureFromVRAM(snesMainScreenTarget);
    gpu3dsDestroyTextureFromVRAM(snesSubScreenTarget);

    // Small bug fix. Previously forgot to destroy textures.
    //
    gpu3dsDestroyTextureFromVRAM(snesDepthForOtherTextures);
    gpu3dsDestroyTextureFromVRAM(snesDepthForScreens);

#ifndef RELEASE
    printf("S9xGraphicsDeinit:\n");
#endif
    S9xGraphicsDeinit();

#ifndef RELEASE
    printf("S9xDeinitAPU:\n");
#endif
    S9xDeinitAPU();
    
#ifndef RELEASE
    printf("Memory.Deinit:\n");
#endif
    Memory.Deinit();

	
}


extern "C" void DspGenerateNoise();
extern "C" void DSP_ReplayWrites(u32 idx);
extern "C" void DspMixSamplesStereo(u32 samples, s16 *mixBuf);

//---------------------------------------------------------
// Mix sound samples into a temporary buffer.
//
// This gives time for the sound generation to execute
// from the 2nd core before copying it to the actual
// output buffer.
//---------------------------------------------------------
void impl3dsGenerateSoundSamples(int numberOfSamples)
{
	if (Settings.UseFastDSPCore)
	{
		S9xSetAPUDSPClearWrites();
	}
	else
	{
		DSP_ClearWrites();
		S9xSetAPUDSPReplay ();
		S9xMixSamplesIntoTempBuffer(numberOfSamples * 2);
	}
}


//---------------------------------------------------------
// Mix sound samples into a temporary buffer.
//
// This gives time for the sound generation to execute
// from the 2nd core before copying it to the actual
// output buffer.
//---------------------------------------------------------
void impl3dsOutputSoundSamples(int numberOfSamples, short *leftSamples, short *rightSamples)
{
	if (Settings.UseFastDSPCore)
	{
		DSP_ReplayWrites(0);
		DspGenerateNoise();
		DspMixSamplesStereo(256, leftSamples);
	}
	else
	{
		S9xApplyMasterVolumeOnTempBufferIntoLeftRightBuffers(leftSamples, rightSamples, 256 * 2);
	}
}

//---------------------------------------------------------
// This is called when a ROM needs to be loaded and the
// emulator engine initialized.
//---------------------------------------------------------
void impl3dsLoadROM(char *romFilePath)
{
    bool loaded = Memory.LoadROM(romFilePath);
    Memory.LoadSRAM (S9xGetFilename (".srm"));

    gpu3dsInitializeMode7Vertexes();
    gpu3dsCopyVRAMTilesIntoMode7TileVertexes(Memory.VRAM);
    cache3dsInit();	

	DspReset();
}


//---------------------------------------------------------
// This is called when the user chooses to reset the
// console
//---------------------------------------------------------
void impl3dsResetConsole()
{
	S9xReset();
	DspReset();
	
	cache3dsInit();
	gpu3dsInitializeMode7Vertexes();
	gpu3dsCopyVRAMTilesIntoMode7TileVertexes(Memory.VRAM);
}


//---------------------------------------------------------
// This is called when preparing to start emulating
// a new frame. Use this to do any preparation of data
// and the hardware before the frame is emulated.
//---------------------------------------------------------
void impl3dsPrepareForNewFrame()
{
    gpu3dsSwapVertexListForNextFrame(&GPU3DSExt.quadVertexes);
    gpu3dsSwapVertexListForNextFrame(&GPU3DSExt.tileVertexes);
    gpu3dsSwapVertexListForNextFrame(&GPU3DSExt.rectangleVertexes);
    gpu3dsSwapVertexListForNextFrame(&GPU3DSExt.mode7LineVertexes);	
}


//---------------------------------------------------------
// Executes one frame.
//---------------------------------------------------------
void impl3dsRunOneFrame(bool firstFrame, bool skipDrawingFrame)
{
	Memory.ApplySpeedHackPatches();
	gpu3dsEnableAlphaBlending();

	if (GPU3DS.emulatorState != EMUSTATE_EMULATE)
		return;

	IPPU.RenderThisFrame = !skipDrawingFrame;

	gpu3dsSetRenderTargetToMainScreenTexture();
	gpu3dsUseShader(2);             // for drawing tiles

#ifdef RELEASE
	if (!Settings.SA1)
		S9xMainLoop();
	else
		S9xMainLoopWithSA1();
#else
	if (!Settings.Paused)
	{
		if (!Settings.SA1)
			S9xMainLoop();
		else
			S9xMainLoopWithSA1();
	}
#endif

	// ----------------------------------------------
	// Copy the SNES main/sub screen to the 3DS frame
	// buffer
	// (Can this be done in the V_BLANK?)
	t3dsStartTiming(3, "CopyFB");
	gpu3dsSetRenderTargetToTopFrameBuffer();

	if (firstFrame)
	{
		// Clear the entire frame buffer to black, including the borders
		//
		gpu3dsDisableAlphaBlending();
		gpu3dsSetTextureEnvironmentReplaceColor();
		gpu3dsDrawRectangle(0, 0, 400, 240, 0, 0x000000ff);
		gpu3dsEnableAlphaBlending();
	}

	gpu3dsUseShader(1);             // for copying to screen.
	gpu3dsDisableAlphaBlending();
	gpu3dsDisableDepthTest();
	gpu3dsDisableAlphaTest();

	gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
	gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsDisableStencilTest();

	int sWidth = settings3DS.StretchWidth;
	int sHeight = (settings3DS.StretchHeight == -1 ? PPU.ScreenHeight - 1 : settings3DS.StretchHeight);
	if (sWidth == 04030000)
		sWidth = sHeight * 4 / 3;
	else if (sWidth == 01010000)
	{
		if (PPU.ScreenHeight < SNES_HEIGHT_EXTENDED)
		{
			sWidth = SNES_HEIGHT_EXTENDED * SNES_WIDTH / SNES_HEIGHT;
			sHeight = SNES_HEIGHT_EXTENDED;
		}
		else
		{
			sWidth = SNES_WIDTH;
			sHeight = SNES_HEIGHT_EXTENDED;
		}
	}

	int sx0 = (400 - sWidth) / 2;
	int sx1 = sx0 + sWidth;
	int sy0 = (240 - sHeight) / 2;
	int sy1 = sy0 + sHeight;

	gpu3dsAddQuadVertexes(
		sx0, sy0, sx1, sy1,
		settings3DS.CropPixels, settings3DS.CropPixels ? settings3DS.CropPixels : 1, 
		256 - settings3DS.CropPixels, PPU.ScreenHeight - settings3DS.CropPixels, 
		0.1f);
	gpu3dsDrawVertexes();

	t3dsEndTiming(3);

	if (!firstFrame)
	{
		// ----------------------------------------------
		// Wait for the rendering to the SNES
		// main/sub screen for the previous frame
		// to complete
		//
		t3dsStartTiming(5, "Transfer");
		gpu3dsTransferToScreenBuffer();
		gpu3dsSwapScreenBuffers();
		t3dsEndTiming(5);

	}
	else
	{
		firstFrame = false;
	}

	// ----------------------------------------------
	// Flush all draw commands of the current frame
	// to the GPU.
	t3dsStartTiming(4, "Flush");
	gpu3dsFlush();
	t3dsEndTiming(4);

	t3dsEndTiming(1);


	// For debugging only.
	/*if (!GPU3DS.isReal3DS)
	{
		snd3dsMixSamples();
		//snd3dsMixSamples();
		//printf ("---\n");
	}*/

	/*
	// Debugging only
	snd3dsMixSamples();
	printf ("\n");

	S9xPrintAPUState ();
	printf ("----\n");*/
	
}


//---------------------------------------------------------
// This is called when the bottom screen is touched
// during emulation, and the emulation engine is ready
// to display the pause menu.
//---------------------------------------------------------
void impl3dsTouchScreenPressed()
{
	// Save the SRAM if it has been modified before we going
	// into the menu.
	//
	if (settings3DS.ForceSRAMWriteOnPause || CPU.SRAMModified || CPU.AutoSaveTimer)
	{
		S9xAutoSaveSRAM();
	}
}


//---------------------------------------------------------
// This is called when the user chooses to save the state.
// This function should save the state into a file whose
// name contains the slot number. This will return
// true if the state is saved successfully.
//---------------------------------------------------------
bool impl3dsSaveStateSlot(int slotNumber)
{
	char s[_MAX_PATH];
	sprintf(s, ".%d.frz", slotNumber);
	return impl3dsSaveState(S9xGetFilename(s));
}

bool impl3dsSaveStateAuto()
{
	return impl3dsSaveState(S9xGetFilename(".auto.frz"));
}

bool impl3dsSaveState(const char* filename)
{
	return Snapshot(filename);
}

//---------------------------------------------------------
// This is called when the user chooses to load the state.
// This function should save the state into a file whose
// name contains the slot number. This will return
// true if the state is loaded successfully.
//---------------------------------------------------------
bool impl3dsLoadStateSlot(int slotNumber)
{
	char s[_MAX_PATH];
	sprintf(s, ".%d.frz", slotNumber);
	return impl3dsLoadState(S9xGetFilename(s));
}

bool impl3dsLoadStateAuto()
{
	return impl3dsLoadState(S9xGetFilename(".auto.frz"));
}

bool impl3dsLoadState(const char* filename)
{
	bool success = S9xLoadSnapshot(filename);
	if (success)
	{
		gpu3dsInitializeMode7Vertexes();
		gpu3dsCopyVRAMTilesIntoMode7TileVertexes(Memory.VRAM);
	}
	return success;
}


//=============================================================================
// Snes9x related functions
//=============================================================================
void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext)
{
	*drive = 0;

	const char	*slash = strrchr(path, SLASH_CHAR),
				*dot   = strrchr(path, '.');

	if (dot && slash && dot < slash)
		dot = NULL;

	if (!slash)
	{
		*dir = 0;

		strcpy(fname, path);

		if (dot)
		{
			fname[dot - path] = 0;
			strcpy(ext, dot + 1);
		}
		else
			*ext = 0;
	}
	else
	{
		strcpy(dir, path);
		dir[slash - path] = 0;

		strcpy(fname, slash + 1);

		if (dot)
		{
			fname[dot - slash - 1] = 0;
			strcpy(ext, dot + 1);
		}
		else
			*ext = 0;
	}
}

void _makepath (char *path, const char *, const char *dir, const char *fname, const char *ext)
{
	if (dir && *dir)
	{
		strcpy(path, dir);
		strcat(path, SLASH_STR);
	}
	else
		*path = 0;

	strcat(path, fname);

	if (ext && *ext)
	{
		strcat(path, ".");
		strcat(path, ext);
	}
}

void S9xMessage (int type, int number, const char *message)
{
	//printf("%s\n", message);
}

bool8 S9xInitUpdate (void)
{
	return (TRUE);
}

bool8 S9xDeinitUpdate (int width, int height, bool8 sixteen_bit)
{
	return (TRUE);
}



void S9xAutoSaveSRAM (void)
{
    // Ensure that the timer is reset
    //
    //CPU.AccumulatedAutoSaveTimer = 0;
    CPU.SRAMModified = false;

    ui3dsDrawRect(50, 140, 270, 154, 0x000000);
    ui3dsDrawStringWithNoWrapping(50, 140, 270, 154, 0x3f7fff, HALIGN_CENTER, "Saving SRAM to SD card...");

    // Bug fix: Instead of stopping CSND, we generate silence
    // like we did prior to v0.61
    //
    snd3DS.generateSilence = true;

    //int millisecondsToWait = 5;
    //svcSleepThread ((long)(millisecondsToWait * 1000));

	Memory.SaveSRAM (S9xGetFilename (".srm"));

    ui3dsDrawRect(50, 140, 270, 154, 0x000000);

    // Bug fix: Instead of starting CSND, we continue to mix
    // like we did prior to v0.61
    //
    snd3DS.generateSilence = false;
}

void S9xGenerateSound ()
{
}


void S9xExit (void)
{

}

void S9xSetPalette (void)
{
	return;
}


bool8 S9xOpenSoundDevice(int mode, bool8 stereo, int buffer_size)
{
	return (TRUE);
}

void S9xLoadSDD1Data ()
{
    //Settings.SDD1Pack=FALSE;

    char filename [_MAX_PATH + 1];
    char index [_MAX_PATH + 1];
    char data [_MAX_PATH + 1];

	Settings.SDD1Pack=FALSE;
    Memory.FreeSDD1Data ();

    if (strncmp (Memory.ROMName, ("Star Ocean"), 10) == 0)
	{
		Settings.SDD1Pack=TRUE;
	}
    else if(strncmp(Memory.ROMName, ("STREET FIGHTER ALPHA2"), 21)==0)
	{
		if(Memory.ROMRegion==1)
		{
			Settings.SDD1Pack=TRUE;
		}
		else
		{
			Settings.SDD1Pack=TRUE;
		}
	}
	else
	{
		Settings.SDD1Pack=TRUE;
	}
}

const char * S9xGetFilename (const char *ex)
{
	static char	s[PATH_MAX + 1];
	char		drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

	_splitpath(Memory.ROMFilename, drive, dir, fname, ext);
	snprintf(s, PATH_MAX + 1, "%s/%s%s", dir, fname, ex);

	return (s);
}

const char * S9xGetFilenameInc (const char *ex)
{
	static char	s[PATH_MAX + 1];
	char		drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

	unsigned int	i = 0;
	const char		*d;
	struct stat		buf;

	_splitpath(Memory.ROMFilename, drive, dir, fname, ext);

	do
		snprintf(s, PATH_MAX + 1, "%s/%s.%03d%s", dir, fname, i++, ex);
	while (stat(s, &buf) == 0 && i < 1000);

	return (s);
}


bool8 S9xReadMousePosition (int which1_0_to_1, int &x, int &y, uint32 &buttons)
{

}

bool8 S9xReadSuperScopePosition (int &x, int &y, uint32 &buttons)
{

}

bool JustifierOffscreen()
{
	return 0;
}

void JustifierButtons(uint32& justifiers)
{

}

char * osd_GetPackDir(void)
{

    return NULL;
}

const char *S9xBasename (const char *f)
{
    const char *p;
    if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL)
	return (p + 1);

    if (p = strrchr (f, SLASH_CHAR))
	return (p + 1);

    return (f);
}

bool8 S9xOpenSnapshotFile (const char *filename, bool8 read_only, STREAM *file)
{

	char	s[PATH_MAX + 1];
	char	drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

    snprintf(s, PATH_MAX + 1, "%s", filename);

	if ((*file = OPEN_STREAM(s, read_only ? "rb" : "wb")))
		return (TRUE);

	return (FALSE);
}

void S9xCloseSnapshotFile (STREAM file)
{
	CLOSE_STREAM(file);
}

void S9xParseArg (char **argv, int &index, int argc)
{

}

void S9xExtraUsage ()
{

}

void S9xGraphicsMode ()
{

}
void S9xTextMode ()
{

}
void S9xSyncSpeed (void)
{
}

uint32 prevConsoleJoyPad = 0;
u32 prevConsoleButtonPressed[10];
u32 buttons3dsPressed[10];

uint32 S9xReadJoypad (int which1_0_to_4)
{
    if (which1_0_to_4 != 0)
        return 0;

	u32 keysHeld3ds = input3dsGetCurrentKeysHeld();
    u32 consoleJoyPad = 0;

    if (keysHeld3ds & KEY_UP) consoleJoyPad |= SNES_UP_MASK;
    if (keysHeld3ds & KEY_DOWN) consoleJoyPad |= SNES_DOWN_MASK;
    if (keysHeld3ds & KEY_LEFT) consoleJoyPad |= SNES_LEFT_MASK;
    if (keysHeld3ds & KEY_RIGHT) consoleJoyPad |= SNES_RIGHT_MASK;

	#define SET_CONSOLE_JOYPAD(i, mask, buttonMapping) 				\
		buttons3dsPressed[i] = (keysHeld3ds & mask);				\
		if (keysHeld3ds & mask) 									\
			consoleJoyPad |= 										\
				buttonMapping[i][0] |								\
				buttonMapping[i][1] |								\
				buttonMapping[i][2] |								\
				buttonMapping[i][3];								\

	if (settings3DS.UseGlobalButtonMappings)
	{
		SET_CONSOLE_JOYPAD(BTN3DS_L, KEY_L, settings3DS.GlobalButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_R, KEY_R, settings3DS.GlobalButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_A, KEY_A, settings3DS.GlobalButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_B, KEY_B, settings3DS.GlobalButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_X, KEY_X, settings3DS.GlobalButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_Y, KEY_Y, settings3DS.GlobalButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_SELECT, KEY_SELECT, settings3DS.GlobalButtonMapping);
		SET_CONSOLE_JOYPAD(BTN3DS_START, KEY_START, settings3DS.GlobalButtonMapping);
		SET_CONSOLE_JOYPAD(BTN3DS_ZL, KEY_ZL, settings3DS.GlobalButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_ZR, KEY_ZR, settings3DS.GlobalButtonMapping)
	}
	else
	{
		SET_CONSOLE_JOYPAD(BTN3DS_L, KEY_L, settings3DS.ButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_R, KEY_R, settings3DS.ButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_A, KEY_A, settings3DS.ButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_B, KEY_B, settings3DS.ButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_X, KEY_X, settings3DS.ButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_Y, KEY_Y, settings3DS.ButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_SELECT, KEY_SELECT, settings3DS.ButtonMapping);
		SET_CONSOLE_JOYPAD(BTN3DS_START, KEY_START, settings3DS.ButtonMapping);
		SET_CONSOLE_JOYPAD(BTN3DS_ZL, KEY_ZL, settings3DS.ButtonMapping)
		SET_CONSOLE_JOYPAD(BTN3DS_ZR, KEY_ZR, settings3DS.ButtonMapping)
	}


    // Handle turbo / rapid fire buttons.
    //
    int *turbo = settings3DS.Turbo;
    if (settings3DS.UseGlobalTurbo)
        turbo = settings3DS.GlobalTurbo;
    
    #define HANDLE_TURBO(i, buttonMapping) 										\
		if (settings3DS.Turbo[i] && buttons3dsPressed[i]) { 		\
			if (!prevConsoleButtonPressed[i]) 						\
			{ 														\
				prevConsoleButtonPressed[i] = 11 - turbo[i]; 		\
			} 														\
			else 													\
			{ 														\
				prevConsoleButtonPressed[i]--; 						\
				consoleJoyPad &= ~(									\
				buttonMapping[i][0] |								\
				buttonMapping[i][1] |								\
				buttonMapping[i][2] |								\
				buttonMapping[i][3]									\
				); \
			} \
		} \


	if (settings3DS.UseGlobalButtonMappings)
	{
		HANDLE_TURBO(BTN3DS_A, settings3DS.GlobalButtonMapping);
		HANDLE_TURBO(BTN3DS_B, settings3DS.GlobalButtonMapping);
		HANDLE_TURBO(BTN3DS_X, settings3DS.GlobalButtonMapping);
		HANDLE_TURBO(BTN3DS_Y, settings3DS.GlobalButtonMapping);
		HANDLE_TURBO(BTN3DS_L, settings3DS.GlobalButtonMapping);
		HANDLE_TURBO(BTN3DS_R, settings3DS.GlobalButtonMapping);
		HANDLE_TURBO(BTN3DS_ZL, settings3DS.GlobalButtonMapping);
		HANDLE_TURBO(BTN3DS_ZR, settings3DS.GlobalButtonMapping);
	}
	else
	{
		HANDLE_TURBO(BTN3DS_A, settings3DS.ButtonMapping);
		HANDLE_TURBO(BTN3DS_B, settings3DS.ButtonMapping);
		HANDLE_TURBO(BTN3DS_X, settings3DS.ButtonMapping);
		HANDLE_TURBO(BTN3DS_Y, settings3DS.ButtonMapping);
		HANDLE_TURBO(BTN3DS_L, settings3DS.ButtonMapping);
		HANDLE_TURBO(BTN3DS_R, settings3DS.ButtonMapping);
		HANDLE_TURBO(BTN3DS_ZL, settings3DS.ButtonMapping);
		HANDLE_TURBO(BTN3DS_ZR, settings3DS.ButtonMapping);
	}

    prevConsoleJoyPad = consoleJoyPad;

    return consoleJoyPad;
}
