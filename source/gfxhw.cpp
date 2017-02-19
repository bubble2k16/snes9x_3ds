/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.

  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman

  S-RTC C emulator code
  (c) Copyright 2001 John Weidman

  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault

  Super FX C emulator code
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se)


  Specific ports contains the works of other authors. See headers in
  individual files.

  Snes9x homepage: http://www.snes9x.com

  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.

  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/

#include "snes9x.h"

#include "memmap.h"
#include "ppu.h"
#include "cpuexec.h"
#include "display.h"
#include "gfx.h"
#include "apu.h"
#include "cheats.h"
#include "screenshot.h"
#include "cliphw.h"

#include <3ds.h>
#include "3dsopt.h"
#include "3dsgpu.h"
#include "3dsimpl_tilecache.h"
#include "3dsimpl_gpu.h"

#define M7 19
#define M8 19

void output_png();
void ComputeClipWindows ();
static void S9xDisplayFrameRate ();
static void S9xDisplayString (const char *string);

extern uint8 BitShifts[8][4];
extern uint8 TileShifts[8][4];
extern uint8 PaletteShifts[8][4];
extern uint8 PaletteMasks[8][4];
extern uint8 Depths[8][4];
extern uint8 BGSizes [2];

extern NormalTileRenderer DrawTilePtr;
extern ClippedTileRenderer DrawClippedTilePtr;
extern NormalTileRenderer DrawHiResTilePtr;
extern ClippedTileRenderer DrawHiResClippedTilePtr;
extern LargePixelRenderer DrawLargePixelPtr;

extern struct SBG BG;

extern struct SLineData LineData[240];
extern struct SLineMatrixData LineMatrixData [240];

extern uint8  Mode7Depths [2];

#define ON_MAIN(N) \
(GFX.r212c & (1 << (N)) && \
 !(PPU.BG_Forced & (1 << (N))))

#define SUB_OR_ADD(N) \
(GFX.r2131 & (1 << (N)))

#define ON_SUB(N) \
((GFX.r2130 & 0x30) != 0x30 && \
 (GFX.r2130 & 2) && \
 (GFX.r212d & (1 << N)) && \
 !(PPU.BG_Forced & (1 << (N))))

#define ON_SUB_PSEUDO(N) \
 ((GFX.r212d & (1 << N)))

#define ON_SUB_HIRES(N) \
((GFX.r212d & (1 << N)) && \
 !(PPU.BG_Forced & (1 << (N))))

#define ANYTHING_ON_SUB \
((GFX.r2130 & 0x30) != 0x30 && \
 (GFX.r2130 & 2) && \
 (GFX.r212d & 0x1f))

#define ADD_OR_SUB_ON_ANYTHING \
(GFX.r2131 & 0x3f)

#define ALPHA_DEFAULT		 		0x0000
#define ALPHA_ZERO 					0x6000 
#define ALPHA_0_5 					0x2000 
#define ALPHA_1_0 					0x4000 

bool layerDrawn[10];


//-------------------------------------------------------------------
// Render the backdrop
//-------------------------------------------------------------------
void S9xDrawBackdropHardware(bool sub, int depth)
{
	t3dsStartTiming(25, "DrawBKClr");
    uint32 starty = GFX.StartY;
    uint32 endy = GFX.EndY;

	gpu3dsSetTextureEnvironmentReplaceColor();
	gpu3dsDisableStencilTest();
	gpu3dsDisableDepthTest();
	gpu3dsDisableAlphaTest();
	
	if (!sub)
	{
		// Performance:
		// Use backdrop color sections for drawing backdrops.
		//
		for (int i = 0; i < IPPU.BackdropColorSections.Count; i++)
		{
			int backColor = IPPU.BackdropColorSections.Section[i].Value;

			backColor =
				((backColor & (0x1F << 11)) << 16) |
				((backColor & (0x1F << 6)) << 13)|
				((backColor & (0x1F << 1)) << 10) | 0xFF;

			if ((GFX.r2130 & 0xc0) == 0xc0)
			{
				//printf ("Backdrop black, Y:%d-%d, 2130:%02x\n", IPPU.BackdropColorSections.Section[i].StartY, IPPU.BackdropColorSections.Section[i].EndY, GFX.r2130);
				gpu3dsAddRectangleVertexes(
					0, IPPU.BackdropColorSections.Section[i].StartY + depth, 
					256, IPPU.BackdropColorSections.Section[i].EndY + 1 + depth, 0, 0xff);
			}
			else
			{
				//printf ("Backdrop %04x, Y:%d-%d, 2130:%02x, d=%4x\n", backColor, IPPU.BackdropColorSections.Section[i].StartY, IPPU.BackdropColorSections.Section[i].EndY, GFX.r2130, depth);
				gpu3dsAddRectangleVertexes(
					0, IPPU.BackdropColorSections.Section[i].StartY + depth, 
					256, IPPU.BackdropColorSections.Section[i].EndY + 1 + depth, 0, backColor);
			}

			/*
			// Bug fix: 
			// Ensure that the clip to black option in $2130 is respected
			// for backgrounds. This ensures that the fade to outdoor in
			// the prologue in Zelda doesn't show a nasty brown color outside
			// the window. 
			//
			if (!IPPU.Clip[0].Count[5])
			{
				gpu3dsAddRectangleVertexes(
					0, IPPU.BackdropColorSections.Section[i].StartY + depth, 
					256, IPPU.BackdropColorSections.Section[i].EndY + 1 + depth, 0, backColor);
			}
			else
			{
				// Bug fix: Draw a solid black first.
				// (previously we drew a black with alpha 0, and it causes 
				// Ghost Chaser Densei's rendering to 'saturate' during in-game)
				//
				gpu3dsAddRectangleVertexes(
					0, IPPU.BackdropColorSections.Section[i].StartY + depth, 
					256, IPPU.BackdropColorSections.Section[i].EndY + 1 + depth, 0, 0xff);

				// Then draw the actual backdrop color
				//
				for (int i = 0; i < IPPU.Clip[0].Count[5]; i++)
				{
					if (IPPU.Clip[0].Right[i][5] > IPPU.Clip[0].Left[i][5])
						gpu3dsAddRectangleVertexes(
							IPPU.Clip[0].Left[i][5], IPPU.BackdropColorSections.Section[i].StartY + depth, 
							IPPU.Clip[0].Right[i][5], IPPU.BackdropColorSections.Section[i].EndY + 1 + depth, 0, backColor);
				}
			} 
			*/

		}

		gpu3dsDrawVertexes();	
		
	}
	else
	{
		// Subscreen
		//
		int32 backColor = *((int32 *)(&LineData[GFX.StartY].FixedColour[0]));
		int32 starty = GFX.StartY;

		gpu3dsDisableAlphaTest();

		// Small performance improvement:
		// Use vertical sections to render the subscreen backdrop
		//
		for (int i = 0; i < IPPU.FixedColorSections.Count; i++)
		{
			int backColor = IPPU.FixedColorSections.Section[i].Value;

			// Bug fix: Ensures that the subscreen is cleared with a
			// transparent color. Otherwise, if the transparency (div 2)
			// is activated it can cause an ugly dark tint.
			// This fixes Chrono Trigger's Leene' Square dark floor and
			// Secret of Mana's dark grass.
			//
			if (backColor == 0xff) 
			{
				backColor = 0;
				depth = depth & 0xfff;		// removes the alpha component
			}
			
			gpu3dsAddRectangleVertexes(
				0, IPPU.FixedColorSections.Section[i].StartY + depth, 
				256, IPPU.FixedColorSections.Section[i].EndY + 1 + depth, 0, backColor);

		}
		
		gpu3dsDrawVertexes();
	}
	t3dsEndTiming(25);

}


//-------------------------------------------------------------------
// Convert tile to 8-bit.
//-------------------------------------------------------------------
uint8 S9xConvertTileTo8Bit (uint8 *pCache, uint32 TileAddr)
{
    //printf ("Tile Addr: %04x\n", TileAddr);
    uint8 *tp = &Memory.VRAM[TileAddr];
    uint32 *p = (uint32 *) pCache;
    uint32 non_zero = 0;
    uint8 line;

    switch (BG.BitShift)
    {
    case 8:
		for (line = 8; line != 0; line--, tp += 2)
		{
			uint32 p1 = 0;
			uint32 p2 = 0;
			uint8 pix;

			if ((pix = *(tp + 0)))
			{
				p1 |= odd_high[0][pix >> 4];
				p2 |= odd_low[0][pix & 0xf];
			}
			if ((pix = *(tp + 1)))
			{
				p1 |= even_high[0][pix >> 4];
				p2 |= even_low[0][pix & 0xf];
			}
			if ((pix = *(tp + 16)))
			{
				p1 |= odd_high[1][pix >> 4];
				p2 |= odd_low[1][pix & 0xf];
			}
			if ((pix = *(tp + 17)))
			{
				p1 |= even_high[1][pix >> 4];
				p2 |= even_low[1][pix & 0xf];
			}
			if ((pix = *(tp + 32)))
			{
				p1 |= odd_high[2][pix >> 4];
				p2 |= odd_low[2][pix & 0xf];
			}
			if ((pix = *(tp + 33)))
			{
				p1 |= even_high[2][pix >> 4];
				p2 |= even_low[2][pix & 0xf];
			}
			if ((pix = *(tp + 48)))
			{
				p1 |= odd_high[3][pix >> 4];
				p2 |= odd_low[3][pix & 0xf];
			}
			if ((pix = *(tp + 49)))
			{
				p1 |= even_high[3][pix >> 4];
				p2 |= even_low[3][pix & 0xf];
			}
			*p++ = p1;
			*p++ = p2;
			non_zero |= p1 | p2;
		}
		break;

    case 4:
		for (line = 8; line != 0; line--, tp += 2)
		{
			uint32 p1 = 0;
			uint32 p2 = 0;
			uint8 pix;
			if ((pix = *(tp + 0)))
			{
				p1 |= odd_high[0][pix >> 4];
				p2 |= odd_low[0][pix & 0xf];
			}
			if ((pix = *(tp + 1)))
			{
				p1 |= even_high[0][pix >> 4];
				p2 |= even_low[0][pix & 0xf];
			}
			if ((pix = *(tp + 16)))
			{
				p1 |= odd_high[1][pix >> 4];
				p2 |= odd_low[1][pix & 0xf];
			}
			if ((pix = *(tp + 17)))
			{
				p1 |= even_high[1][pix >> 4];
				p2 |= even_low[1][pix & 0xf];
			}
			*p++ = p1;
			*p++ = p2;
			non_zero |= p1 | p2;
		}
		break;

    case 2:
		for (line = 8; line != 0; line--, tp += 2)
		{
			uint32 p1 = 0;
			uint32 p2 = 0;
			uint8 pix;
			if ((pix = *(tp + 0)))
			{
				p1 |= odd_high[0][pix >> 4];
				p2 |= odd_low[0][pix & 0xf];
			}
			if ((pix = *(tp + 1)))
			{
				p1 |= even_high[0][pix >> 4];
				p2 |= even_low[0][pix & 0xf];
			}
			*p++ = p1;
			*p++ = p2;
			non_zero |= p1 | p2;
		}
		break;
    }
    return (non_zero ? TRUE : BLANK_TILE);
}



