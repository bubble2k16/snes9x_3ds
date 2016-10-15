
#include "3ds.h"
#include "snes9x.h"
#include "ppu.h"

#ifndef _CLIPHW_H_
#define _CLIPHW_H_


#define WIN1_MASK			0x01
#define WIN2_MASK			0x02
#define WIN1XOR2_MASK		0x04


void ComputeClipWindowsForStenciling (
	uint32 window1Left, uint32 window1Right, uint32 window2Left, uint32 window2Right, 
	int *endX, int *stencilMask);

#endif