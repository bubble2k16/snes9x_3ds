#ifndef _3DSEXIT_H_
#define _3DSEXIT_H_

#include <3ds.h>

aptHookCookie hookCookie;
int appExiting = 0;

void setExitFlag(APT_HookType hook, void* param);
void enableExitHook();

#endif