uint8 stencilFunc[128][4]
{
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:0 | Logic = OR   (idx: 0)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:0 | Logic = AND  (idx: 1)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:0 | Logic = XOR  (idx: 2)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:0 | Logic = XNOR (idx: 3)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:0 | Logic = OR   (idx: 4)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:0 | Logic = AND  (idx: 5)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:0 | Logic = XOR  (idx: 6)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:0 | Logic = XNOR (idx: 7)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:0 | Logic = OR   (idx: 8)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:0 | Logic = AND  (idx: 9)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:0 | Logic = XOR  (idx: 10)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:0 | Logic = XNOR (idx: 11)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:0 | Logic = OR   (idx: 12)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:0 | Logic = AND  (idx: 13)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:0 | Logic = XOR  (idx: 14)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:0 | Logic = XNOR (idx: 15)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:0 | Logic = OR   (idx: 16)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:0 | Logic = AND  (idx: 17)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:0 | Logic = XOR  (idx: 18)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:0 | Logic = XNOR (idx: 19)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:0 | Logic = OR   (idx: 20)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:0 | Logic = AND  (idx: 21)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:0 | Logic = XOR  (idx: 22)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:0 | Logic = XNOR (idx: 23)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:0 | Logic = OR   (idx: 24)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:0 | Logic = AND  (idx: 25)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:0 | Logic = XOR  (idx: 26)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:0 | Logic = XNOR (idx: 27)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:0 | Logic = OR   (idx: 28)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:0 | Logic = AND  (idx: 29)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:0 | Logic = XOR  (idx: 30)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:0 | Logic = XNOR (idx: 31)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:1 | Logic = OR   (idx: 32)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:1 | Logic = AND  (idx: 33)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:1 | Logic = XOR  (idx: 34)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:1 | Logic = XNOR (idx: 35)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:1 | Logic = OR   (idx: 36)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:1 | Logic = AND  (idx: 37)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:1 | Logic = XOR  (idx: 38)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:1 | Logic = XNOR (idx: 39)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:1 | Logic = OR   (idx: 40)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:1 | Logic = AND  (idx: 41)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:1 | Logic = XOR  (idx: 42)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:1 | Logic = XNOR (idx: 43)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:1 | Logic = OR   (idx: 44)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:1 | Logic = AND  (idx: 45)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:1 | Logic = XOR  (idx: 46)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:1 | Logic = XNOR (idx: 47)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:1 | Logic = OR   (idx: 48)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:1 | Logic = AND  (idx: 49)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:1 | Logic = XOR  (idx: 50)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:1 | Logic = XNOR (idx: 51)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:1 | Logic = OR   (idx: 52)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:1 | Logic = AND  (idx: 53)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:1 | Logic = XOR  (idx: 54)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:1 | Logic = XNOR (idx: 55)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:1 | Logic = OR   (idx: 56)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:1 | Logic = AND  (idx: 57)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:1 | Logic = XOR  (idx: 58)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:1 | Logic = XNOR (idx: 59)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:1 | Logic = OR   (idx: 60)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:1 | Logic = AND  (idx: 61)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:1 | Logic = XOR  (idx: 62)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:0 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:1 | Logic = XNOR (idx: 63)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:0 | Logic = OR   (idx: 64)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:0 | Logic = AND  (idx: 65)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:0 | Logic = XOR  (idx: 66)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:0 | Logic = XNOR (idx: 67)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:0 | Logic = OR   (idx: 68)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:0 | Logic = AND  (idx: 69)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:0 | Logic = XOR  (idx: 70)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:0 | Logic = XNOR (idx: 71)
    { GPU_EQUAL,    0x00, 0x01 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:0 | Logic = OR   (idx: 72)
    { GPU_EQUAL,    0x00, 0x01 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:0 | Logic = AND  (idx: 73)
    { GPU_EQUAL,    0x00, 0x01 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:0 | Logic = XOR  (idx: 74)
    { GPU_EQUAL,    0x00, 0x01 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:0 | Logic = XNOR (idx: 75)
    { GPU_EQUAL,    0x01, 0x01 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:0 | Logic = OR   (idx: 76)
    { GPU_EQUAL,    0x01, 0x01 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:0 | Logic = AND  (idx: 77)
    { GPU_EQUAL,    0x01, 0x01 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:0 | Logic = XOR  (idx: 78)
    { GPU_EQUAL,    0x01, 0x01 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:0 | Logic = XNOR (idx: 79)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:0 | Logic = OR   (idx: 80)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:0 | Logic = AND  (idx: 81)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:0 | Logic = XOR  (idx: 82)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:0 | Logic = XNOR (idx: 83)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:0 | Logic = OR   (idx: 84)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:0 | Logic = AND  (idx: 85)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:0 | Logic = XOR  (idx: 86)
    { GPU_ALWAYS,   0x00, 0x00 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:0 | Logic = XNOR (idx: 87)
    { GPU_EQUAL,    0x00, 0x01 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:0 | Logic = OR   (idx: 88)
    { GPU_EQUAL,    0x00, 0x01 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:0 | Logic = AND  (idx: 89)
    { GPU_EQUAL,    0x00, 0x01 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:0 | Logic = XOR  (idx: 90)
    { GPU_EQUAL,    0x00, 0x01 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:0 | Logic = XNOR (idx: 91)
    { GPU_EQUAL,    0x01, 0x01 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:0 | Logic = OR   (idx: 92)
    { GPU_EQUAL,    0x01, 0x01 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:0 | Logic = AND  (idx: 93)
    { GPU_EQUAL,    0x01, 0x01 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:0 | Logic = XOR  (idx: 94)
    { GPU_EQUAL,    0x01, 0x01 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:0 | Logic = XNOR (idx: 95)
    { GPU_EQUAL,    0x00, 0x02 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:1 | Logic = OR   (idx: 96)
    { GPU_EQUAL,    0x00, 0x02 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:1 | Logic = AND  (idx: 97)
    { GPU_EQUAL,    0x00, 0x02 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:1 | Logic = XOR  (idx: 98)
    { GPU_EQUAL,    0x00, 0x02 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:0 Ena:1 | Logic = XNOR (idx: 99)
    { GPU_EQUAL,    0x00, 0x02 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:1 | Logic = OR   (idx: 100)
    { GPU_EQUAL,    0x00, 0x02 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:1 | Logic = AND  (idx: 101)
    { GPU_EQUAL,    0x00, 0x02 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:1 | Logic = XOR  (idx: 102)
    { GPU_EQUAL,    0x00, 0x02 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:0 Ena:1 | Logic = XNOR (idx: 103)
    { GPU_EQUAL,    0x00, 0x03 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:1 | Logic = OR   (idx: 104)
    { GPU_NOTEQUAL, 0x03, 0x03 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:1 | Logic = AND  (idx: 105)
    { GPU_NOTEQUAL, 0x04, 0x04 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:1 | Logic = XOR  (idx: 106)
    { GPU_NOTEQUAL, 0x00, 0x04 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:0 Ena:1 | Logic = XNOR (idx: 107)
    { GPU_EQUAL,    0x01, 0x03 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:1 | Logic = OR   (idx: 108)
    { GPU_NOTEQUAL, 0x02, 0x03 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:1 | Logic = AND  (idx: 109)
    { GPU_NOTEQUAL, 0x00, 0x04 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:1 | Logic = XOR  (idx: 110)
    { GPU_NOTEQUAL, 0x04, 0x04 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:0 Ena:1 | Logic = XNOR (idx: 111)
    { GPU_EQUAL,    0x02, 0x02 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:1 | Logic = OR   (idx: 112)
    { GPU_EQUAL,    0x02, 0x02 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:1 | Logic = AND  (idx: 113)
    { GPU_EQUAL,    0x02, 0x02 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:1 | Logic = XOR  (idx: 114)
    { GPU_EQUAL,    0x02, 0x02 },    // 212e/f:1 | W1 Inv:0 Ena:0 | W2 Inv:1 Ena:1 | Logic = XNOR (idx: 115)
    { GPU_EQUAL,    0x02, 0x02 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:1 | Logic = OR   (idx: 116)
    { GPU_EQUAL,    0x02, 0x02 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:1 | Logic = AND  (idx: 117)
    { GPU_EQUAL,    0x02, 0x02 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:1 | Logic = XOR  (idx: 118)
    { GPU_EQUAL,    0x02, 0x02 },    // 212e/f:1 | W1 Inv:1 Ena:0 | W2 Inv:1 Ena:1 | Logic = XNOR (idx: 119)
    { GPU_EQUAL,    0x02, 0x03 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:1 | Logic = OR   (idx: 120)
    { GPU_NOTEQUAL, 0x01, 0x03 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:1 | Logic = AND  (idx: 121)
    { GPU_NOTEQUAL, 0x00, 0x04 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:1 | Logic = XOR  (idx: 122)
    { GPU_NOTEQUAL, 0x04, 0x04 },    // 212e/f:1 | W1 Inv:0 Ena:1 | W2 Inv:1 Ena:1 | Logic = XNOR (idx: 123)
    { GPU_EQUAL,    0x03, 0x03 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:1 | Logic = OR   (idx: 124)
    { GPU_NOTEQUAL, 0x00, 0x03 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:1 | Logic = AND  (idx: 125)
    { GPU_NOTEQUAL, 0x04, 0x04 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:1 | Logic = XOR  (idx: 126)
    { GPU_NOTEQUAL, 0x00, 0x04 },    // 212e/f:1 | W1 Inv:1 Ena:1 | W2 Inv:1 Ena:1 | Logic = XNOR (idx: 127)

};


// Based on the original enumeration:
//    GPU_NEVER    - 0
//    GPU_ALWAYS   - 1
//    GPU_EQUAL    - 2
//    GPU_NOTEQUAL - 3
//
GPU_TESTFUNC inverseFunction[] = { GPU_ALWAYS, GPU_NEVER, GPU_NOTEQUAL, GPU_EQUAL };
char *funcName[] = { "NVR", "ALW", "EQ ", "NEQ" };

//-----------------------------------------------------------
// Updates the screen using the 3D hardware.
//-----------------------------------------------------------
inline bool S9xComputeAndEnableStencilFunction(int layer, int subscreen)
{
	if (!IPPU.WindowingEnabled)
	{
		//printf ("  ES: 2130: %x\n", GFX.r2130);

		// For modes 5 and 6 (hi-res) mode, 
		// we will always blend the two screens regardless of
		// the color math because in a real TV, it's not the
		// SNES doing color math, but the TV blending the 
		// hi-res screen making it look like color math.
		//
		if (PPU.BGMode == 5 || PPU.BGMode == 6)
		{
			gpu3dsDisableStencilTest();
			return true;			
		}

		// Can we do this outside? 
		if (layer == 5 && subscreen == 0)
		{
			if ((GFX.r2130 & 0xc0) == 0x80 ||
				(GFX.r2130 & 0xc0) == 00)
			{
				gpu3dsEnableStencilTest(GPU_NEVER, 0, 0);
				return false;
			}
		}
		else if (layer == 5 && subscreen == 1)
		{
			if ((GFX.r2130 & 0x30) == 0x10 ||
				(GFX.r2130 & 0x30) == 0x30)
			{
				gpu3dsEnableStencilTest(GPU_NEVER, 0, 0);
				return false;
			}
		}
		gpu3dsDisableStencilTest();
		return true;
	}
	
	uint8 windowMaskEnableFlag = (layer == 5) ? 1 : ((Memory.FillRAM[0x212e + subscreen] >> layer) & 1);
	uint32 windowLogic = (uint32)Memory.FillRAM[0x212a] + ((uint32)Memory.FillRAM[0x212b] << 8); 
	windowLogic = (windowLogic >> (layer * 2)) & 0x3;

	uint32 windowEnableInv = (uint32)Memory.FillRAM[0x2123] + ((uint32)Memory.FillRAM[0x2124] << 8) + ((uint32)Memory.FillRAM[0x2125] << 16);
	windowEnableInv = (windowEnableInv >> (layer * 4)) & 0xF;

	int idx = windowMaskEnableFlag << 6 | windowEnableInv << 2 | windowLogic;
	 
	// debugging only
	/*if (layer == 5)
	{
	printf ("ST L%d S%d Y:%d-%d F=%s R=%x M=%x (%d)\n", layer, subscreen, GFX.StartY, GFX.EndY, 
		funcName[stencilFunc[idx][0]], stencilFunc[idx][1], stencilFunc[idx][2], idx);
	printf ("  W1E:%d W1I:%d W2E:%d W2I:%d WLog:%d\n", PPU.ClipWindow1Enable[layer], PPU.ClipWindow1Inside[layer],
	 	PPU.ClipWindow2Enable[layer], PPU.ClipWindow2Inside[layer], PPU.ClipWindowOverlapLogic[layer] );
	printf ("  212c-%02x %02x %02x %02x 2130-%02x\n", 
		Memory.FillRAM[0x212c], Memory.FillRAM[0x212d], Memory.FillRAM[0x212e], Memory.FillRAM[0x212f], 
		GFX.r2130);
	}*/
		 

	GPU_TESTFUNC func = (GPU_TESTFUNC)stencilFunc[idx][0];

	// If we are doing color math, then we must inspect 2130 and
	// modify the func accordingly
	//
	if (layer == 5)
	{
		if (subscreen == 1)
		{
			switch (GFX.r2130 & 0x30)
			{
				case 0x00:  // never prevent color math 
					func = GPU_ALWAYS;
					break;
				case 0x10:  // prevent color math outside window
					func = inverseFunction[func];
					break;
				case 0x20:  // prevent color math inside window (no change to the original mask)
					break;
				case 0x30: // always prevent color math
					func = GPU_NEVER;
					break;
			}
			/*printf ("  212c-%02x %02x %02x %02x 2130-%02x - final func: %s\n", 
				Memory.FillRAM[0x212c], Memory.FillRAM[0x212d], Memory.FillRAM[0x212e], Memory.FillRAM[0x212f], 
				GFX.r2130, funcName[func]);*/
		}
		else 
		{
			// clip to black
			//
			switch (GFX.r2130 & 0xc0)
			{
				case 0x00:  // never clear to black
					func = GPU_NEVER;
					break;
				case 0x40:  // clear to black outside window (no change to the original mask)
					break;
				case 0x80:  // clear to black inside window 
					func = inverseFunction[func];
					break;
				case 0xc0: // always clear to black
					func = GPU_ALWAYS;
					break;
			}
			/*printf ("  212c-%02x %02x %02x %02x 2130-%02x - final func: %s\n", 
				Memory.FillRAM[0x212c], Memory.FillRAM[0x212d], Memory.FillRAM[0x212e], Memory.FillRAM[0x212f], 
				GFX.r2130, funcName[func]);*/
		}
	}

	if (func == GPU_ALWAYS)
		gpu3dsDisableStencilTest();
	else
		gpu3dsEnableStencilTest(func, stencilFunc[idx][1] << 5, stencilFunc[idx][2] << 5);

	return true;
}


int curBG = 0;



//-------------------------------------------------------------------
// Draw a full tile 8xh tile using 3D hardware
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawBGFullTileHardwareInline (
    int tileSize, int tileShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
	int prio, int depth0, int depth1, 
	int32 snesTile, int32 screenX, int32 screenY, 
	int32 startLine, int32 height)
{
	int texturePos = 0;

	// Prepare tile for rendering
	//
    uint8 *pCache;
    uint16 *pCache16;

    uint32 TileAddr = BG.TileAddress + ((snesTile & 0x3ff) << tileShift);

	// Bug fix: overflow in Dragon Ball Budoten 3 
	// (this was accidentally removed while optimizing for this 3DS port)
	TileAddr &= 0xff00ffff;		// hope the compiler generates a BIC instruction.

    uint32 TileNumber;
    pCache = &BG.Buffer[(TileNumber = (TileAddr >> tileShift)) << 6];

	//if (screenOffset == 0)
	//	printf ("  tile: %d\n", TileNumber);

	//if ((snesTile & 0x3ff) == 0 && curBG == 2)
	//	printf ("* %d,%d BG%d TileAddr=%4x Buf=%d\n", screenOffset & 0xff, screenOffset >> 8, curBG, TileAddr, BG.Buffered[TileNumber]);

    if (!BG.Buffered [TileNumber])
    {
	    BG.Buffered[TileNumber] = S9xConvertTileTo8Bit (pCache, TileAddr);
        if (BG.Buffered [TileNumber] == BLANK_TILE)
            return;

        GFX.VRAMPaletteFrame[TileAddr / 8][0] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][1] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][2] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][3] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][4] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][5] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][6] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][7] = 0;
    }

	//if (screenOffset == 0)
	//	printf ("  buffered: %d\n", BG.Buffered [TileNumber]);

    if (BG.Buffered [TileNumber] == BLANK_TILE)
	    return;

	/*if ((snesTile & 0x3ff) == 0 && curBG == 2)
	{
		for (int i = 0; i < 64; i++)
		{
			printf ("%2x", pCache[i]);
			if (i % 8 == 7)
				printf ("\n");
		}
	}*/

    uint32 l;
    uint8 pal;
    if (directColourMode)
    {
        if (IPPU.DirectColourMapsNeedRebuild)
            S9xBuildDirectColourMaps ();
        pal = (snesTile >> 10) & paletteMask;
		texturePos = cache3dsGetTexturePositionFast(TileAddr, pal);

        if (GFX.VRAMPaletteFrame[TileAddr / 8][pal] != GFX.PaletteFrame[pal])
        {
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
            GFX.VRAMPaletteFrame[TileAddr / 8][pal] = GFX.PaletteFrame[pal];

			uint16 *screenColors = DirectColourMaps [pal];
			cache3dsCacheSnesTileToTexturePosition(pCache, screenColors, texturePos);
        }
    }
    else
    {
        pal = (snesTile >> 10) & paletteMask;
		texturePos = cache3dsGetTexturePositionFast(TileAddr, pal);
		//printf ("%d\n", texturePos);

		uint32 *paletteFrame = GFX.PaletteFrame;
		if (paletteShift == 2)
			paletteFrame = GFX.PaletteFrame4BG[startPalette / 32];
		else if (paletteShift == 0)
		{
			paletteFrame = GFX.PaletteFrame256;
			pal = 0;
		}

		//if (screenOffset == 0)
		//	printf ("  %d %d %d %d\n", startPalette, pal, paletteFrame[pal + startPalette / 16], GFX.VRAMPaletteFrame[TileAddr / 8][pal]);

		if (GFX.VRAMPaletteFrame[TileAddr / 8][pal] != paletteFrame[pal])
		{
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
			GFX.VRAMPaletteFrame[TileAddr / 8][pal] = paletteFrame[pal];

			//if (screenOffset == 0)
			//	printf ("cache %d\n", texturePos);
			uint16 *screenColors = &IPPU.ScreenColors [(pal << paletteShift) + startPalette];
			cache3dsCacheSnesTileToTexturePosition(pCache, screenColors, texturePos);
		}
    }
	

	// Render tile
	//
	int x0 = screenX;
	int y0 = screenY;
	if (prio == 0)
		y0 += depth0;
	else
		y0 += depth1;
	int x1 = x0 + 8;
	int y1 = y0 + height;

	int tx0 = 0;
	int ty0 = startLine >> 3;
	int tx1 = 8;
	int ty1 = ty0 + height;

	gpu3dsAddTileVertexes(
		x0, y0, x1, y1,
		tx0, ty0,
		tx1, ty1, (snesTile & (H_FLIP | V_FLIP)) + texturePos);
		
}


//-------------------------------------------------------------------
// Draw a hi-res full tile 8xh tile using 3D hardware
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawHiresBGFullTileHardwareInline (
    int tileSize, int tileShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
	int prio, int depth0, int depth1,
	int32 snesTile, int32 screenX, int32 screenY,
	int32 startLine, int32 height)
{
	int texturePos = 0;

	// Prepare tile for rendering
	//
    uint8 *pCache;
    uint16 *pCache16;

    uint32 TileAddr = BG.TileAddress + ((snesTile & 0x3ff) << tileShift);

	// Bug fix: overflow in Dragon Ball Budoten 3 
	// (this was accidentally removed while optimizing for this 3DS port)
	TileAddr &= 0xff00ffff;		// hope the compiler generates a BIC instruction.

    uint32 TileNumber;
    pCache = &BG.Buffer[(TileNumber = (TileAddr >> tileShift)) << 6];

	//if (screenOffset == 0)
	//	printf ("  tile: %d\n", TileNumber);

	//if ((snesTile & 0x3ff) == 0 && curBG == 2)
	//	printf ("* %d,%d BG%d TileAddr=%4x Buf=%d\n", screenOffset & 0xff, screenOffset >> 8, curBG, TileAddr, BG.Buffered[TileNumber]);

    if (!BG.Buffered [TileNumber])
    {
	    BG.Buffered[TileNumber] = S9xConvertTileTo8Bit (pCache, TileAddr);
        if (BG.Buffered [TileNumber] == BLANK_TILE)
            return;

        GFX.VRAMPaletteFrame[TileAddr / 8][0] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][1] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][2] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][3] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][4] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][5] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][6] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][7] = 0;
    }

	//if (screenOffset == 0)
	//	printf ("  buffered: %d\n", BG.Buffered [TileNumber]);

    if (BG.Buffered [TileNumber] == BLANK_TILE)
	    return;

	/*if ((snesTile & 0x3ff) == 0 && curBG == 2)
	{
		for (int i = 0; i < 64; i++)
		{
			printf ("%2x", pCache[i]);
			if (i % 8 == 7)
				printf ("\n");
		}
	}*/

    uint32 l;
    uint8 pal;
    if (directColourMode)
    {
        if (IPPU.DirectColourMapsNeedRebuild)
            S9xBuildDirectColourMaps ();
        pal = (snesTile >> 10) & paletteMask;
		GFX.ScreenColors = DirectColourMaps [pal];
		texturePos = cache3dsGetTexturePositionFast(TileAddr, pal);

        if (GFX.VRAMPaletteFrame[TileAddr / 8][pal] != GFX.PaletteFrame[pal])
        {
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
            GFX.VRAMPaletteFrame[TileAddr / 8][pal] = GFX.PaletteFrame[pal];

			cache3dsCacheSnesTileToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        }
    }
    else
    {
        pal = (snesTile >> 10) & paletteMask;
		texturePos = cache3dsGetTexturePositionFast(TileAddr, pal);
		//printf ("%d\n", texturePos);

		uint32 *paletteFrame = GFX.PaletteFrame;
		if (paletteShift == 2)
			paletteFrame = GFX.PaletteFrame4BG[startPalette / 32];
		else if (paletteShift == 0)
		{
			paletteFrame = GFX.PaletteFrame256;
			pal = 0;
		}

		//if (screenOffset == 0)
		//	printf ("  %d %d %d %d\n", startPalette, pal, paletteFrame[pal + startPalette / 16], GFX.VRAMPaletteFrame[TileAddr / 8][pal]);

		if (GFX.VRAMPaletteFrame[TileAddr / 8][pal] != paletteFrame[pal])
		{
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
			GFX.VRAMPaletteFrame[TileAddr / 8][pal] = paletteFrame[pal];

			//if (screenOffset == 0)
			//	printf ("cache %d\n", texturePos);
			uint16 *screenColors = &IPPU.ScreenColors [(pal << paletteShift) + startPalette];
			cache3dsCacheSnesTileToTexturePosition(pCache, screenColors, texturePos);
		}
    }
	
	int x0 = screenX >> 1;
	int y0 = screenY;
	if (prio == 0)
		y0 += depth0;
	else
		y0 += depth1;
	
	int x1 = x0 + 4;
	int y1 = y0 + height;

	int tx0 = 0;
	int ty0 = startLine >> 3;
	int tx1 = 7;
	int ty1 = ty0 + height;

	if (IPPU.Interlace)
		ty1 = ty1 + height - 1;

	gpu3dsAddTileVertexes(
		x0, y0, x1, y1,
		tx0, ty0,
		tx1, ty1, (snesTile & (H_FLIP | V_FLIP)) + texturePos);
	// Render tile
	//
	/*
	if (!IPPU.Interlace)
	{
		int x0 = screenX >> 1;
		int y0 = screenY;
		if (prio == 0)
			y0 += depth0;
		else
			y0 += depth1;
		
		int x1 = x0 + 4;
		int y1 = y0 + height;

		int tx0 = IPPU.HiresFlip;
		int ty0 = startLine >> 3;
		int tx1 = 7 + IPPU.HiresFlip;
		int ty1 = ty0 + height;
		gpu3dsAddTileVertexes(
			x0, y0, x1, y1,
			tx0, ty0,
			tx1, ty1, (snesTile & (H_FLIP | V_FLIP)) + texturePos);
	}
	else
	{
		int x0 = screenX >> 1;
		int y0 = screenY;
		if (prio == 0)
			y0 += depth0;
		else
			y0 += depth1;
		
		int x1 = x0 + 4;
		int y1 = y0 + height;

		int tx0 = IPPU.HiresFlip;
		int ty0 = startLine >> 3 + IPPU.HiresFlip;
		int tx1 = 7 + IPPU.HiresFlip;
		int ty1 = ty0 + height * 2 - 1 + IPPU.HiresFlip;
		gpu3dsAddTileVertexes(
			x0, y0, x1, y1,
			tx0, ty0,
			tx1, ty1, (snesTile & (H_FLIP | V_FLIP)) + texturePos);
		
	}
	*/
}



//-------------------------------------------------------------------
// Draw offset-per-tile background.
//-------------------------------------------------------------------

inline void __attribute__((always_inline)) S9xDrawOffsetBackgroundHardwarePriority0Inline (
    int tileSize, int tileShift, int bitShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
	uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    uint32 Tile;
    uint16 *SC0;
    uint16 *SC1;
    uint16 *SC2;
    uint16 *SC3;
    uint16 *BPS0;
    uint16 *BPS1;
    uint16 *BPS2;
    uint16 *BPS3;
    //uint32 Width;
    int VOffsetOffset = BGMode == 4 ? 0 : 32;

	S9xComputeAndEnableStencilFunction(bg, sub);

	// Note: We draw subscreens first, then the main screen.
	// So if the subscreen has already been drawn, and we are drawing the main screen,
	// we simply just redraw the same vertices that we have saved.
	//
	if (layerDrawn[bg])
	{
		gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
		gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();
		gpu3dsEnableAlphaTestNotEqualsZero();
		gpu3dsEnableDepthTest();
		//printf ("Redraw: %d (%s)\n", bg, sub ? "sub" : "main");
		gpu3dsDrawVertexes(true, bg);	// redraw saved vertices
		return;
	}

    GFX.PixSize = 1;

    BG.TileSize = tileSize;
    BG.BitShift = bitShift;
    BG.TileShift = tileShift;
    BG.TileAddress = PPU.BG[bg].NameBase << 1;
    BG.NameSelect = 0;
    BG.Buffer = IPPU.TileCache [Depths [BGMode][bg]];
    BG.Buffered = IPPU.TileCached [Depths [BGMode][bg]];
    BG.PaletteShift = paletteShift;
    BG.PaletteMask = paletteMask;
    BG.DirectColourMode = directColourMode;
	//BG.Depth = depth;
	//printf ("mode:%d depth: %d\n", PPU.BGMode, BG.Depth);
	
	//BG.DrawTileCount[bg] = 0;
	
    BPS0 = (uint16 *) &Memory.VRAM[PPU.BG[2].SCBase << 1];
	
    if (PPU.BG[2].SCSize & 1)
		BPS1 = BPS0 + 1024;
    else
		BPS1 = BPS0;
	
    if (PPU.BG[2].SCSize & 2)
		BPS2 = BPS1 + 1024;
    else
		BPS2 = BPS0;
	
    if (PPU.BG[2].SCSize & 1)
		BPS3 = BPS2 + 1024;
    else
		BPS3 = BPS2;
    
    SC0 = (uint16 *) &Memory.VRAM[PPU.BG[bg].SCBase << 1];
	
    if (PPU.BG[bg].SCSize & 1)
		SC1 = SC0 + 1024;
    else
		SC1 = SC0;
	
	if(((uint8*)SC1-Memory.VRAM)>=0x10000)
		SC1-=0x08000;


    if (PPU.BG[bg].SCSize & 2)
		SC2 = SC1 + 1024;
    else
		SC2 = SC0;

	if(((uint8*)SC2-Memory.VRAM)>=0x10000)
		SC2-=0x08000;


    if (PPU.BG[bg].SCSize & 1)
		SC3 = SC2 + 1024;
    else
		SC3 = SC2;
	
	if(((uint8*)SC3-Memory.VRAM)>=0x10000)
		SC3-=0x08000;


    
    int OffsetMask;
    int OffsetShift;
    int OffsetEnableMask = 1 << (bg + 13);
	
    if (tileSize == 16)
    {
		OffsetMask = 0x3ff;
		OffsetShift = 4;
    }
    else
    {
		OffsetMask = 0x1ff;
		OffsetShift = 3;
    }

	// Optimized version of the offset per tile renderer
	//
    for (uint32 OY = GFX.StartY; OY <= GFX.EndY; )
    {
		// Do a check to find out how many scanlines
		// that the BGnVOFS, BGnHOFS, BG2VOFS, BG2HOS
		// remains constant
		//
		int TotalLines = 1;
		for (; TotalLines < PPU.ScreenHeight - 1; TotalLines++)
		{
			int y = OY + TotalLines - 1;
			if (y >= GFX.EndY)
				break;
			if (!(LineData [y].BG[bg].VOffset == LineData [y + 1].BG[bg].VOffset &&
				LineData [y].BG[bg].HOffset == LineData [y + 1].BG[bg].HOffset &&
				LineData [y].BG[2].VOffset == LineData [y + 1].BG[2].VOffset && 
				LineData [y].BG[2].HOffset == LineData [y + 1].BG[2].HOffset))
				break;
		}
		
		// For those lines, draw the tiles column by column 
		// (from the left to the right of the screen)
		//
		for (int Left = 0; Left <= 256; Left += 8)	// Bug fix: It should be Left <= 256 instead of Left < 256
		{
			for (int Y = OY; Y < OY + TotalLines; )
			{
				uint32 VOff = LineData [Y].BG[2].VOffset - 1;
		//		uint32 VOff = LineData [Y].BG[2].VOffset;
				uint32 HOff = LineData [Y].BG[2].HOffset;

				int VirtAlign;
				int ScreenLine = VOff >> 3;
				int t1;
				int t2;
				uint16 *s0;
				uint16 *s1;
				uint16 *s2;
				
				if (ScreenLine & 0x20)
					s1 = BPS2, s2 = BPS3;
				else
					s1 = BPS0, s2 = BPS1;
				
				
				s1 += (ScreenLine & 0x1f) << 5;
				s2 += (ScreenLine & 0x1f) << 5;
				
				if(BGMode != 4)
				{
					if((ScreenLine & 0x1f) == 0x1f)
					{
						if(ScreenLine & 0x20)
							VOffsetOffset = BPS0 - BPS2 - 0x1f*32;
						else
							VOffsetOffset = BPS2 - BPS0 - 0x1f*32;
					}
					else
					{
						VOffsetOffset = 32;
					}
				}
				
				//for (int clip = 0; clip < clipcount; clip++)
			
				uint32 VOffset;
				uint32 HOffset;
				//added:
				uint32 LineHOffset=LineData [Y].BG[bg].HOffset;
				
				uint32 Offset;
				uint32 HPos;
				uint32 Quot;
				uint16 *t;
				uint32 Quot2;
				uint32 VCellOffset;
				uint32 HCellOffset;
				uint16 *b1;
				uint16 *b2;
				uint32 Lines;
				
				//int sX = Left;
				bool8 left_hand_edge = (Left == 0);
				//Width = Right - Left;
				
				//while (Left < Right) 
				{
					if (left_hand_edge)
					{
						// The SNES offset-per-tile background mode has a
						// hardware limitation that the offsets cannot be set
						// for the tile at the left-hand edge of the screen.
						VOffset = LineData [Y].BG[bg].VOffset;

						//MKendora; use temp var to reduce memory accesses
						//HOffset = LineData [Y].BG[bg].HOffset;

						HOffset = LineHOffset;
						//End MK

						left_hand_edge = FALSE;
					}
					else
					{
						// All subsequent offset tile data is shifted left by one,
						// hence the - 1 below.

						Quot2 = ((HOff + Left - 1) & OffsetMask) >> 3;
						
						if (Quot2 > 31)
							s0 = s2 + (Quot2 & 0x1f);
						else
							s0 = s1 + Quot2;
						
						HCellOffset = READ_2BYTES (s0);
						
						if (BGMode == 4)
						{
							VOffset = LineData [Y].BG[bg].VOffset;
							
							//MKendora another mem access hack
							//HOffset = LineData [Y].BG[bg].HOffset;
							HOffset=LineHOffset;
							//end MK

							if ((HCellOffset & OffsetEnableMask))
							{
								if (HCellOffset & 0x8000)
									VOffset = HCellOffset + 1;
								else
									HOffset = HCellOffset;
							}
						}
						else
						{
							VCellOffset = READ_2BYTES (s0 + VOffsetOffset);
							if ((VCellOffset & OffsetEnableMask))
								VOffset = VCellOffset + 1;
							else
								VOffset = LineData [Y].BG[bg].VOffset;

							//MKendora Strike Gunner fix
							if ((HCellOffset & OffsetEnableMask))
							{
								//HOffset= HCellOffset;
								
								HOffset = (HCellOffset & ~7)|(LineHOffset&7);
								//HOffset |= LineData [Y].BG[bg].HOffset&7;
							}
							else
								HOffset=LineHOffset;
								//HOffset = LineData [Y].BG[bg].HOffset - 
								//Settings.StrikeGunnerOffsetHack;
							//HOffset &= (~7);
							//end MK
						}
					}
					VirtAlign = ((Y + VOffset) & 7) << 3;
					Lines = 8 - (VirtAlign >> 3);
					//printf ("    L=%d\n", Lines);
					if (Y + Lines >= OY + TotalLines)
						Lines = OY + TotalLines - Y;
					ScreenLine = (VOffset + Y) >> OffsetShift;
					
					if (((VOffset + Y) & 15) > 7)
					{
						t1 = 16;
						t2 = 0;
					}
					else
					{
						t1 = 0;
						t2 = 16;
					}
					
					if (ScreenLine & 0x20)
						b1 = SC2, b2 = SC3;
					else
						b1 = SC0, b2 = SC1;
					
					b1 += (ScreenLine & 0x1f) << 5;
					b2 += (ScreenLine & 0x1f) << 5;
					
					HPos = (HOffset + Left) & OffsetMask;
					
					Quot = HPos >> 3;
					
					if (tileSize == 8)
					{
						if (Quot > 31)
							t = b2 + (Quot & 0x1f);
						else
							t = b1 + Quot;
					}
					else
					{
						if (Quot > 63)
							t = b2 + ((Quot >> 1) & 0x1f);
						else
							t = b1 + (Quot >> 1);
					}
					
					Offset = HPos & 7;
					
					int sX = Left - Offset;

					// Don't display anything beyond the right edge.
					if (sX >= 256)
						break;

					Tile = READ_2BYTES(t);

					int tpriority = (Tile & 0x2000) >> 13;
					
					if (tileSize == 8)
					{
						S9xDrawBGFullTileHardwareInline (
							tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
							tpriority, depth0, depth1,
							Tile, sX, Y, 
							VirtAlign, Lines);
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							S9xDrawBGFullTileHardwareInline (
								tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
								tpriority, depth0, depth1,
								Tile + t1 + (Quot & 1), sX, Y, 
								VirtAlign, Lines);
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									S9xDrawBGFullTileHardwareInline (
										tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
										tpriority, depth0, depth1,
										Tile + t2 + 1 - (Quot & 1), sX, Y, 
										VirtAlign, Lines);
								}
								else
								{
									S9xDrawBGFullTileHardwareInline (
										tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
										tpriority, depth0, depth1,
										Tile + t1 + 1 - (Quot & 1), sX, Y, 
										VirtAlign, Lines);
								}
							}
							else
							{
								S9xDrawBGFullTileHardwareInline (
									tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
									tpriority, depth0, depth1,
									Tile + t2 + (Quot & 1), sX, Y, 
									VirtAlign, Lines);
							}
					}
				}

				// Proceed to the tile below, in the same column.
				//
				Y += Lines;
			}
		}
		OY += TotalLines;
    }

	//printf ("BG OPT %d (Mode = %d)\n", bg, BGMode);
	//gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
	gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsEnableDepthTest();
	//printf ("Draw  : %d (%s)\n", bg, sub ? "sub" : "main");
	gpu3dsDrawVertexes(false, bg);
	layerDrawn[bg] = true;
}


//-------------------------------------------------------------------
// 4-color offset-per tile BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color_8x8(
            BGMode, bg, sub, depth0, depth1);
    }
    else
    {
        S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color_16x16(
            BGMode, bg, sub, depth0, depth1);
    }
}

//-------------------------------------------------------------------
// 16-color offset-per tile BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        8,              // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        16,             // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color_8x8(
            BGMode, bg, sub, depth0, depth1);
    }
    else
    {
        S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color_16x16(
            BGMode, bg, sub, depth0, depth1);
    }
}


//-------------------------------------------------------------------
// 256-color offset-per tile BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawOffsetBackgroundHardwarePriority0Inline_256NormalColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        8,              // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_256NormalColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        16,             // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_256DirectColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        8,              // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_256DirectColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        16,             // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_256Color
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        if (!(GFX.r2130 & 1))
            S9xDrawOffsetBackgroundHardwarePriority0Inline_256NormalColor_8x8(
                BGMode, bg, sub, depth0, depth1);
        else
            S9xDrawOffsetBackgroundHardwarePriority0Inline_256DirectColor_8x8(
                BGMode, bg, sub, depth0, depth1);
    }
    else
    {
        if (!(GFX.r2130 & 1))
            S9xDrawOffsetBackgroundHardwarePriority0Inline_256NormalColor_16x16(
                BGMode, bg, sub, depth0, depth1);
        else
            S9xDrawOffsetBackgroundHardwarePriority0Inline_256DirectColor_16x16(
                BGMode, bg, sub, depth0, depth1);
    }
}



//-------------------------------------------------------------------
// Draw non-offset-per-tile backgrounds
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawBackgroundHardwarePriority0Inline (
    int tileSize, int tileShift, int bitShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
    uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    GFX.PixSize = 1;

	S9xComputeAndEnableStencilFunction(bg, sub);

	//printf ("BG%d Y=%d-%d W1:%d-%d W2:%d-%d\n", bg, GFX.StartY, GFX.EndY, PPU.Window1Left, PPU.Window1Right, PPU.Window2Left, PPU.Window2Right);

	// Note: We draw subscreens first, then the main screen.
	// So if the subscreen has already been drawn, and we are drawing the main screen,
	// we simply just redraw the same vertices that we have saved.
	//
	if (layerDrawn[bg])
	{
		gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
		gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();
		gpu3dsEnableAlphaTestNotEqualsZero();
		gpu3dsEnableDepthTest();
		//printf ("Redraw: %d (%s)\n", bg, sub ? "sub" : "main");
		gpu3dsDrawVertexes(true, bg);	// redraw saved vertices
		return;
	}

    BG.TileSize = tileSize;
    BG.BitShift = bitShift;
    BG.TileShift = tileShift;
    BG.TileAddress = PPU.BG[bg].NameBase << 1;
    BG.NameSelect = 0;
    BG.Buffer = IPPU.TileCache [Depths [BGMode][bg]];
    BG.Buffered = IPPU.TileCached [Depths [BGMode][bg]];
    BG.PaletteShift = paletteShift;
    BG.PaletteMask = paletteMask;
    BG.DirectColourMode = directColourMode;
	//BG.Depth = depth;

	//BG.DrawTileCount[bg] = 0;

	curBG = bg;


    uint32 Tile;
    uint16 *SC0;
    uint16 *SC1;
    uint16 *SC2;
    uint16 *SC3;
    uint32 Width;

    if (BGMode == 0)
		BG.StartPalette = startPalette;
    else BG.StartPalette = 0;

    SC0 = (uint16 *) &Memory.VRAM[PPU.BG[bg].SCBase << 1];

	
    if (PPU.BG[bg].SCSize & 1)
		SC1 = SC0 + 1024;
    else
		SC1 = SC0;

	if(SC1>=(unsigned short*)(Memory.VRAM+0x10000))
		SC1=(uint16*)&Memory.VRAM[((uint8*)SC1-&Memory.VRAM[0])%0x10000];

    if (PPU.BG[bg].SCSize & 2)
		SC2 = SC1 + 1024;
    else
		SC2 = SC0;

	if(((uint8*)SC2-Memory.VRAM)>=0x10000)
		SC2-=0x08000;

    if (PPU.BG[bg].SCSize & 1)
		SC3 = SC2 + 1024;
    else
		SC3 = SC2;

	if(((uint8*)SC3-Memory.VRAM)>=0x10000)
		SC3-=0x08000;

    int Lines;
    int OffsetMask;
    int OffsetShift;

    if (tileSize == 16)
    {
		OffsetMask = 0x3ff;
		OffsetShift = 4;
    }
    else
    {
		OffsetMask = 0x1ff;
		OffsetShift = 3;
    }

    for (uint32 Y = GFX.StartY; Y <= GFX.EndY; Y += Lines)
    {
		uint32 VOffset = LineData [Y].BG[bg].VOffset;
		uint32 HOffset = LineData [Y].BG[bg].HOffset;

		int VirtAlign = (Y + VOffset) & 7;

		for (Lines = 1; Lines < 8 - VirtAlign; Lines++)
			if ((VOffset != LineData [Y + Lines].BG[bg].VOffset) ||
				(HOffset != LineData [Y + Lines].BG[bg].HOffset))
				break;

		if (Y + Lines > GFX.EndY)
			Lines = GFX.EndY + 1 - Y;

		//if (GFX.EndY - GFX.StartY < 10)
		//printf ("bg:%d Y/L:%3d/%3d OFS:%d,%d\n", bg, Y, Lines, HOffset, VOffset);

		VirtAlign <<= 3;

		uint32 ScreenLine = (VOffset + Y) >> OffsetShift;
		uint32 t1;
		uint32 t2;
		if (((VOffset + Y) & 15) > 7)
		{
			t1 = 16;
			t2 = 0;
		}
		else
		{
			t1 = 0;
			t2 = 16;
		}
		uint16 *b1;
		uint16 *b2;

		if (ScreenLine & 0x20)
			b1 = SC2, b2 = SC3;
		else
			b1 = SC0, b2 = SC1;

		b1 += (ScreenLine & 0x1f) << 5;
		b2 += (ScreenLine & 0x1f) << 5;

		//int clipcount = GFX.pCurrentClip->Count [bg];
		//if (!clipcount)
		//	clipcount = 1;
		//for (int clip = 0; clip < clipcount; clip++)
		{
			uint32 Left;
			uint32 Right;

			//if (!GFX.pCurrentClip->Count [bg])
			{
				Left = 0;
				Right = 256;
			}


			//uint32 s = Left * GFX.PixSize + Y * GFX.PPL;
			uint32 s = Left * GFX.PixSize + Y * 256;	
			//printf ("s = %d, Lines = %d\n", s, Lines);
			uint32 HPos = (HOffset + Left) & OffsetMask;

			uint32 Quot = HPos >> 3;
			uint32 Count = 0;

			uint16 *t;
			if (tileSize == 8)
			{
				if (Quot > 31)
					t = b2 + (Quot & 0x1f);
				else
					t = b1 + Quot;
			}
			else
			{
				if (Quot > 63)
					t = b2 + ((Quot >> 1) & 0x1f);
				else
					t = b1 + (Quot >> 1);
			}

			// screen coordinates of the tile.
			int sX = 0 - (HPos & 7);
			int sY = Y;


			int tilesToDraw = 32;
			if (sX != 0)
				tilesToDraw++;

			// Middle, unclipped tiles
			//Count = Width - Count;
			//int Middle = Count >> 3;
			//Count &= 7;

			//for (int C = Middle; C > 0; s += 8 * GFX.PixSize, Quot++, C--)
			for (int tno = 0; tno <= tilesToDraw; tno++, sX += 8, Quot++)
			{
				Tile = READ_2BYTES(t);

				int tpriority = (Tile & 0x2000) >> 13;
				//if (tpriority == priority)
				{
					if (tileSize != 8)
					{
						if (Tile & H_FLIP)
						{
							// Horizontal flip, but what about vertical flip ?
							if (Tile & V_FLIP)
							{
								// Both horzontal & vertical flip
								//if (tpriority == 0)
                                    S9xDrawBGFullTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
										tpriority, depth0, depth1,
                                        Tile + t2 + 1 - (Quot & 1), sX, sY, VirtAlign, Lines);
								//else
								//	DrawFullTileLater (Tile + t2 + 1 - (Quot & 1), sX, sY, VirtAlign, Lines);
							}
							else
							{
								// Horizontal flip only
								//if (tpriority == 0)
                                    S9xDrawBGFullTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
										tpriority, depth0, depth1,
                                        Tile + t1 + 1 - (Quot & 1), sX, sY, VirtAlign, Lines);
								//else
								//	DrawFullTileLater (Tile + t1 + 1 - (Quot & 1), sX, sY, VirtAlign, Lines);
							}
						}
						else
						{
							// No horizontal flip, but is there a vertical flip ?
							if (Tile & V_FLIP)
							{
								// Vertical flip only
								//if (tpriority == 0)
                                    S9xDrawBGFullTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
										tpriority, depth0, depth1,
                                        Tile + t2 + (Quot & 1), sX, sY, VirtAlign, Lines);
								//else
								//	DrawFullTileLater (Tile + t2 + (Quot & 1), sX, sY, VirtAlign, Lines);
							}
							else
							{
								// Normal unflipped
								//if (tpriority == 0)
                                    S9xDrawBGFullTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
										tpriority, depth0, depth1,
                                        Tile + t1 + (Quot & 1), sX, sY, VirtAlign, Lines);
								//else
								//	DrawFullTileLater (Tile + t1 + (Quot & 1), sX, sY, VirtAlign, Lines);
							}
						}
					}
					else
					{
						//if (tpriority == 0)
							S9xDrawBGFullTileHardwareInline (
                                tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
								tpriority, depth0, depth1,
                                Tile, sX, sY, VirtAlign, Lines);
						//else
						//	DrawFullTileLater (Tile, sX, sY, VirtAlign, Lines);
					}
				}

				if (tileSize == 8)
				{
					t++;
					if (Quot == 31)
						t = b2;
					else
						if (Quot == 63)
							t = b1;
				}
				else
				{
					t += Quot & 1;
					if (Quot == 63)
						t = b2;
					else
						if (Quot == 127)
							t = b1;
				}
			}

		}
    }

	//printf ("BG %d P0\n", bg);
	//gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
	gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsEnableDepthTest();
	//printf ("Draw  : %d (%s)\n", bg, sub ? "sub" : "main");
	gpu3dsDrawVertexes(false, bg);
	layerDrawn[bg] = true;
}


//-------------------------------------------------------------------
// 4-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority0Inline_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawBackgroundHardwarePriority0Inline_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}


void S9xDrawBackgroundHardwarePriority0Inline_4Color
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    if (BGMode != 0)
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawBackgroundHardwarePriority0Inline_4Color_8x8(
                BGMode, bg, sub, depth0, depth1);
        else
            S9xDrawBackgroundHardwarePriority0Inline_4Color_16x16(
                BGMode, bg, sub, depth0, depth1);
    }
    else
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_8x8(
                BGMode, bg, sub, depth0, depth1);
        else
            S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_16x16(
                BGMode, bg, sub, depth0, depth1);
    }
}



//-------------------------------------------------------------------
// 16-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority0Inline_16Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawBackgroundHardwarePriority0Inline_16Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawBackgroundHardwarePriority0Inline_16Color
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawBackgroundHardwarePriority0Inline_16Color_8x8(
            BGMode, bg, sub, depth0, depth1);
    }
    else
    {
        S9xDrawBackgroundHardwarePriority0Inline_16Color_16x16(
            BGMode, bg, sub, depth0, depth1);
    }
}



//-------------------------------------------------------------------
// 256-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawBackgroundHardwarePriority0Inline_256Color
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        if (!(GFX.r2130 & 1))
            S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_8x8(
                BGMode, bg, sub, depth0, depth1);
        else
            S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_8x8(
                BGMode, bg, sub, depth0, depth1);
    }
    else
    {
        if (!(GFX.r2130 & 1))
            S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_16x16(
                BGMode, bg, sub, depth0, depth1);
        else
            S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_16x16(
                BGMode, bg, sub, depth0, depth1);
    }
}


//-------------------------------------------------------------------
// Draw hires backgrounds
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawHiresBackgroundHardwarePriority0Inline (
    int tileSize, int tileShift, int bitShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
    uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    GFX.PixSize = 1;

	S9xComputeAndEnableStencilFunction(bg, sub);

 	// Note: We draw subscreens first, then the main screen.
	// So if the subscreen has already been drawn, and we are drawing the main screen,
	// we simply just redraw the same vertices that we have saved.
	//
	if (layerDrawn[bg])
	{
		gpu3dsBindTextureSnesTileCacheForHires(GPU_TEXUNIT0);
		gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();
		gpu3dsEnableAlphaTestNotEqualsZero();
		gpu3dsEnableDepthTest();
		gpu3dsDrawVertexes(true, bg);
		return;		
	}

	//printf ("BG%d Y=%d-%d W1:%d-%d W2:%d-%d\n", bg, GFX.StartY, GFX.EndY, PPU.Window1Left, PPU.Window1Right, PPU.Window2Left, PPU.Window2Right);

    BG.TileSize = tileSize;
    BG.BitShift = bitShift;
    BG.TileShift = tileShift;
    BG.TileAddress = PPU.BG[bg].NameBase << 1;
    BG.NameSelect = 0;
    BG.Buffer = IPPU.TileCache [Depths [BGMode][bg]];
    BG.Buffered = IPPU.TileCached [Depths [BGMode][bg]];
    BG.PaletteShift = paletteShift;
    BG.PaletteMask = paletteMask;
    BG.DirectColourMode = directColourMode;
	//BG.Depth = depth;

	//BG.DrawTileCount[bg] = 0;

	curBG = bg;

    uint32 Tile;
    uint16 *SC0;
    uint16 *SC1;
    uint16 *SC2;
    uint16 *SC3;
    uint32 Width;
    //uint8 depths [2] = {Z1, Z2};

    BG.StartPalette = 0;
	
    SC0 = (uint16 *) &Memory.VRAM[PPU.BG[bg].SCBase << 1];
	
    if ((PPU.BG[bg].SCSize & 1))
		SC1 = SC0 + 1024;
    else
		SC1 = SC0;
	
	if((SC1-(unsigned short*)Memory.VRAM)>0x10000)
		SC1=(uint16*)&Memory.VRAM[(((uint8*)SC1)-Memory.VRAM)%0x10000];
	
    if ((PPU.BG[bg].SCSize & 2))
		SC2 = SC1 + 1024;
    else SC2 = SC0;
	
	if(((uint8*)SC2-Memory.VRAM)>=0x10000)
		SC2-=0x08000;

    if ((PPU.BG[bg].SCSize & 1))
		SC3 = SC2 + 1024;
    else
		SC3 = SC2;
    
	if(((uint8*)SC3-Memory.VRAM)>=0x10000)
		SC3-=0x08000;

	
	
    int Lines;
    int VOffsetMask;
    int VOffsetShift;
	
    if (BG.TileSize == 16)
    {
		VOffsetMask = 0x3ff;
		VOffsetShift = 4;
    }
    else
    {
		VOffsetMask = 0x1ff;
		VOffsetShift = 3;
    }

    int endy = IPPU.Interlace ? 1 + (GFX.EndY << 1) : GFX.EndY;
	
    for (int Y = IPPU.Interlace ? GFX.StartY << 1 : GFX.StartY; Y <= endy; Y += Lines)
    {
		int y = IPPU.Interlace ? (Y >> 1) : Y;
		uint32 VOffset = LineData [y].BG[bg].VOffset;
		uint32 HOffset = LineData [y].BG[bg].HOffset;
		int VirtAlign = (Y + VOffset) & 7;
		
		for (Lines = 1; Lines < 8 - VirtAlign; Lines++)
			if ((VOffset != LineData [y + Lines].BG[bg].VOffset) ||
				(HOffset != LineData [y + Lines].BG[bg].HOffset))
				break;
			
		HOffset <<= 1;
		if (Y + Lines > endy)
			Lines = endy + 1 - Y;
		VirtAlign <<= 3;
		
		int ScreenLine = (VOffset + Y) >> VOffsetShift;
		int t1;
		int t2;
		if (((VOffset + Y) & 15) > 7)
		{
			t1 = 16;
			t2 = 0;
		}
		else
		{
			t1 = 0;
			t2 = 16;
		}
		uint16 *b1;
		uint16 *b2;
		
		if (ScreenLine & 0x20)
			b1 = SC2, b2 = SC3;
		else
			b1 = SC0, b2 = SC1;
		
		b1 += (ScreenLine & 0x1f) << 5;
		b2 += (ScreenLine & 0x1f) << 5;

		//int clipcount = GFX.pCurrentClip->Count [bg];
		//if (!clipcount)
		//	clipcount = 1;
		//for (int clip = 0; clip < clipcount; clip++)
		{
			uint32 Left;
			uint32 Right;

			//if (!GFX.pCurrentClip->Count [bg])
			{
				Left = 0;
				Right = 256;
			}
			/*else
			{
				Left = GFX.pCurrentClip->Left [clip][bg];
				Right = GFX.pCurrentClip->Right [clip][bg];

				if (Right <= Left)
					continue;
			}*/

			//uint32 s = Left * GFX.PixSize + Y * GFX.PPL;
			uint32 s = Left * GFX.PixSize + Y * 256;		// Once hardcoded, Hires mode no longer supported.
			uint32 HPos = (HOffset + Left * GFX.PixSize) & 0x3ff;
			
			uint32 Quot = HPos >> 3;
			uint32 Count = 0;
			
			uint16 *t;
			if (Quot > 63)
				t = b2 + ((Quot >> 1) & 0x1f);
			else
				t = b1 + (Quot >> 1);
				

			// screen coordinates of the tile.
			int sX = 0 - (HPos & 7);
			int sY = Y;
			int actualLines = Lines;
			if (IPPU.Interlace)
			{
				sY = sY >> 1;
				actualLines = actualLines >> 1;
			}

			int tilesToDraw = 64;
			if (sX != 0)
				tilesToDraw += 2;

			for (int tno = 0; tno <= tilesToDraw; tno++, sX += 8, Quot++)
			{
				Tile = READ_2BYTES(t);

				int tpriority = (Tile & 0x2000) >> 13;

					if (BG.TileSize == 8)
					{
						if (!(Tile & H_FLIP))
						{
							//if (tpriority == 0)
								S9xDrawHiresBGFullTileHardwareInline (
									tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
									tpriority, depth0, depth1,
									Tile + (Quot & 1), sX, sY, VirtAlign, actualLines);
							//else
							//		DrawFullTileLater (Tile + (Quot & 1), sX, sY, VirtAlign, actualLines);
							
							// Normal, unflipped
							//(*DrawHiResTilePtr) (Tile + (Quot & 1),
							//	s, VirtAlign, Lines);
						}
						else
						{
							//if (tpriority == 0)
								S9xDrawHiresBGFullTileHardwareInline (
									tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
									tpriority, depth0, depth1,
									Tile + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);
							//else
							//		DrawFullTileLater (Tile + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);

							// H flip
							//(*DrawHiResTilePtr) (Tile + 1 - (Quot & 1),
							//	s, VirtAlign, Lines);
						}
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							//if (tpriority == 0)
								S9xDrawHiresBGFullTileHardwareInline (
									tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
									tpriority, depth0, depth1,
									Tile + t1 + (Quot & 1), sX, sY, VirtAlign, actualLines);
							//else
							//		DrawFullTileLater (Tile + t1 + (Quot & 1), sX, sY, VirtAlign, actualLines);
							
							// Normal, unflipped
							//(*DrawHiResTilePtr) (Tile + t1 + (Quot & 1),
							//	s, VirtAlign, Lines);
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									//if (tpriority == 0)
										S9xDrawHiresBGFullTileHardwareInline (
											tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
											tpriority, depth0, depth1,
											Tile + t2 + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);
									//else
									//		DrawFullTileLater (Tile + t2 + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);
									
									// H & V flip
									//(*DrawHiResTilePtr) (Tile + t2 + 1 - (Quot & 1),
									//	s, VirtAlign, Lines);
								}
								else
								{
									//if (tpriority == 0)
										S9xDrawHiresBGFullTileHardwareInline (
											tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
											tpriority, depth0, depth1,
											Tile + t1 + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);
									//else
									//		DrawFullTileLater (Tile + t1 + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);

									// H flip only
									//(*DrawHiResTilePtr) (Tile + t1 + 1 - (Quot & 1),
									//	s, VirtAlign, Lines);
								}
							}
							else
							{
								//if (tpriority == 0)
									S9xDrawHiresBGFullTileHardwareInline (
										tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
										tpriority, depth0, depth1,
										Tile + t2 + (Quot & 1), sX, sY, VirtAlign, actualLines);
								//else
								//		DrawFullTileLater (Tile + t2 + (Quot & 1), sX, sY, VirtAlign, actualLines);
								
								// V flip only
								//(*DrawHiResTilePtr) (Tile + t2 + (Quot & 1),
								//	s, VirtAlign, Lines);
							}
					}
					
					t += Quot & 1;
					if (Quot == 63)
						t = b2;
					else
						if (Quot == 127)
							t = b1;

			}

		}
    }

	//printf ("BG %d P0\n", bg);
	//gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsBindTextureSnesTileCacheForHires(GPU_TEXUNIT0);
	gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsEnableDepthTest();
	gpu3dsDrawVertexes(false, bg);
	layerDrawn[bg] = true;
}




//-------------------------------------------------------------------
// 4-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawHiresBackgroundHardwarePriority0Inline_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawHiresBackgroundHardwarePriority0Inline_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawHiresBackgroundHardwarePriority0Inline_Mode0_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawHiresBackgroundHardwarePriority0Inline_Mode0_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}


void S9xDrawHiresBackgroundHardwarePriority0Inline_4Color
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    if (BGMode != 0)
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawHiresBackgroundHardwarePriority0Inline_4Color_8x8(
                BGMode, bg, sub, depth0, depth1);
        else
            S9xDrawHiresBackgroundHardwarePriority0Inline_4Color_16x16(
                BGMode, bg, sub, depth0, depth1);
    }
    else
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawHiresBackgroundHardwarePriority0Inline_Mode0_4Color_8x8(
                BGMode, bg, sub, depth0, depth1);
        else
            S9xDrawHiresBackgroundHardwarePriority0Inline_Mode0_4Color_16x16(
                BGMode, bg, sub, depth0, depth1);
    }
}


//-------------------------------------------------------------------
// 16-color Hires BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawHiresBackgroundHardwarePriority0Inline_16Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        8,              // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawHiresBackgroundHardwarePriority0Inline_16Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        16,             // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth0, depth1);
}

void S9xDrawHiresBackgroundHardwarePriority0Inline_16Color
    (uint32 BGMode, uint32 bg, bool sub, int depth0, int depth1)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawHiresBackgroundHardwarePriority0Inline_16Color_8x8(
            BGMode, bg, sub, depth0, depth1);
    }
    else
    {
        S9xDrawHiresBackgroundHardwarePriority0Inline_16Color_16x16(
            BGMode, bg, sub, depth0, depth1);
    }
}




inline void __attribute__((always_inline)) S9xDrawOBJTileHardware2 (
	bool sub, int depth, 
	uint32 snesTile,
	int screenX, int screenY, uint32 textureYOffset, int height)
{

	// Prepare tile for rendering
	//
    uint8 *pCache;
    uint16 *pCache16;

    uint32 TileAddr = BG.TileAddress + ((snesTile & 0x1ff) << 5);

	// OBJ tiles can be name-selected.
	if ((snesTile & 0x1ff) >= 256)
		TileAddr += BG.NameSelect;
	TileAddr &= 0xffff;

    uint32 TileNumber;
    pCache = &BG.Buffer[(TileNumber = (TileAddr >> 5)) << 6];

    if (!BG.Buffered [TileNumber])
    {
	    BG.Buffered[TileNumber] = S9xConvertTileTo8Bit (pCache, TileAddr);
        if (BG.Buffered [TileNumber] == BLANK_TILE)
            return;

        GFX.VRAMPaletteFrame[TileAddr / 8][8] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][9] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][10] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][11] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][12] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][13] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][14] = 0;
        GFX.VRAMPaletteFrame[TileAddr / 8][15] = 0;
    }

    if (BG.Buffered [TileNumber] == BLANK_TILE)
	    return;

	int texturePos = 0;

    uint32 l;
    uint8 pal;

	{
        pal = (snesTile >> 10) & 7;
		texturePos = cache3dsGetTexturePositionFast(TileAddr, pal + 8);
		//printf ("%d\n", texturePos);
        if (GFX.VRAMPaletteFrame[TileAddr / 8][pal + 8] != GFX.PaletteFrame[pal + 8])
        {
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal + 8);
            GFX.VRAMPaletteFrame[TileAddr / 8][pal + 8] = GFX.PaletteFrame[pal + 8];

			//printf ("cache %d\n", texturePos);
	        uint16 *screenColors = &IPPU.ScreenColors [(pal << 4) + 128];
			cache3dsCacheSnesTileToTexturePosition(pCache, screenColors, texturePos);
        }
    }

	// Render tile
	//
	// Remove the test for sub screen (fixed Mickey mouse transparency problem when Mickey's
	// talking to the wizard)
	//
	if (pal < 4)					
		depth = depth & 0xfff;		// remove the alpha.
	int x0 = screenX;
	int y0 = screenY + depth;
		
	int x1 = x0 + 8;
	int y1 = y0 + height;

	int tx0 = 0;
	int ty0 = textureYOffset;
	int tx1 = tx0 + 8;
	int ty1 = ty0 + height;

	//printf ("Draw: %d %d %d, %d %d %d %d - %d %d %d %d (%d)\n", screenOffset, startX, startY, x0, y0, x1, y1, txBase + tx0, tyBase + ty0, txBase + tx1, tyBase + ty1, texturePos);
	gpu3dsAddTileVertexes(
		x0, y0, x1, y1,
		tx0, ty0,
		tx1, ty1, (snesTile & (V_FLIP | H_FLIP)) + texturePos);
}


