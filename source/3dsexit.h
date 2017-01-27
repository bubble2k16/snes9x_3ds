#ifndef _3DSEXIT_H_
#define _3DSEXIT_H_

#include <3ds.h>

extern aptHookCookie hookCookie;
extern int appExiting;

void setExitFlag(APT_HookType hook, void* param);
void enableExitHook();

#endif
