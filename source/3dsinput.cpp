
#include <3ds.h>
#include "3dsimpl.h"
#include "3dsgpu.h"
#include "3dssettings.h"

static u32 currKeysHeld = 0;
static u32 lastKeysHeld = 0;

extern S9xSettings3DS settings3DS;

//int adjustableValue = 0x70;

//---------------------------------------------------------
// Reads and processes Joy Pad buttons.
//
// This should be called only once every frame only in the
// emulator loop. For all other purposes, you should
// use the standard hidScanInput.
//---------------------------------------------------------
u32 input3dsScanInputForEmulation()
{
    hidScanInput();
    currKeysHeld = hidKeysHeld();

    u32 keysDown = (~lastKeysHeld) & currKeysHeld;

#ifndef RELEASE
    // -----------------------------------------------
    // For debug only
    // -----------------------------------------------
    if (GPU3DS.enableDebug)
    {
        keysDown = keysDown & (~lastKeysHeld);
        if (keysDown || (currKeysHeld & KEY_L))
        {
            //printf ("  kd:%x lkh:%x nkh:%x\n", keysDown, lastKeysHeld, currKeysHeld);
            //Settings.Paused = false;
        }
        else
        {
            //printf ("  kd:%x lkh:%x nkh:%x\n", keysDown, lastKeysHeld, currKeysHeld);
            //Settings.Paused = true;
        }
    }

    if (keysDown & (KEY_SELECT))
    {
        GPU3DS.enableDebug = !GPU3DS.enableDebug;
        printf ("Debug mode = %d\n", GPU3DS.enableDebug);
    }

    /*if (keysDown & (KEY_L))
    {
        adjustableValue -= 1;
        printf ("Adjust: %d\n", adjustableValue);
    }
    if (keysDown & (KEY_R))
    {
        adjustableValue += 1;
        printf ("Adjust: %d\n", adjustableValue);
    }*/
    // -----------------------------------------------
#endif

    if (keysDown & KEY_TOUCH || 
        (!settings3DS.UseGlobalEmuControlKeys && settings3DS.ButtonHotkeyOpenMenu.IsHeld(keysDown)) ||
        (settings3DS.UseGlobalEmuControlKeys && settings3DS.GlobalButtonHotkeyOpenMenu.IsHeld(keysDown))
        )
    {
        impl3dsTouchScreenPressed();

        if (GPU3DS.emulatorState == EMUSTATE_EMULATE)
            GPU3DS.emulatorState = EMUSTATE_PAUSEMENU;
    }
    lastKeysHeld = currKeysHeld;
    return keysDown;

}


//---------------------------------------------------------
// Get the bitmap of keys currently held on by the user
//---------------------------------------------------------
u32 input3dsGetCurrentKeysHeld()
{
    return currKeysHeld;
}