typedef struct 
{
	int		Height;
	int		Y;
	int		StartLine;
} SOBJList;

SOBJList OBJList[128];


//-------------------------------------------------------------------
// Draw the OBJ layers using 3D hardware.
//-------------------------------------------------------------------
void S9xDrawOBJSHardware (bool8 sub, int depth = 0, int priority = 0)
{
	S9xComputeAndEnableStencilFunction(4, sub);

	
	// Note: We draw subscreens first, then the main screen.
	// So if the subscreen has already been drawn, and we are drawing the main screen,
	// we simply just redraw the same vertices that we have saved.
	//
	if (layerDrawn[5])
	{
		gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
		gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();
		gpu3dsEnableAlphaTestNotEqualsZero();
		gpu3dsDisableDepthTest();
		gpu3dsDrawVertexes(true, 5);
		return;		
	}
	
#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Entering DrawOBJS() for %d-%d\n", GFX.StartY, GFX.EndY);
#endif
	CHECK_SOUND();

	//printf ("--------------------\n");
	int p = 0;			// To be used in the DrawTileLater/DrawClippedTileLater macros.

	BG.BitShift = 4;
	BG.TileShift = 5;
	BG.TileAddress = PPU.OBJNameBase;
	BG.StartPalette = 128;
	BG.PaletteShift = 4;
	BG.PaletteMask = 7;
	BG.Buffer = IPPU.TileCache [TILE_4BIT];
	BG.Buffered = IPPU.TileCached [TILE_4BIT];
	BG.NameSelect = PPU.OBJNameSelect;
	BG.DirectColourMode = FALSE;
	BG.Depth = depth;

	GFX.PixSize = 1;


#ifdef MK_DEBUG_RTO
if(Settings.BGLayering) {
	fprintf(stderr, "Windows:\n");
	for(int xxx=0; xxx<6; xxx++){ fprintf(stderr, "%d: %d = %d\n", xxx, Windows[xxx].Pos, Windows[xxx].Value); }
}
#endif

	if (PPU.PriorityDrawFromSprite >= 0 && 
		GFX.EndY - GFX.StartY >= 16)	// Wonder what is the best value for this to get the optimal performance? 
	{
		//printf ("Fast OBJ draw %d\n", PPU.PriorityDrawFromSprite);
		// Clear all heights
		for (int i = 0; i < 128;)
		{
			OBJList[i++].Height = 0;
			OBJList[i++].Height = 0;
			OBJList[i++].Height = 0;
			OBJList[i++].Height = 0;
			OBJList[i++].Height = 0;
			OBJList[i++].Height = 0;
			OBJList[i++].Height = 0;
			OBJList[i++].Height = 0;
		}
		for(uint32 Y=GFX.StartY; Y<=GFX.EndY; Y++)
		{
			for (int I = GFX.OBJLines[Y].OBJCount - 1; I >= 0; I --)
			{
				int S = GFX.OBJLines[Y].OBJ[I].Sprite;
				if (S < 0) continue;

				if (OBJList[S].Height == 0)
				{
					OBJList[S].Y = Y;
					OBJList[S].StartLine = GFX.OBJLines[Y].OBJ[I].Line;
				}
				OBJList[S].Height ++;
			}
		}

		int FirstSprite = PPU.PriorityDrawFromSprite;
		int StartDrawingSprite = (FirstSprite - 1) & 0x7F;
		int S = StartDrawingSprite;
		do {
			if (OBJList[S].Height)
			{
				int Height = OBJList[S].Height;
				int Y = OBJList[S].Y;
				int StartLine = OBJList[S].StartLine;

				while (Height > 0)
				{
					int BaseTile = (((StartLine<<1) + (PPU.OBJ[S].Name&0xf0))&0xf0) | (PPU.OBJ[S].Name&0x100) | (PPU.OBJ[S].Palette << 10);
					int TileX = PPU.OBJ[S].Name&0x0f;
					int TileLine = (StartLine&7);
					int TileInc = 1;
					int TileHeight = 0;
					if (PPU.OBJ[S].VFlip)
					{
						TileHeight = TileLine + 1;
						TileLine = 7 - TileLine;
						BaseTile |= V_FLIP;
					}
					else
					{
						TileHeight = 8 - TileLine;
					}
					if (TileHeight > Height)
						TileHeight = Height;

					if (PPU.OBJ[S].HFlip)
					{
						TileX = (TileX + (GFX.OBJWidths[S] >> 3) - 1) & 0x0f;
						BaseTile |= H_FLIP;
						TileInc = -1;
					}

					int X=PPU.OBJ[S].HPos; if(X==-256) X=256;

					//if (!clipcount)
					{
						// No clipping at all.
						//
						for (; X<=256 && X<PPU.OBJ[S].HPos+GFX.OBJWidths[S]; TileX=(TileX+TileInc)&0x0f, X+=8)
						{
							//if (X < 255) printf ("Draw S=%d @ %d,%d (Line=%d, H=%d, P=%d)\n", S, X, Y, TileLine, TileHeight, PPU.OBJ[S].Priority);
							
							//DrawOBJTileLater (PPU.OBJ[S].Priority, BaseTile|TileX, X, Y, TileLine);
							S9xDrawOBJTileHardware2 (sub, (PPU.OBJ[S].Priority + 1) * 3 * 256 + depth, 
								BaseTile|TileX, X, Y, TileLine, TileHeight);

						} // end for
					}
					Height -= TileHeight;
					Y += TileHeight;

					if (PPU.OBJ[S].VFlip)
					{
						StartLine -= TileHeight;
						if (StartLine < 0)
							StartLine += GFX.OBJWidths[S];
					}
					else
						StartLine += TileHeight;
				}
			}

			S = (S-1) & 0x7F;
		} while (S != StartDrawingSprite);
	}
	else
	{

	for(uint32 Y=GFX.StartY, Offset=Y*GFX.PPL; Y<=GFX.EndY; Y++, Offset+=GFX.PPL)
	{
#ifdef MK_DEBUG_RTO
		bool8 Flag=0;
#endif
		int I = 0;
#ifdef MK_DISABLE_TIME_OVER
		int tiles=0;
#else
		int tiles=GFX.OBJLines[Y].Tiles;
#endif
		//for (int S = GFX.OBJLines[Y].OBJ[I].Sprite; S >= 0 && I<32; S = GFX.OBJLines[Y].OBJ[++I].Sprite)
		for (int I = GFX.OBJLines[Y].OBJCount - 1; I >= 0; I --)
		{
			int S = GFX.OBJLines[Y].OBJ[I].Sprite;
			if (S < 0) continue;

			int BaseTile = (((GFX.OBJLines[Y].OBJ[I].Line<<1) + (PPU.OBJ[S].Name&0xf0))&0xf0) | (PPU.OBJ[S].Name&0x100) | (PPU.OBJ[S].Palette << 10);
			int TileX = PPU.OBJ[S].Name&0x0f;
			int TileLine = (GFX.OBJLines[Y].OBJ[I].Line&7);
			int TileInc = 1;

			if (PPU.OBJ[S].HFlip)
			{
				TileX = (TileX + (GFX.OBJWidths[S] >> 3) - 1) & 0x0f;
				BaseTile |= H_FLIP;
				TileInc = -1;
			}

			int X=PPU.OBJ[S].HPos; if(X==-256) X=256;

			//if (!clipcount)
			{
				// No clipping at all.
				//
				for (int t=tiles, O=Offset+X*GFX.PixSize; X<=256 && X<PPU.OBJ[S].HPos+GFX.OBJWidths[S]; TileX=(TileX+TileInc)&0x0f, X+=8, O+=8*GFX.PixSize)
				{
					//DrawOBJTileLater (PPU.OBJ[S].Priority, BaseTile|TileX, X, Y, TileLine);
					S9xDrawOBJTileHardware2 (sub, (PPU.OBJ[S].Priority + 1) * 3 * 256 + depth, 
						BaseTile|TileX, X, Y, TileLine, 1);

				} // end for
			}

		}
#ifdef MK_DEBUG_RTO
		if(Settings.BGLayering) if(Flag) fprintf(stderr, "\n");
#endif
	}
#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Exiting DrawOBJS() for %d-%d\n", GFX.StartY, GFX.EndY);
#endif

	}

	//gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
	gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsDisableDepthTest();
	gpu3dsDrawVertexes(false, 5);
	layerDrawn[5] = true;
}





