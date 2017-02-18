#include "3dssnes9x.h"
#include "3dsgpu.h"

#ifndef _3DSIMPL_H
#define _3DSIMPL_H

//---------------------------------------------------------
// 3DS textures
//---------------------------------------------------------
extern SGPUTexture *snesMainScreenTarget;
extern SGPUTexture *snesSubScreenTarget;

extern SGPUTexture *snesTileCacheTexture;
extern SGPUTexture *snesMode7FullTexture;
extern SGPUTexture *snesMode7TileCacheTexture;
extern SGPUTexture *snesMode7Tile0Texture;

extern SGPUTexture *snesDepthForScreens;
extern SGPUTexture *snesDepthForOtherTextures;


//---------------------------------------------------------
// Initializes the emulator core.
//
// You must call snd3dsSetSampleRate here to set 
// the CSND's sampling rate.
//---------------------------------------------------------
bool impl3dsInitializeCore();


//---------------------------------------------------------
// Finalizes and frees up any resources.
//---------------------------------------------------------
void impl3dsFinalize();


//---------------------------------------------------------
// Mix sound samples into a temporary buffer.
//
// This gives time for the sound generation to execute
// from the 2nd core before copying it to the actual
// output buffer.
//---------------------------------------------------------
void impl3dsGenerateSoundSamples();


//---------------------------------------------------------
// Mix sound samples into a temporary buffer.
//
// This gives time for the sound generation to execute
// from the 2nd core before copying it to the actual
// output buffer.
//---------------------------------------------------------
void impl3dsOutputSoundSamples(short *leftSamples, short *rightSamples);


//---------------------------------------------------------
// This is called when a ROM needs to be loaded and the
// emulator engine initialized.
//---------------------------------------------------------
void impl3dsLoadROM(char *romFilePath);


//---------------------------------------------------------
// This is called when the user chooses to reset the
// console
//---------------------------------------------------------
void impl3dsResetConsole();


//---------------------------------------------------------
// This is called when preparing to start emulating
// a new frame. Use this to do any preparation of data
// and the hardware before the frame is emulated.
//---------------------------------------------------------
void impl3dsPrepareForNewFrame();


//---------------------------------------------------------
// Executes one frame.
//
// Note: TRUE will be passed in the firstFrame if this
// frame is to be run just after the emulator has booted
// up or returned from the menu.
//---------------------------------------------------------
void impl3dsRunOneFrame(bool firstFrame, bool skipDrawingFrame);


//---------------------------------------------------------
// This is called when the bottom screen is touched
// during emulation, and the emulation engine is ready
// to display the pause menu.
//---------------------------------------------------------
void impl3dsTouchScreenPressed();


//---------------------------------------------------------
// This is called when the user chooses to save the state.
// This function should save the state into a file whose
// name contains the slot number. This will return
// true if the state is saved successfully.
//---------------------------------------------------------
bool impl3dsSaveState(int slotNumber);


//---------------------------------------------------------
// This is called when the user chooses to load the state.
// This function should save the state into a file whose
// name contains the slot number. This will return
// true if the state is loaded successfully.
//---------------------------------------------------------
bool impl3dsLoadState(int slotNumber);

#endif