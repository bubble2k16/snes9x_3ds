#include "3dssnes9x.h"
#include "3dsgpu.h"

#ifndef _3DSIMPL_H
#define _3DSIMPL_H

#define BTN3DS_A        0
#define BTN3DS_B        1
#define BTN3DS_X        2
#define BTN3DS_Y        3
#define BTN3DS_L        4
#define BTN3DS_R        5
#define BTN3DS_ZL       6
#define BTN3DS_ZR       7
#define BTN3DS_SELECT   8
#define BTN3DS_START    9

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
void impl3dsGenerateSoundSamples(int numberOfSamples);


//---------------------------------------------------------
// Mix sound samples into a temporary buffer.
//
// This gives time for the sound generation to execute
// from the 2nd core before copying it to the actual
// output buffer.
//---------------------------------------------------------
void impl3dsOutputSoundSamples(int numberOfSamples, short *leftSamples, short *rightSamples);


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
bool impl3dsSaveStateSlot(int slotNumber);


//---------------------------------------------------------
// This is called when a game or the emulator is exiting
// and the user has enabled the auto-savestate option.
// This saves the current game's state into a game-specific
// file whose name indicates that it's an automatic state.
// Returns true if the state has been saved successfully.
//---------------------------------------------------------
bool impl3dsSaveStateAuto();


//---------------------------------------------------------
// Saves the current game's state to the given filename.
// Returns true if the state has been saved successfully.
//---------------------------------------------------------
bool impl3dsSaveState(const char* filename);


//---------------------------------------------------------
// This is called when the user chooses to load the state.
// This function should load the state from the file that
// impl3dsSaveStateSlot() saves to when called with the
// same slotNumber.
// Returns true if the state has been loaded successfully.
//---------------------------------------------------------
bool impl3dsLoadStateSlot(int slotNumber);


//---------------------------------------------------------
// This is called when on game boot when the user has
// enabled the auto-savestate option.
// This loads the the state from the file that
// impl3dsSaveStateAuto() saves to.
// Returns true if the state has been saved successfully.
//---------------------------------------------------------
bool impl3dsLoadStateAuto();


//---------------------------------------------------------
// Loads the state from the given filename.
// Returns true if the state has been loaded successfully.
//---------------------------------------------------------
bool impl3dsLoadState(const char* filename);

#endif