//---------------------------------------------------------------------------
// Update one of the 256 mode 7 tiles with the latest texture.
// (uses a 256 color palette)
//---------------------------------------------------------------------------
void S9xPrepareMode7UpdateCharTile(int tileNumber)
{
	uint8 *charMap = &Memory.VRAM[1];	
	cache3dsCacheSnesTileToMode7TexturePosition( 
		&charMap[tileNumber * 128], GFX.ScreenColors, tileNumber, &IPPU.Mode7CharPaletteMask[tileNumber]); 
}


//---------------------------------------------------------------------------
// Update one of the 256 mode 7 tiles with the latest texture.
// (uses a 128 color palette)
//---------------------------------------------------------------------------
void S9xPrepareMode7ExtBGUpdateCharTile(int tileNumber)
{
	uint8 *charMap = &Memory.VRAM[1];	
	cache3dsCacheSnesTileToMode7TexturePosition( \
		&charMap[tileNumber * 128], GFX.ScreenColors128, tileNumber, &IPPU.Mode7CharPaletteMask[tileNumber]); \
}

//---------------------------------------------------------------------------
// Check to see if it is necessary to update the tile to the
// full texture.
//---------------------------------------------------------------------------
void S9xPrepareMode7CheckAndMarkPaletteChangedTiles()
{
	int charcount = 0;
	for (int c = 0; c < 256; c++)
	{
		if (IPPU.Mode7PaletteDirtyFlag & IPPU.Mode7CharPaletteMask[c])
		{
			//printf ("  chr %d, pal mask = %08x\n", c, IPPU.Mode7CharPaletteMask[c]);
			IPPU.Mode7CharDirtyFlag[c] = 2;
			IPPU.Mode7CharDirtyFlagCount = 1;
			charcount++;
		}
	}
	//printf ("M7pal: %08x chars:%d ", IPPU.Mode7PaletteDirtyFlag, charcount);
}


