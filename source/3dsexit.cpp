#include <stdlib.h>
#include <3ds.h>

#include "3dsexit.h"
#include "3dsgpu.h"
#include "3dssound.h"
#include "memmap.h"
#include "snes9x.h"

aptHookCookie hookCookie;
int appExiting = 0;
int appSuspended = 0;

void handleAptHook(APT_HookType hook, void* param)
{
    switch (hook) {
        case APTHOOK_ONEXIT:
            appExiting = 1;
            break;
        case APTHOOK_ONSUSPEND:
        case APTHOOK_ONSLEEP:
            appSuspended = 1;
            if (GPU3DS.emulatorState == EMUSTATE_EMULATE) {
                snd3dsStopPlaying();
                if (CPU.SRAMModified || CPU.AutoSaveTimer) {
                    S9xAutoSaveSRAM();
                }
            }
            break;
        case APTHOOK_ONRESTORE:
        case APTHOOK_ONWAKEUP:
            appSuspended = 1;
            break;
    }
}

void enableAptHooks() {
    aptHook(&hookCookie, handleAptHook, NULL);
}
