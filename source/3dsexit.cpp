#include <stdlib.h>
#include <3ds.h>

#include "3dsexit.h"

aptHookCookie hookCookie;
int appExiting = 0;

void setExitFlag(APT_HookType hook, void* param)
{
    if (hook == APTHOOK_ONEXIT) {
        appExiting = 1;
    }
}

void enableExitHook() {
    aptHook(&hookCookie, setExitFlag, NULL);
}