void S9xPrepareMode7CheckAndUpdateCharTiles()
{
	register uint8 *tileMap = &Memory.VRAM[0];
	register uint8 *charDirtyFlag = IPPU.Mode7CharDirtyFlag;

	int tilecount = 0;
	//register int tileNumber;
	int texturePos;
	int tileNumber;
	uint8 charFlag;

	#define CACHE_MODE7_TILE \
			tileNumber = tileMap[i * 2]; \
			charFlag = charDirtyFlag[tileNumber]; \
			if (charFlag) \
			{  \
					tilecount++; \
				gpu3dsSetMode7TileModifiedFlag(i); \
				gpu3dsSetMode7TileTexturePos(i, tileNumber); \
				if (charFlag == 2) \
				{ \ 
					S9xPrepareMode7UpdateCharTile(tileNumber); \
					charDirtyFlag[tileNumber] = 1; \
				} \
			} \
			i++; 

	#define CACHE_MODE7_EXTBG_TILE \
			tileNumber = tileMap[i * 2]; \
			charFlag = charDirtyFlag[tileNumber]; \
			if (charFlag) \
			{  \
				gpu3dsSetMode7TileModifiedFlag(i); \
				gpu3dsSetMode7TileTexturePos(i, tileNumber); \
					tilecount++; \
				if (charFlag == 2) \
				{ \ 
					S9xPrepareMode7ExtBGUpdateCharTile(tileNumber); \
					charDirtyFlag[tileNumber] = 1; \
				} \
			} \
			i++; 

	// Bug fix: The logic for the test was previously wrong.
	// This fixes some of the mode 7 tile problems in Secret of Mana.
	//
	//if (!Memory.FillRAM [0x2133] & 0x40)
	if (!IPPU.Mode7EXTBGFlag)
	{
		// Bug fix: Super Mario Kart Bowser Castle's tile 0 
		//
		if (PPU.Mode7Repeat == 3)
		{
			tileNumber = 0;
			charFlag = charDirtyFlag[tileNumber]; 
			if (charFlag == 2) 
			{ 
				S9xPrepareMode7UpdateCharTile(tileNumber); 
				charDirtyFlag[tileNumber] = 1; 
			}
		} 
		
		// Normal BG with 256 colours
		//
		for (int i = 0; i < 16384; )
		{
			CACHE_MODE7_TILE
			CACHE_MODE7_TILE
			CACHE_MODE7_TILE
			CACHE_MODE7_TILE
			
			CACHE_MODE7_TILE
			CACHE_MODE7_TILE
			CACHE_MODE7_TILE
			CACHE_MODE7_TILE

			CACHE_MODE7_TILE
			CACHE_MODE7_TILE
			CACHE_MODE7_TILE
			CACHE_MODE7_TILE

			CACHE_MODE7_TILE
			CACHE_MODE7_TILE
			CACHE_MODE7_TILE
			CACHE_MODE7_TILE
		}
	}
	else
	{
		// Bug fix: Super Mario Kart Bowser Castle's tile 0 
		//
		if (PPU.Mode7Repeat == 3)
		{
			tileNumber = 0;
			charFlag = charDirtyFlag[tileNumber]; 
			if (charFlag == 2) 
			{ 
				S9xPrepareMode7ExtBGUpdateCharTile(tileNumber); 
				charDirtyFlag[tileNumber] = 1; 
			}
		} 
		
		// Prepare the 128 color palette by duplicate colors from 0-127 to 128-255
		//
		// Low priority (set the alpha to 0xe, and make use of the inprecise
		// floating point math to achieve the same alpha translucency)
		//
		for (int i = 0; i < 128; i++)
			GFX.ScreenColors128[i] = GFX.ScreenRGB555toRGBA4[GFX.ScreenColors[i]] & 0xfffe;		
		// High priority 	
		for (int i = 0; i < 128; i++)
			GFX.ScreenColors128[i + 128] = GFX.ScreenRGB555toRGBA4[GFX.ScreenColors[i]];		

		// Ext BG with 128 colours
		//
		for (int i = 0; i < 16384; )
		{
			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE
			
			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE

			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE

			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE
			CACHE_MODE7_EXTBG_TILE

		}	
	}

	//printf ("t:%d\n ", tilecount);
	
}


//---------------------------------------------------------------------------
// Check to see if it is necessary to update the full texture.
// There are 128x128 full texture tiles and we will have to go through
// them one by one to do it.
//---------------------------------------------------------------------------
void S9xPrepareMode7CheckAndUpdateFullTexture()
{
	int prevShader = GPU3DS.currentShader;

	// Use our mode 7 shader
	//
	gpu3dsUseShader(3);	
	gpu3dsSetMode7UpdateFrameCountUniform();				
	
	for (int section = 0; section < 4; section++)
	{
		gpu3dsSetRenderTargetToMode7FullTexture((3 - section) * 0x40000, 512, 512);
		gpu3dsDrawMode7Vertexes(section * 4096, 4096);
	}	    
	
	gpu3dsSetRenderTargetToMode7Tile0Texture();
	gpu3dsDrawMode7Vertexes(16384, 4);
 
	//gpu3dsIncrementMode7UpdateFrameCount();

	// Restore our original shader.
	//
	gpu3dsUseShader(prevShader);		
}

//---------------------------------------------------------------------------
// Prepare the Mode 7 texture. This will be done only once in a single
// frame.
//---------------------------------------------------------------------------
void S9xPrepareMode7(bool sub)
{
	if (IPPU.Mode7Prepared)
		return;
	
	//printf ("xy= %d,%d - %d,%d \n", 
	//	PalXMin, PalYMin, PalXMax, PalYMax);

	t3dsStartTiming(70, "PrepM7");
	
	IPPU.Mode7Prepared = 1;

	// Bug fix: Force mode 7 tiles to update.
	//
	if ((Memory.FillRAM [0x2133] & 0x40) != IPPU.Mode7EXTBGFlag)
	{
		IPPU.Mode7EXTBGFlag = (Memory.FillRAM [0x2133] & 0x40);
		IPPU.Mode7PaletteDirtyFlag = 0xffffffff;
	}

	// Prepare the palette
	//
    if (GFX.r2130 & 1) 
    { 
		if (IPPU.DirectColourMapsNeedRebuild)
		{ 
			S9xBuildDirectColourMaps ();
			IPPU.Mode7PaletteDirtyFlag = 0xffffffff;
		} 
		GFX.ScreenColors = DirectColourMaps [0]; 
    } 
    else 
	{
		GFX.ScreenColors = IPPU.ScreenColors;
	} 

	t3dsStartTiming(71, "PrepM7-Palette");

	if (!IPPU.Mode7EXTBGFlag)
	{
		gpu3dsSetMode7TexturesPixelFormatToRGB5551();
	}
	else
	{
		gpu3dsSetMode7TexturesPixelFormatToRGB4444();
	}

	// If any of the palette colours in a palette group have changed, 
	// then we must refresh all tiles having those colours in that group.
	//
	if (IPPU.Mode7PaletteDirtyFlag)
		S9xPrepareMode7CheckAndMarkPaletteChangedTiles();

	// If any of the characters are updated due to palette changes,
	// or due to change in the bitmaps, then cache the new characters and
	// update the entire map.
	//
	if (IPPU.Mode7CharDirtyFlagCount)
		S9xPrepareMode7CheckAndUpdateCharTiles();

	t3dsEndTiming(71);

	t3dsStartTiming(72, "PrepM7-FullTile");
	gpu3dsDisableDepthTest();
	gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT0);
	gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsDisableAlphaTest();

	S9xPrepareMode7CheckAndUpdateFullTexture();
	t3dsEndTiming(72);

	t3dsStartTiming(73, "PrepM7-CharFlag");
	//printf ("Tiles updated %d, char map %d\n", tilecount, charmapupdated);

	// Restore the render target.
	//
	if (!sub)
	{
		gpu3dsSetRenderTargetToMainScreenTexture();
	}
	else
		gpu3dsSetRenderTargetToSubScreenTexture();

	for (int i = 0; i < 256; )
	{
		uint8 f1, f2, f3, f4;

		// We are loading the flags this way to force GCC
		// to re-arrange instructions to avoid the 3-cycle latency.
		//
		#define UPDATE_CHAR_FLAG \
			f1 = IPPU.Mode7CharDirtyFlag[i];   \
			f2 = IPPU.Mode7CharDirtyFlag[i+1]; \
			f3 = IPPU.Mode7CharDirtyFlag[i+2]; \
			f4 = IPPU.Mode7CharDirtyFlag[i+3]; \
			if (f1 == 1) { IPPU.Mode7CharDirtyFlag[i] = 0; }   \
			if (f2 == 1) { IPPU.Mode7CharDirtyFlag[i+1] = 0; } \
			if (f3 == 1) { IPPU.Mode7CharDirtyFlag[i+2] = 0; } \
			if (f4 == 1) { IPPU.Mode7CharDirtyFlag[i+3] = 0; } \
			i += 4; 

		UPDATE_CHAR_FLAG
		UPDATE_CHAR_FLAG
		UPDATE_CHAR_FLAG
		UPDATE_CHAR_FLAG

	}
	IPPU.Mode7PaletteDirtyFlag = 0;
	IPPU.Mode7CharDirtyFlagCount = 0;	

    gpu3dsIncrementMode7UpdateFrameCount();
	
	t3dsEndTiming(73);
		
	t3dsEndTiming(70);
}


extern int adjustableValue;

//---------------------------------------------------------------------------
// Draws the Mode 7 background.
//---------------------------------------------------------------------------
void S9xDrawBackgroundMode7Hardware(int bg, bool8 sub, int depth, int alphaTest)
{
	t3dsStartTiming(27, "DrawBG0_M7");
	//printf ("M7BG alphatest=%d\n", alphaTest);
	//printf ("adjustableValue: %x\n", adjustableValue);

	if (layerDrawn[bg])
	{
		gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();

		if (alphaTest == 0)
			gpu3dsEnableAlphaTestNotEqualsZero();
		else
		{
			if (GFX.r2131 & 0x40)
				gpu3dsEnableAlphaTestGreaterThanEquals(0x7f);
			else
				gpu3dsEnableAlphaTestGreaterThanEquals(0xf0);
		}

		gpu3dsEnableDepthTest();

		gpu3dsDrawMode7LineVertexes();
		gpu3dsDrawVertexes(true, 4);
	}

	S9xComputeAndEnableStencilFunction(bg, sub);

	for (int Y = GFX.StartY; Y <= GFX.EndY; Y++)
	{

		struct SLineMatrixData *p = &LineMatrixData [Y];

		int HOffset = ((int) LineData [Y].BG[0].HOffset << M7) >> M7; 
		int VOffset = ((int) LineData [Y].BG[0].VOffset << M7) >> M7; 
	
		int CentreX = ((int) p->CentreX << M7) >> M7; 
		int CentreY = ((int) p->CentreY << M7) >> M7; 

		//if (Y == GFX.StartY)
		//	printf ("OFS %d,%d M %d,%d,%d,%d C %d,%d\n", HOffset, VOffset, p->MatrixA, p->MatrixB, p->MatrixC, p->MatrixD, CentreX, CentreY);

		/*int clipcount = GFX.pCurrentClip->Count [0];
		if (!clipcount)
			clipcount = 1;
		
		for (int clip = 0; clip < clipcount; clip++)*/
		{
			int Left;
			int Right;
			int m7Left;
			int m7Right;

			//if (!GFX.pCurrentClip->Count [0])
			{
				m7Left = Left = 0;
				m7Right = Right = 256;
			}
			/*else
			{
				m7Left = Left = GFX.pCurrentClip->Left [clip][0];
				m7Right = Right = GFX.pCurrentClip->Right [clip][0];

				if (Right <= Left)
					continue;
			}*/
 
			// Bug fix: Used the original CLIP_10_BIT_SIGNED from Snes9x
			// This fixes the intro for Super Chase HQ.
			#define CLIP_10_BIT_SIGNED(a) \
				((a) & ((1 << 10) - 1)) + (((((a) & (1 << 13)) ^ (1 << 13)) - (1 << 13)) >> 3)

 			int yy = Y;

			// Bug fix: The mode 7 flipping problem.
			//
			if (PPU.Mode7VFlip) 
				yy = 255 - (int) Y; 
			if (PPU.Mode7HFlip) 
			{
				m7Left = 255 - m7Left;
				m7Right = 255 - m7Right;
			}

 			yy = yy + CLIP_10_BIT_SIGNED(VOffset - CentreY);

			int xx0 = m7Left + CLIP_10_BIT_SIGNED(HOffset - CentreX);
			int xx1 = m7Right + CLIP_10_BIT_SIGNED(HOffset - CentreX);

			int BB = p->MatrixB * yy + (CentreX << 8); 
			int DD = p->MatrixD * yy + (CentreY << 8); 

		    int AA0 = p->MatrixA * xx0; 
		    int CC0 = p->MatrixC * xx0; 
		    int AA1 = p->MatrixA * xx1; 
		    int CC1 = p->MatrixC * xx1; 

			//if (Y == GFX.StartY)
			//	printf ("xx0=%d, xx1=%d, yy=%d, AA0=%d, CC0=%d, AA1=%d, CC1=%d, BB=%d, DD=%d\n", xx0, xx1, yy, AA0, CC0, AA1, CC1, BB, DD);

		    /*int tx0 = ((AA0 + BB) / 256); 
		    int ty0 = ((CC0 + DD) / 256); 
		    int tx1 = ((AA1 + BB) / 256); 
		    int ty1 = ((CC1 + DD) / 256); */
		    float tx0 = ((float)(AA0 + BB) / 256.0f); 
		    float ty0 = ((float)(CC0 + DD) / 256.0f); 
		    float tx1 = ((float)(AA1 + BB) / 256.0f); 
		    float ty1 = ((float)(CC1 + DD) / 256.0f); 

			//if (Y==GFX.StartY)
			//	printf ("%d %d X=%d,%d Y=%d T=%d,%d %d,%d\n", sub, depth, Left, Right, Y, tx0, ty0, tx1, ty1);
			//if (Y % 4 == 0)
			//if (Y == GFX.StartY || Y == GFX.EndY)
			//	printf ("Y=%d D=%d T=%4.1f,%4.1f %4.1f,%4.1f\n", Y, depth, tx0, ty0, tx1, ty1);

			//gpu3dsAddMode7ScanlineVertexes(Left, Y+depth, Right, Y+1+depth, tx0, ty0, tx1, ty1, 0);
			gpu3dsAddMode7LineVertexes(Left, Y+depth, Right, Y+1+depth, tx0, ty0, tx1, ty1);
		}
	}

	gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();

	if (alphaTest == 0)
		gpu3dsEnableAlphaTestNotEqualsZero();
	else
	{
		if (GFX.r2131 & 0x40)
			gpu3dsEnableAlphaTestGreaterThanEquals(0x7f);
		else
			gpu3dsEnableAlphaTestGreaterThanEquals(0xf0);
	}

	gpu3dsEnableDepthTest();

	gpu3dsDrawMode7LineVertexes();
	gpu3dsDrawVertexes(false, 4);
	layerDrawn[bg] = true;
	t3dsEndTiming(27);
}


extern SGPUTexture *snesMode7Tile0Texture;

//---------------------------------------------------------------------------
// Draws the Mode 7 background (with repeat tile0)
//---------------------------------------------------------------------------
void S9xDrawBackgroundMode7HardwareRepeatTile0(int bg, bool8 sub, int depth)
{
	t3dsStartTiming(27, "DrawBG0_M7");
	
	S9xComputeAndEnableStencilFunction(bg, sub);
	
	for (int Y = GFX.StartY; Y <= GFX.EndY; Y++)
	{

		struct SLineMatrixData *p = &LineMatrixData [Y];

		int HOffset = ((int) LineData [Y].BG[0].HOffset << M7) >> M7; 
		int VOffset = ((int) LineData [Y].BG[0].VOffset << M7) >> M7; 
	
		int CentreX = ((int) p->CentreX << M7) >> M7; 
		int CentreY = ((int) p->CentreY << M7) >> M7; 

		//if (Y == GFX.StartY)
		//	printf ("OFS %d,%d M %d,%d,%d,%d C %d,%d\n", HOffset, VOffset, p->MatrixA, p->MatrixB, p->MatrixC, p->MatrixD, CentreX, CentreY);

		/*int clipcount = GFX.pCurrentClip->Count [0];
		if (!clipcount)
			clipcount = 1;
		
		for (int clip = 0; clip < clipcount; clip++)*/
		{
			uint32 Left;
			uint32 Right;

			//if (!GFX.pCurrentClip->Count [0])
			{
				Left = 0;
				Right = 256;
			}
			/*else
			{
				Left = GFX.pCurrentClip->Left [clip][0];
				Right = GFX.pCurrentClip->Right [clip][0];

				if (Right <= Left)
					continue;
			}*/
 
			// Bug fix: Used the original CLIP_10_BIT_SIGNED from Snes9x
			// This fixes the intro for Super Chase HQ.
			#define CLIP_10_BIT_SIGNED(a) \
				((a) & ((1 << 10) - 1)) + (((((a) & (1 << 13)) ^ (1 << 13)) - (1 << 13)) >> 3)
			 
 			int yy = Y;
 			yy = yy + CLIP_10_BIT_SIGNED(VOffset - CentreY);

			int xx0 = Left + CLIP_10_BIT_SIGNED(HOffset - CentreX);
			int xx1 = Right + CLIP_10_BIT_SIGNED(HOffset - CentreX);

			int BB = p->MatrixB * yy + (CentreX << 8); 
			int DD = p->MatrixD * yy + (CentreY << 8); 

		    int AA0 = p->MatrixA * xx0; 
		    int CC0 = p->MatrixC * xx0; 
		    int AA1 = p->MatrixA * xx1; 
		    int CC1 = p->MatrixC * xx1; 

			//if (Y == GFX.StartY)
			//	printf ("AA0=%d, CC0=%d, AA1=%d, CC1=%d, BB=%d, DD=%d\n", AA0, CC0, AA1, CC1, BB, DD);

		    /*int tx0 = ((AA0 + BB) / 256); 
		    int ty0 = ((CC0 + DD) / 256); 
		    int tx1 = ((AA1 + BB) / 256); 
		    int ty1 = ((CC1 + DD) / 256); */
		    float tx0 = ((float)(AA0 + BB) / 256.0f); 
		    float ty0 = ((float)(CC0 + DD) / 256.0f); 
		    float tx1 = ((float)(AA1 + BB) / 256.0f); 
		    float ty1 = ((float)(CC1 + DD) / 256.0f);

			//if (Y==GFX.StartY)
			//	printf ("%d %d X=%d,%d Y=%d T=%d,%d %d,%d\n", sub, depth, Left, Right, Y, tx0, ty0, tx1, ty1);
			//if (Y == GFX.StartY || Y == GFX.EndY)
			//	printf ("Y=%d D=%d T=%4.2f,%4.2f %4.2f,%4.2f\n", Y, depth, tx0 / 8, ty0 / 8, tx1 / 8, ty1 / 8);

			// This is used for repeating tile 0.
			// So the texture is completely within the 0-1024 boundary,
			// the tile 0 will not show up anyway, so we will skip drawing 
			// tile 0.
			//
			/*if (tx0 >= 0 && tx0 <= 1024 &&
				ty0 >= 0 && ty0 <= 1024 &&
				tx1 >= 0 && tx1 <= 1024 &&
				ty1 >= 0 && ty1 <= 1024)
				continue;
*/
			//gpu3dsAddMode7ScanlineVertexes(Left, Y+depth, Right, Y+1+depth, tx0, ty0, tx1, ty1, 0);
			//gpu3dsAddMode7LineVertexes(Left, Y+depth, Right, Y+1+depth, tx0, ty0, tx1, ty1);
			bool withinTexture = false;
			if ((tx0 >= 0 && tx0 <= 1024) &&
				(ty0 >= 0 && ty0 <= 1024) &&
			    (tx1 >= 0 && tx1 <= 1024) &&
				(ty1 >= 0 && ty1 <= 1024))
				withinTexture = true;

			if (!withinTexture)
				gpu3dsAddMode7LineVertexes(Left, Y+depth, Right, Y+1+depth, tx0, ty0, tx1, ty1);
		}
	}

	gpu3dsSetTextureEnvironmentReplaceTexture0WithColorAlpha();
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsEnableDepthTest();

	//gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsDrawMode7LineVertexes();
	gpu3dsDrawVertexes(false, 5);
	t3dsEndTiming(27);
}






//---------------------------------------------------------------------------
// Renders the screen from GFX.StartY to GFX.EndY
//---------------------------------------------------------------------------

void S9xRenderScreenHardware (bool8 sub, bool8 force_no_add, uint8 D)
{
	t3dsStartTiming(31, "RenderScnHW");
    bool8 BG0;
    bool8 BG1;
    bool8 BG2;
    bool8 BG3;
    bool8 OB;

	int BGAlpha0 = ALPHA_ZERO; 
	int BGAlpha1 = ALPHA_ZERO;
	int BGAlpha2 = ALPHA_ZERO;
	int BGAlpha3 = ALPHA_ZERO;
	int OBAlpha = ALPHA_ZERO;
	int BackAlpha = ALPHA_ZERO;

    if (!sub)
    {
		// Main Screen
		GFX.pCurrentClip = &IPPU.Clip [0];
		BG0 = ON_MAIN (0);
		BG1 = ON_MAIN (1);
		BG2 = ON_MAIN (2);
		BG3 = ON_MAIN (3);
		OB  = ON_MAIN (4);

		//printf ("Main Y:%d BGEnable:%d%d%d%d%d\n", GFX.StartY, BG0, BG1, BG2, BG3, OB);

    }
    else
    {
		// Sub Screen
		GFX.pCurrentClip = &IPPU.Clip [1];

		if (PPU.BGMode != 5 && PPU.BGMode != 6)
		{
			if (!GFX.Pseudo)
			{
				BG0 = ON_SUB (0);
				BG1 = ON_SUB (1);
				BG2 = ON_SUB (2);
				BG3 = ON_SUB (3);
				OB  = ON_SUB (4);
			}
			else
			{
				BG0 = ON_SUB_PSEUDO (0);
				BG1 = ON_SUB_PSEUDO (1);
				BG2 = ON_SUB_PSEUDO (2);
				BG3 = ON_SUB_PSEUDO (3);
				OB  = ON_SUB_PSEUDO (4);
			}
		}
		else
		{
			BG0 = ON_SUB_HIRES (0);
			BG1 = ON_SUB_HIRES (1);
			BG2 = ON_SUB_HIRES (2);
			BG3 = ON_SUB_HIRES (3);
			OB  = ON_SUB_HIRES (4);
		}
    }

	// We are going to use the same alphas as we do
	// for the main screen. This is because we are
	// going to create the same set of vertex data for
	// the given background in the sub screen as the
	// set we use for the main screen. This improves
	// performance of games that render the same BG to
	// main and sub screens.
	// 
	// Anyway the sub screen's alpha do not factor into
	// color math. So this is fine.
	//
	if (PPU.BGMode != 5 && PPU.BGMode != 6 && !GFX.Pseudo)
	{
		int alpha = ALPHA_1_0;	 	// for Add or Sub   
		if (GFX.r2131 & 0x40)	
			alpha = ALPHA_0_5;		// for Add / 2 or Sub / 2

		BGAlpha0 = SUB_OR_ADD (0) ? alpha : ALPHA_ZERO;
		BGAlpha1 = SUB_OR_ADD (1) ? alpha : ALPHA_ZERO;
		BGAlpha2 = SUB_OR_ADD (2) ? alpha : ALPHA_ZERO;
		BGAlpha3 = SUB_OR_ADD (3) ? alpha : ALPHA_ZERO;

		OBAlpha = SUB_OR_ADD (4) ? alpha : ALPHA_ZERO;
		BackAlpha = SUB_OR_ADD (5) ? alpha : ALPHA_ZERO;
	}
	else
	{
		BGAlpha0 = ALPHA_0_5;
		BGAlpha1 = ALPHA_0_5;
		BGAlpha2 = ALPHA_0_5;
		BGAlpha3 = ALPHA_0_5;

		OBAlpha = ALPHA_0_5;
		BackAlpha = ALPHA_0_5;
	}

    sub |= force_no_add;

	int depth = 0;


	#define DRAW_OBJS(p)  \
		if (OB) \
		{ \
			t3dsStartTiming(26, "DrawOBJS"); \
			S9xDrawOBJSHardware (sub, OBAlpha, p); \
			t3dsEndTiming(26); \
		} 


	#define DRAW_4COLOR_BG_INLINE(bg, p, d0, d1) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			S9xDrawBackgroundHardwarePriority0Inline_4Color (PPU.BGMode, bg, sub, d0 * 256 + BGAlpha##bg, d1 * 256 + BGAlpha##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_16COLOR_BG_INLINE(bg, p, d0, d1) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			S9xDrawBackgroundHardwarePriority0Inline_16Color (PPU.BGMode, bg, sub, d0 * 256 + BGAlpha##bg, d1 * 256 + BGAlpha##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_256COLOR_BG_INLINE(bg, p, d0, d1) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			S9xDrawBackgroundHardwarePriority0Inline_256Color (PPU.BGMode, bg, sub, d0 * 256 + BGAlpha##bg, d1 * 256 + BGAlpha##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_4COLOR_OFFSET_BG_INLINE(bg, p, d0, d1) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color (PPU.BGMode, bg, sub, d0 * 256 + BGAlpha##bg, d1 * 256 + BGAlpha##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_16COLOR_OFFSET_BG_INLINE(bg, p, d0, d1) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color (PPU.BGMode, bg, sub, d0 * 256 + BGAlpha##bg, d1 * 256 + BGAlpha##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_256COLOR_OFFSET_BG_INLINE(bg, p, d0, d1) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			S9xDrawOffsetBackgroundHardwarePriority0Inline_256Color (PPU.BGMode, bg, sub, d0 * 256 + BGAlpha##bg, d1 * 256 + BGAlpha##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_4COLOR_HIRES_BG_INLINE(bg, p, d0, d1) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			S9xDrawHiresBackgroundHardwarePriority0Inline_4Color (PPU.BGMode, bg, sub, d0 * 256 + BGAlpha##bg, d1 * 256 + BGAlpha##bg); \
			t3dsEndTiming(21 + bg); \
		}


	#define DRAW_16COLOR_HIRES_BG_INLINE(bg, p, d0, d1) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			S9xDrawHiresBackgroundHardwarePriority0Inline_16Color (PPU.BGMode, bg, sub, d0 * 256 + BGAlpha##bg, d1 * 256 + BGAlpha##bg); \
			t3dsEndTiming(21 + bg); \
		}


	//printf ("Mode: %d (%d-%d), %s\n", PPU.BGMode, GFX.StartY, GFX.EndY, sub ? "S" : "M");
	//printf ("BG Enable %d%d%d%d%d (%s)\n", BG0, BG1, BG2, BG3, OB, sub ? "S" : "M");
	
	//if (GFX.StartY == 0)
	//	printf("BG Mode: %d\n", PPU.BGMode);

	switch (PPU.BGMode)
	{
		case 0:
			S9xDrawBackdropHardware(sub, BackAlpha);

			DRAW_OBJS(0);
			DRAW_4COLOR_BG_INLINE(0, 0, 8, 11);
			DRAW_4COLOR_BG_INLINE(1, 0, 7, 10);
			DRAW_4COLOR_BG_INLINE(2, 0, 2, 5);
			DRAW_4COLOR_BG_INLINE(3, 0, 1, 4);

			break;
		case 1:
			S9xDrawBackdropHardware(sub, BackAlpha);

			DRAW_OBJS(0);
			DRAW_16COLOR_BG_INLINE(0, 0, 8, 11);
			DRAW_16COLOR_BG_INLINE(1, 0, 7, 10);
			if (PPU.BG3Priority) 
				{ DRAW_4COLOR_BG_INLINE(2, 0, 2, 13); }
			else 
				{ DRAW_4COLOR_BG_INLINE(2, 0, 2, 5); }

			break;

		case 2:
			S9xDrawBackdropHardware(sub, BackAlpha);

			DRAW_OBJS(0);
			DRAW_16COLOR_OFFSET_BG_INLINE(0, 0, 5, 11);
			DRAW_16COLOR_OFFSET_BG_INLINE(1, 0, 2, 8);

			break;
		case 3:
			S9xDrawBackdropHardware(sub, BackAlpha);

			DRAW_OBJS(0);
			DRAW_256COLOR_BG_INLINE(0, 0, 5, 11);
			DRAW_16COLOR_BG_INLINE(1, 0, 2, 8);

			break;
		case 4:
			S9xDrawBackdropHardware(sub, BackAlpha);

			DRAW_OBJS(0);
			DRAW_256COLOR_OFFSET_BG_INLINE(0, 0, 5, 11);
			DRAW_4COLOR_OFFSET_BG_INLINE(1, 0, 2, 8);

			break;
		case 5:
			S9xDrawBackdropHardware(false, BackAlpha);

			DRAW_OBJS(0);

			if (sub)
				gpu3dsSetTextureOffset(0, 0);		// even pixels on sub-screen
			else
				gpu3dsSetTextureOffset(1, 0);
			
			DRAW_16COLOR_HIRES_BG_INLINE(0, 0, 5, 11);
			DRAW_4COLOR_HIRES_BG_INLINE(1, 0, 2, 8);

			break;
		case 6:
			S9xDrawBackdropHardware(false, BackAlpha);

			DRAW_OBJS(0);

			if (sub)
				gpu3dsSetTextureOffset(0, 0);		// even pixels on sub-screen
			else
				gpu3dsSetTextureOffset(1, 0);
			
			DRAW_16COLOR_OFFSET_BG_INLINE(0, 0, 5, 11);

			break;
		case 7:
			// TODO: Mode 7 graphics.
			//
			S9xDrawBackdropHardware(sub, BackAlpha);

			gpu3dsSetTextureEnvironmentReplaceTexture0();
			S9xPrepareMode7(sub);

			#define DRAW_M7BG(bg, d, alphaTest) \
				if (BG##bg) \
				{ \
					if (PPU.Mode7Repeat == 0) \
					{ \
						gpu3dsBindTextureSnesMode7FullRepeat(GPU_TEXUNIT0); \
						S9xDrawBackgroundMode7Hardware(bg, sub, BGAlpha##bg + d*256, alphaTest); \
					} \
					else if (PPU.Mode7Repeat == 2) \
					{ \
						gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT0); \
						S9xDrawBackgroundMode7Hardware(bg, sub, BGAlpha##bg + d*256, alphaTest); \
					} \
					else \ 
					{ \
						gpu3dsBindTextureSnesMode7Tile0CacheRepeat(GPU_TEXUNIT0); \
						S9xDrawBackgroundMode7HardwareRepeatTile0(bg, sub, BGAlpha##bg + d*256); \
						gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT0); \
						S9xDrawBackgroundMode7Hardware(bg, sub, BGAlpha##bg + d*256, alphaTest); \
					} \
				}

				
			DRAW_OBJS(0);
			//printf ("M7Repeat:%d EXTBG:%d\n", PPU.Mode7Repeat, IPPU.Mode7EXTBGFlag);
			//printf ("$2131 = %x, $2133 = %x (%s)\n", GFX.r2131, Memory.FillRAM [0x2133], sub ? "S" : "M");
			if (IPPU.Mode7EXTBGFlag)
			{
				DRAW_M7BG(1, 2, 0);
				DRAW_M7BG(1, 8, 1);
				DRAW_M7BG(0, 5, 0);
			}
			else
			{
				DRAW_M7BG(0, 5, 0);
			}

			// debugging only
			//
			/*
			printf ("x");
			gpu3dsSetTextureEnvironmentReplaceTexture0();
			//gpu3dsSetRenderTargetToMainScreenTexture();
			gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT0);
			gpu3dsDisableDepthTest();
			gpu3dsDisableAlphaTest();
			gpu3dsAddTileVertexes(0, 0, 200, 200, 0, 0, 16, 16, 0);*/
			break;
	
	}
	t3dsEndTiming(31);
}


// ********************************************************************************************

//-----------------------------------------------------------
// Render color math.
//-----------------------------------------------------------
inline void S9xRenderColorMath()
{
	gpu3dsEnableAlphaTestNotEqualsZero();
	
	if (PPU.BGMode == 5 || PPU.BGMode == 6 || GFX.Pseudo)
	{
		// For hi-res modes, we will always do add / 2 blending
		// NOTE: This is not the SNES doing any blending, but
		// we are actually emulating the TV doing the blending 
		// of both main/sub screens!

		//gpu3dsEnableDepthTest();
		gpu3dsDisableDepthTest();

		// Subscreen math
		//
		gpu3dsBindTextureSubScreen(GPU_TEXUNIT0);
		gpu3dsSetTextureEnvironmentReplaceTexture0();
		gpu3dsSetRenderTargetToMainScreenTexture();

		gpu3dsEnableAdditiveDiv2Blending();	// div 2

		gpu3dsAddTileVertexes(0, GFX.StartY, 256, GFX.EndY + 1,
			0, GFX.StartY, 256, GFX.EndY + 1, 0);
		gpu3dsDrawVertexes();
		gpu3dsDisableDepthTest();

	}
	else if (GFX.r2130 & 2)
	{
		// Bug fix: We have to render the subscreen as long either of the
		//          212D and 2131 registers are set for any BGs.
		//
		//			This fixes Zelda's prologue's where the room is supposed to
		//			be dark.
		//
		if (ANYTHING_ON_SUB || ADD_OR_SUB_ON_ANYTHING)
		{
			//gpu3dsEnableDepthTest();
			gpu3dsDisableDepthTest();

			// Subscreen math
			//
			gpu3dsBindTextureSubScreen(GPU_TEXUNIT0);
			gpu3dsSetTextureEnvironmentReplaceTexture0();
			gpu3dsSetRenderTargetToMainScreenTexture();
			
			if (GFX.r2131 & 0x80)
			{
				// Subtractive
				if (GFX.r2131 & 0x40) gpu3dsEnableSubtractiveDiv2Blending();	// div 2
				else gpu3dsEnableSubtractiveBlending();						// no div
			}
			else
			{
				// Additive
				if (GFX.r2131 & 0x40) gpu3dsEnableAdditiveDiv2Blending();	// div 2
				else gpu3dsEnableAdditiveBlending();					// no div
			}

			// Debugging only
			/*
			if (GFX.r2131 & 0x80)
			{
				// Subtractive
				if (GFX.r2131 & 0x40) printf("  subcreen SUB/2\n");
				else printf("  subcreen SUB\n");
			}
			else
			{
				// Additive
				if (GFX.r2131 & 0x40) printf("  subcreen ADD/2\n");	// div 2
				else printf("  subcreen ADD\n");					// no div
			}
			*/

			gpu3dsAddTileVertexes(0, GFX.StartY, 256, GFX.EndY + 1,
				0, GFX.StartY, 256, GFX.EndY + 1, 0);
			gpu3dsDrawVertexes();

			gpu3dsDisableDepthTest();

		}
	}
	else
	{

		// Colour Math
		//
		//gpu3dsEnableDepthTest();
		gpu3dsDisableDepthTest();

		gpu3dsSetTextureEnvironmentReplaceColor();
		gpu3dsSetRenderTargetToMainScreenTexture();

		if (GFX.r2131 & 0x80)
		{
			// Subtractive
			if (GFX.r2131 & 0x40) gpu3dsEnableSubtractiveDiv2Blending();	// div 2
			else gpu3dsEnableSubtractiveBlending();						// no div
		}
		else
		{
			// Additive
			if (GFX.r2131 & 0x40) gpu3dsEnableAdditiveDiv2Blending();	// div 2
			else gpu3dsEnableAdditiveBlending();					// no div
		}
		
		// Debugging only
		/*
		if (GFX.r2131 & 0x80)
		{
			// Subtractive
			if (GFX.r2131 & 0x40) printf("  fixedcol SUB/2\n");
			else printf("  fixedcol SUB\n");
		}
		else
		{
			// Additive
			if (GFX.r2131 & 0x40) printf("  fixedcol ADD/2\n");	// div 2
			else printf("  fixedcol ADD\n");					// no div
		}
		*/

		for (int i = 0; i < IPPU.FixedColorSections.Count; i++)
		{
			uint32 fixedColour = IPPU.FixedColorSections.Section[i].Value;

			if (fixedColour != 0xff)
			{
				// debugging only
				//if (GFX.r2131 & 0x80) printf ("  -"); else printf("  +");
				//if (GFX.r2131 & 0x40) printf ("/2"); else printf("/1");
				//printf (" cmath Y:%d-%d, 2131:%02x, %04x\n", IPPU.FixedColorSections.Section[i].StartY, IPPU.FixedColorSections.Section[i].EndY, GFX.r2131, fixedColour);

				gpu3dsAddRectangleVertexes(
					0, IPPU.FixedColorSections.Section[i].StartY, 
					256, IPPU.FixedColorSections.Section[i].EndY + 1, 0, fixedColour);
			}
		}
		gpu3dsDrawVertexes();

		gpu3dsDisableDepthTest();
	}
}

inline void S9xRenderClipToBlackAndColorMath()
{
	t3dsStartTiming(29, "Colormath");

	if ((GFX.r2130 & 0xc0) != 0)
	{
		// Clip to main screen to black before color math
		//
		if (S9xComputeAndEnableStencilFunction(5, 0))
		{
			//printf ("clear to black: Y %d-%d 2130:%02x\n", GFX.StartY, GFX.EndY, GFX.r2130);
			
			gpu3dsDisableAlphaBlendingKeepDestAlpha();
			gpu3dsDisableDepthTest();
			
			gpu3dsSetTextureEnvironmentReplaceColor();
			gpu3dsSetRenderTargetToMainScreenTexture();
			gpu3dsDisableAlphaTest();

			gpu3dsAddRectangleVertexes(
				0, GFX.StartY, 256, GFX.EndY + 1, 0, 0xff);
			gpu3dsDrawVertexes();
		}
	}

	if ((GFX.r2130 & 0x30) != 0x30 || PPU.BGMode == 5 || PPU.BGMode == 6 || GFX.Pseudo)
	{
		// Do actual color math
		//
		if (S9xComputeAndEnableStencilFunction(5, 1))
		{
			S9xRenderColorMath();
		}
	}

	t3dsEndTiming(29);
}


//-----------------------------------------------------------
// Render brightness / forced blanking.
// Improves performance slightly.
//-----------------------------------------------------------
void S9xRenderBrightness()
{
	gpu3dsSetRenderTargetToMainScreenTexture();
	gpu3dsDisableStencilTest();
	gpu3dsDisableDepthTest();
	gpu3dsEnableAlphaBlending();
	gpu3dsSetTextureEnvironmentReplaceColor();

	for (int i = 0; i < IPPU.BrightnessSections.Count; i++)
	{
		int brightness = IPPU.BrightnessSections.Section[i].Value;
		if (brightness != 0xF)
		{
			int32 alpha = 0xF - brightness;
			alpha = alpha | (alpha << 4);	

			gpu3dsAddRectangleVertexes(
				0, IPPU.BrightnessSections.Section[i].StartY, 
				256, IPPU.BrightnessSections.Section[i].EndY + 1, 0, alpha);
			
		}
	}

	gpu3dsDrawVertexes();	
}

//-----------------------------------------------------------
// Draws the windows on the stencils.
//-----------------------------------------------------------
void S9xDrawStencilForWindows()
{
	int stencilEndX[10];
	int stencilMask[10];


	// If none of the windows are enabled, we are not going to draw any stencils
	//
	uint8 windowEnableMask = Memory.FillRAM[0x212e] | Memory.FillRAM[0x212f] | 0x20;
	IPPU.WindowingEnabled = false;
	for (int layer = 0; layer < 6; layer++)
		if ((PPU.ClipWindow1Enable[layer] || PPU.ClipWindow2Enable[layer]) && 
			((windowEnableMask >> layer) & 1) )
		{
			IPPU.WindowingEnabled = true;
		}
	
	//printf ("Y %d-%d Window Enabled: %d\n", GFX.StartY, GFX.EndY, IPPU.WindowingEnabled);
	if (!IPPU.WindowingEnabled)
		return;

	t3dsStartTiming(30, "DrawWindowStencils");

	gpu3dsSetRenderTargetToDepthTexture();
	gpu3dsSetTextureEnvironmentReplaceColor();
	gpu3dsDisableDepthTest();
	gpu3dsDisableAlphaBlending();
	gpu3dsDisableAlphaTest();
	gpu3dsDisableStencilTest();

	for (int i = 0; i < IPPU.WindowLRSections.Count; i++)
	{
		int startY = IPPU.WindowLRSections.Section[i].StartY;
		int endY = IPPU.WindowLRSections.Section[i].EndY;

		int w1Left = IPPU.WindowLRSections.Section[i].V1;
		int w1Right = IPPU.WindowLRSections.Section[i].V2;
		int w2Left = IPPU.WindowLRSections.Section[i].V3;
		int w2Right = IPPU.WindowLRSections.Section[i].V4;

		ComputeClipWindowsForStenciling (w1Left, w1Right, w2Left, w2Right, stencilEndX, stencilMask);

		//printf ("Y=%d-%d W1:%d-%d W2:%d-%d \n", startY, endY, w1Left, w1Right, w2Left, w2Right);
		int startX = 0;
		for (int s = 0; s < 10; s++)
		{
			int endX = stencilEndX[s];
			int mask = stencilMask[s];

			//printf ("  X=%3d-%3d m:%d%d%d (%x)\n", startX, endX, 
			//	(mask >> 2) & 1, (mask >> 1) & 1, (mask >> 0) & 1, mask  );
			
			gpu3dsAddRectangleVertexes(startX, startY, endX, endY + 1, 0, (mask << (29)));	

			startX = endX;
			if (startX >= 256)
				break;
		}
	}

	gpu3dsDrawVertexes();
	//printf ("\n"); 
	t3dsEndTiming(30);
}




//-----------------------------------------------------------
// Updates the screen using the 3D hardware.
//-----------------------------------------------------------
void S9xUpdateScreenHardware ()
{	
	// debugging only
	/*
	static int prevnewcacheTexturePosition = -1;
	if (GPU3DS.newCacheTexturePosition != prevnewcacheTexturePosition)
	{
		printf ("nctp: %d\n", GPU3DS.newCacheTexturePosition);
		prevnewcacheTexturePosition = GPU3DS.newCacheTexturePosition;
	}*/

	t3dsStartTiming(11, "S9xUpdateScreen");
    int32 x2 = 1; 

    GFX.S = GFX.Screen;
    GFX.r2131 = Memory.FillRAM [0x2131];
    GFX.r212c = Memory.FillRAM [0x212c];
    GFX.r212d = Memory.FillRAM [0x212d];
    GFX.r2130 = Memory.FillRAM [0x2130];

/*
#ifdef JP_FIX

    GFX.Pseudo = (Memory.FillRAM [0x2133] & 8) != 0 &&
				 (GFX.r212c & 15) != (GFX.r212d & 15) &&
				 (GFX.r2131 == 0x3f);

#else

    GFX.Pseudo = (Memory.FillRAM [0x2133] & 8) != 0 &&
		(GFX.r212c & 15) != (GFX.r212d & 15) &&
		(GFX.r2131 & 0x3f) == 0;

#endif*/

	// Fixed pseudo hi-res (Kirby Dreamland 3)
    GFX.Pseudo = (Memory.FillRAM [0x2133] & 8) != 0 &&
				 (GFX.r212c & 15) != (GFX.r212d & 15);

    if (IPPU.OBJChanged)
		S9xSetupOBJ ();

    /*if (PPU.RecomputeClipWindows)
    {
		t3dsStartTiming(30, "ComputeClipWindows");
		ComputeClipWindows ();
		PPU.RecomputeClipWindows = FALSE;
		t3dsEndTiming(30);
    }*/

	// Vertical sections
	// We commit the current values to create a new section up
	// till the current rendered line - 1.
	//
	S9xCommitVerticalSection(&IPPU.BrightnessSections);
	S9xCommitVerticalSection(&IPPU.BackdropColorSections);
	S9xCommitVerticalSection(&IPPU.FixedColorSections);
	S9xCommitVerticalSection(&IPPU.WindowLRSections);

	S9xDrawStencilForWindows();

    GFX.StartY = IPPU.PreviousLine;
    if ((GFX.EndY = IPPU.CurrentLine - 1) >= PPU.ScreenHeight)
		GFX.EndY = PPU.ScreenHeight - 1;

	// XXX: Check ForceBlank? Or anything else?
	PPU.RangeTimeOver |= GFX.OBJLines[GFX.EndY].RTOFlags;

    uint32 starty = GFX.StartY;
    uint32 endy = GFX.EndY;

	gpu3dsSetTextureOffset(0, 0); 
	gpu3dsDisableDepthTest();
	gpu3dsDisableAlphaBlending();

	layerDrawn[0] = false;
	layerDrawn[1] = false;
	layerDrawn[2] = false;
	layerDrawn[3] = false;
	layerDrawn[4] = false;
	layerDrawn[5] = false;
	layerDrawn[6] = false;

	// Bug fix: We have to render as long as 
	// the 2130 register says that we have are
	// doing color math using the subscreen 
	// (instead of the fixed color)
	//
	// This is because the backdrop color will be
	// used for the color math.
	//
	//printf ("Render Y:%d-%d M%d\n", GFX.StartY, GFX.EndY, PPU.BGMode);

	if (ANYTHING_ON_SUB || (GFX.r2130 & 2) || PPU.BGMode == 5 || PPU.BGMode == 6 || GFX.Pseudo)
	{
		// debugging only
		//printf ("SS Y:%d-%d M%d TS:%x\n", GFX.StartY, GFX.EndY, PPU.BGMode, GFX.r212d & 0x1f);

		// Render the subscreen
		//
		gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
		gpu3dsSetRenderTargetToSubScreenTexture();
		S9xRenderScreenHardware (TRUE, TRUE, SUB_SCREEN_DEPTH);
	}

	// debugging only
	//printf ("MS Y:%d-%d M%d TM:%x\n", GFX.StartY, GFX.EndY, PPU.BGMode, GFX.r212c & 0x1f);

	// Render the main screen.
	//
	gpu3dsSetRenderTargetToMainScreenTexture();
	gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
	S9xRenderScreenHardware (FALSE, FALSE, MAIN_SCREEN_DEPTH);

	// Do clip to black + color math here
	//
	gpu3dsEnableAlphaBlending();
	S9xRenderClipToBlackAndColorMath();

	// Render the brightness
	//
	S9xRenderBrightness();

	/*
	// For debugging only	
	// (displays the mode 7 full texture)
	// 
	gpu3dsDisableStencilTest();
	gpu3dsDisableDepthTest();
	gpu3dsDisableAlphaTest();
	gpu3dsDisableAlphaBlending();
	gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT0);
	gpu3dsSetRenderTargetToMainScreenTexture();
	gpu3dsAddTileVertexes(0, 0, 220, 220, 437, 894, 587, 1025, 0);
	gpu3dsDrawVertexes();
	*/
	
	/*	
	// For debugging only	
	// (displays the final main screen/sub screen at the bottom right corner)
	// 
	gpu3dsDisableStencilTest();
	gpu3dsDisableDepthTest();
	gpu3dsDisableAlphaTest();
	gpu3dsDisableAlphaBlending();
	gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsSetRenderTargetToMainScreenTexture();
	gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
	gpu3dsAddTileVertexes(150, 170, 200, 220, 0, 0, 256, 256, 0);
	gpu3dsDrawVertexes();
	gpu3dsBindTextureSubScreen(GPU_TEXUNIT0);
	gpu3dsAddTileVertexes(200, 170, 250, 220, 0, 0, 256, 256, 0);
	gpu3dsDrawVertexes();
	*/

	/*
	// For debugging only	
	// (displays the mode 7 full texture)
	// 
	gpu3dsDisableStencilTest();
	gpu3dsDisableDepthTest();
	gpu3dsDisableAlphaTest();
	gpu3dsDisableAlphaBlending();
	gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsSetRenderTargetToMainScreenTexture();
	gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT0);
	gpu3dsAddTileVertexes(0, 0, 200, 200, 0, 0, 1024, 1024, 0);
	gpu3dsDrawVertexes();
	*/

	S9xResetVerticalSection(&IPPU.BrightnessSections);
	S9xResetVerticalSection(&IPPU.BackdropColorSections);
	S9xResetVerticalSection(&IPPU.FixedColorSections);
	S9xResetVerticalSection(&IPPU.WindowLRSections);

    IPPU.PreviousLine = IPPU.CurrentLine;
	t3dsEndTiming(11);
}
