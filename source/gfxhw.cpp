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

#include "3dsopt.h"
#include "3dsgpu.h"
#include <3ds.h>

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

#define ANYTHING_ON_SUB \
((GFX.r2130 & 0x30) != 0x30 && \
 (GFX.r2130 & 2) && \
 (GFX.r212d & 0x1f))

#define ADD_OR_SUB_ON_ANYTHING \
(GFX.r2131 & 0x3f)



#define DrawTileLater(Tile, Offset, StartLine, LineCount) \
	{ \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][0] = 0; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][1] = Tile; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][2] = Offset; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][3] = StartLine; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][4] = LineCount; \
		BG.DrawTileCount[bg]++; \
	}


#define DrawFullTileLater(Tile, ScreenX, ScreenY, StartLine, LineCount) \
	{ \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][0] = 2; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][1] = Tile; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][2] = ScreenX; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][3] = ScreenY; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][4] = StartLine; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][5] = LineCount; \
		BG.DrawTileCount[bg]++; \
	}

#define DrawClippedTileLater(Tile, Offset, StartPixel, Width, StartLine, LineCount) \
	{ \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][0] = 1; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][1] = Tile; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][2] = Offset; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][3] = StartPixel; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][4] = Width; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][5] = StartLine; \
		BG.DrawTileParameters[bg][BG.DrawTileCount[bg]][6] = LineCount; \
		BG.DrawTileCount[bg]++; \
	}

#define DrawOBJTileLater(priority, Tile, ScreenX, ScreenY, TextureYOffset) \
	{ \
		int newIndex = BG.DrawOBJTileLaterParametersCount++; \
		int newBGIndex = BG.DrawOBJTileLaterIndexCount[priority]++; \
		BG.DrawOBJTileLaterIndex[priority][newBGIndex] = newIndex; \
		BG.DrawOBJTileLaterParameters[newIndex][0] = priority; \
		BG.DrawOBJTileLaterParameters[newIndex][1] = Tile; \
		BG.DrawOBJTileLaterParameters[newIndex][2] = ScreenX; \
		BG.DrawOBJTileLaterParameters[newIndex][3] = ScreenY; \
		BG.DrawOBJTileLaterParameters[newIndex][4] = TextureYOffset; \
	}



//-------------------------------------------------------------------
// Render the backdrop
//-------------------------------------------------------------------
void S9xDrawBackdropHardware(bool sub, int depth)
{
    uint32 starty = GFX.StartY;
    uint32 endy = GFX.EndY;

	gpu3dsDisableStencilTest();
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
				//printf ("Backdrop %04x, Y:%d-%d, 2130:%02x\n", backColor, IPPU.BackdropColorSections.Section[i].StartY, IPPU.BackdropColorSections.Section[i].EndY, GFX.r2130);
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
			if (backColor == 0xff) backColor = 0;
			
			gpu3dsAddRectangleVertexes(
				0, IPPU.FixedColorSections.Section[i].StartY + depth, 
				256, IPPU.FixedColorSections.Section[i].EndY + 1 + depth, 0, backColor);

		}
		
		gpu3dsDrawVertexes();
	}


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
// Draw a clipped BG tile using 3D hardware.
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawBGClippedTileHardwareInline (
    int tileSize, int tileShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
	int32 snesTile, int32 screenOffset,
	int32 startX, int32 width,
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

        GFX.VRAMPaletteFrame[TileAddr][0] = 0;
        GFX.VRAMPaletteFrame[TileAddr][1] = 0;
        GFX.VRAMPaletteFrame[TileAddr][2] = 0;
        GFX.VRAMPaletteFrame[TileAddr][3] = 0;
        GFX.VRAMPaletteFrame[TileAddr][4] = 0;
        GFX.VRAMPaletteFrame[TileAddr][5] = 0;
        GFX.VRAMPaletteFrame[TileAddr][6] = 0;
        GFX.VRAMPaletteFrame[TileAddr][7] = 0;
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
		texturePos = cacheGetTexturePositionFast(TileAddr, pal);

		//printf ("  TIDM addr:%x pal:%d %d\n", TileAddr, pal, texturePos);

        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + startPalette / 16])
        {
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + startPalette / 16];

			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        }
    }
    else
    {

        pal = (snesTile >> 10) & paletteMask;
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << paletteShift) + startPalette];
		texturePos = cacheGetTexturePositionFast(TileAddr, pal);
		//printf ("%d\n", texturePos);

		uint32 *paletteFrame = GFX.PaletteFrame;
		if (paletteShift == 2)
			paletteFrame = GFX.PaletteFrame4;
		else if (paletteShift == 0)
		{
			paletteFrame = GFX.PaletteFrame4;
			pal = 0;
		}

		//printf ("  TILE addr:%x pal:%d %d\n", TileAddr, pal, texturePos);
		//if (screenOffset == 0)
		//	printf ("  %d %d %d %d\n", startPalette, pal, paletteFrame[pal + startPalette / 16], GFX.VRAMPaletteFrame[TileAddr][pal]);

		if (GFX.VRAMPaletteFrame[TileAddr][pal] != paletteFrame[pal + startPalette / 16])
		{
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
			GFX.VRAMPaletteFrame[TileAddr][pal] = paletteFrame[pal + startPalette / 16];

			//if (screenOffset == 0)
			//	printf ("cache %d\n", texturePos);
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
		}
    }
	

	// Render tile
	//
	screenOffset += startX;
	int x0 = screenOffset & 0xFF;
	int y0 = (screenOffset >> 8) + BG.Depth;
	int x1 = x0 + width;
	int y1 = y0 + height;

	int tx0 = startX;
	int ty0 = startLine >> 3;
	int tx1 = tx0 + width;
	int ty1 = ty0 + height;

	gpu3dsAddTileVertexes(
		x0, y0, x1, y1,
		tx0, ty0,
		tx1, ty1, (snesTile & (H_FLIP | V_FLIP)) + texturePos);
		
}


//-------------------------------------------------------------------
// Draw tile using 3D hardware
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawBGTileHardwareInline (
    int tileSize, int tileShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
    uint32 snesTile, uint32 screenOffset, uint32 startLine, uint32 height)
{
	S9xDrawBGClippedTileHardwareInline (
        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
        snesTile, screenOffset, 0, 8, startLine, height);
}



//-------------------------------------------------------------------
// Draw a full tile 8xh tile using 3D hardware
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawBGFullTileHardwareInline (
    int tileSize, int tileShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
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

        GFX.VRAMPaletteFrame[TileAddr][0] = 0;
        GFX.VRAMPaletteFrame[TileAddr][1] = 0;
        GFX.VRAMPaletteFrame[TileAddr][2] = 0;
        GFX.VRAMPaletteFrame[TileAddr][3] = 0;
        GFX.VRAMPaletteFrame[TileAddr][4] = 0;
        GFX.VRAMPaletteFrame[TileAddr][5] = 0;
        GFX.VRAMPaletteFrame[TileAddr][6] = 0;
        GFX.VRAMPaletteFrame[TileAddr][7] = 0;
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
		texturePos = cacheGetTexturePositionFast(TileAddr, pal);

        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + startPalette / 16])
        {
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + startPalette / 16];

			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        }
    }
    else
    {
        pal = (snesTile >> 10) & paletteMask;
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << paletteShift) + startPalette];
		texturePos = cacheGetTexturePositionFast(TileAddr, pal);
		//printf ("%d\n", texturePos);

		uint32 *paletteFrame = GFX.PaletteFrame;
		if (paletteShift == 2)
			paletteFrame = GFX.PaletteFrame4;
		else if (paletteShift == 0)
		{
			paletteFrame = GFX.PaletteFrame256;
			pal = 0;
		}

		//if (screenOffset == 0)
		//	printf ("  %d %d %d %d\n", startPalette, pal, paletteFrame[pal + startPalette / 16], GFX.VRAMPaletteFrame[TileAddr][pal]);

		if (GFX.VRAMPaletteFrame[TileAddr][pal] != paletteFrame[pal + startPalette / 16])
		{
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
			GFX.VRAMPaletteFrame[TileAddr][pal] = paletteFrame[pal + startPalette / 16];

			//if (screenOffset == 0)
			//	printf ("cache %d\n", texturePos);
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
		}
    }
	

	// Render tile
	//
	int x0 = screenX;
	int y0 = screenY + BG.Depth;
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

        GFX.VRAMPaletteFrame[TileAddr][0] = 0;
        GFX.VRAMPaletteFrame[TileAddr][1] = 0;
        GFX.VRAMPaletteFrame[TileAddr][2] = 0;
        GFX.VRAMPaletteFrame[TileAddr][3] = 0;
        GFX.VRAMPaletteFrame[TileAddr][4] = 0;
        GFX.VRAMPaletteFrame[TileAddr][5] = 0;
        GFX.VRAMPaletteFrame[TileAddr][6] = 0;
        GFX.VRAMPaletteFrame[TileAddr][7] = 0;
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
		texturePos = cacheGetTexturePositionFast(TileAddr, pal);

        if (GFX.VRAMPaletteFrame[TileAddr][pal] != GFX.PaletteFrame[pal + startPalette / 16])
        {
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
            GFX.VRAMPaletteFrame[TileAddr][pal] = GFX.PaletteFrame[pal + startPalette / 16];

			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        }
    }
    else
    {
        pal = (snesTile >> 10) & paletteMask;
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << paletteShift) + startPalette];
		texturePos = cacheGetTexturePositionFast(TileAddr, pal);
		//printf ("%d\n", texturePos);

		uint32 *paletteFrame = GFX.PaletteFrame;
		if (paletteShift == 2)
			paletteFrame = GFX.PaletteFrame4;
		else if (paletteShift == 0)
		{
			paletteFrame = GFX.PaletteFrame256;
			pal = 0;
		}

		//if (screenOffset == 0)
		//	printf ("  %d %d %d %d\n", startPalette, pal, paletteFrame[pal + startPalette / 16], GFX.VRAMPaletteFrame[TileAddr][pal]);

		if (GFX.VRAMPaletteFrame[TileAddr][pal] != paletteFrame[pal + startPalette / 16])
		{
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
			GFX.VRAMPaletteFrame[TileAddr][pal] = paletteFrame[pal + startPalette / 16];

			//if (screenOffset == 0)
			//	printf ("cache %d\n", texturePos);
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
		}
    }
	

	// Render tile
	//
	if (!IPPU.Interlace)
	{
		int x0 = screenX >> 1;
		int y0 = screenY + BG.Depth;
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
		int y0 = screenY + BG.Depth;
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
}



//-------------------------------------------------------------------
// Draw offset-per-tile background.
//-------------------------------------------------------------------

inline void __attribute__((always_inline)) S9xDrawOffsetBackgroundHardwarePriority0Inline (
    int tileSize, int tileShift, int bitShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
	uint32 BGMode, uint32 bg, bool sub, int depth)
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
    uint32 Width;
    int VOffsetOffset = BGMode == 4 ? 0 : 32;

	S9xComputeAndEnableStencilFunction(bg, sub);

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
	BG.Depth = depth;
	
	BG.DrawTileCount[bg] = 0;
	
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


    static const int Lines = 1;
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
	
    for (uint32 Y = GFX.StartY; Y <= GFX.EndY; Y++)
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
		
		int clipcount = GFX.pCurrentClip->Count [bg];
		if (!clipcount)
			clipcount = 1;
		
		for (int clip = 0; clip < clipcount; clip++)
		{
			uint32 Left;
			uint32 Right;
			
			if (!GFX.pCurrentClip->Count [bg])
			{
				Left = 0;
				Right = 256;
			}
			else
			{
				Left = GFX.pCurrentClip->Left [clip][bg];
				Right = GFX.pCurrentClip->Right [clip][bg];
				
				if (Right <= Left)
					continue;
			}
			
			uint32 VOffset;
			uint32 HOffset;
			//added:
			uint32 LineHOffset=LineData [Y].BG[bg].HOffset;
			
			uint32 Offset;
			uint32 HPos;
			uint32 Quot;
			uint32 Count;
			uint16 *t;
			uint32 Quot2;
			uint32 VCellOffset;
			uint32 HCellOffset;
			uint16 *b1;
			uint16 *b2;
			uint32 TotalCount = 0;
			uint32 MaxCount = 8;
			
			uint32 s = Left * GFX.PixSize + Y * 256;
			bool8 left_hand_edge = (Left == 0);
			Width = Right - Left;
			
			if (Left & 7)
				MaxCount = 8 - (Left & 7);
			
			while (Left < Right) 
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
				
				if (MaxCount + TotalCount > Width)
					MaxCount = Width - TotalCount;
				
				Offset = HPos & 7;
				
				//Count =1;
				Count = 8 - Offset;
				if (Count > MaxCount)
					Count = MaxCount;
				
				s -= Offset * GFX.PixSize;
				Tile = READ_2BYTES(t);

				int tpriority = (Tile & 0x2000) >> 13;
				
				if (tileSize == 8)
				{
					if (tpriority == 0)
						S9xDrawBGClippedTileHardwareInline (
							tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
							Tile, s, Offset, Count, VirtAlign, Lines);
					else
						DrawClippedTileLater (Tile, s, Offset, Count, VirtAlign, Lines);
				}
				else
				{
					if (!(Tile & (V_FLIP | H_FLIP)))
					{
						if (tpriority == 0)
							S9xDrawBGClippedTileHardwareInline (
								tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
								Tile + t1 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
						else
							DrawClippedTileLater (Tile + t1 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
					}
					else
						if (Tile & H_FLIP)
						{
							if (Tile & V_FLIP)
							{
								if (tpriority == 0)
									S9xDrawBGClippedTileHardwareInline (
										tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
										Tile + t2 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
								else
									DrawClippedTileLater (Tile + t2 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
							}
							else
							{
								if (tpriority == 0)
									S9xDrawBGClippedTileHardwareInline (
										tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
										Tile + t1 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
								else
									DrawClippedTileLater (Tile + t1 + 1 - (Quot & 1), s, Offset, Count, VirtAlign, Lines);
							}
						}
						else
						{
							if (tpriority == 0)
								S9xDrawBGClippedTileHardwareInline (
									tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
									Tile + t2 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
							else
								DrawClippedTileLater (Tile + t2 + (Quot & 1), s, Offset, Count, VirtAlign, Lines);
						}
				}
				
				Left += Count;
				TotalCount += Count;
				s += (Offset + Count) * GFX.PixSize;
				MaxCount = 8;
			}
		}
    }

	gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsDrawVertexes();
}


//-------------------------------------------------------------------
// 4-color offset-per tile BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color_8x8(
            BGMode, bg, sub, depth);
    }
    else
    {
        S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color_16x16(
            BGMode, bg, sub, depth);
    }
}

//-------------------------------------------------------------------
// 16-color offset-per tile BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        8,              // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        16,             // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color_8x8(
            BGMode, bg, sub, depth);
    }
    else
    {
        S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color_16x16(
            BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// 256-color offset-per tile BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawOffsetBackgroundHardwarePriority0Inline_256NormalColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        8,              // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_256NormalColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        16,             // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_256DirectColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        8,              // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_256DirectColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawOffsetBackgroundHardwarePriority0Inline(
        16,             // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawOffsetBackgroundHardwarePriority0Inline_256Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        if (!(GFX.r2130 & 1))
            S9xDrawOffsetBackgroundHardwarePriority0Inline_256NormalColor_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawOffsetBackgroundHardwarePriority0Inline_256DirectColor_8x8(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (!(GFX.r2130 & 1))
            S9xDrawOffsetBackgroundHardwarePriority0Inline_256NormalColor_16x16(
                BGMode, bg, sub, depth);
        else
            S9xDrawOffsetBackgroundHardwarePriority0Inline_256DirectColor_16x16(
                BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// Draw all priority 1 tiles.
//-------------------------------------------------------------------

inline void __attribute__((always_inline)) S9xDrawBackgroundHardwarePriority1Inline (
    int tileSize, int tileShift, int bitShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
    uint32 BGMode, uint32 bg, bool sub, int depth)
{
    GFX.PixSize = 1;

	S9xComputeAndEnableStencilFunction(bg, sub);

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
	BG.Depth = depth;

	curBG = bg;

    if (BGMode == 0)
		BG.StartPalette = startPalette;
    else BG.StartPalette = 0;

	for (int i = 0; i < BG.DrawTileCount[bg]; i++)
	{
		if (BG.DrawTileParameters[bg][i][0] == 0)
		{
			// unclipped tile.
			S9xDrawBGTileHardwareInline (
                tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
				BG.DrawTileParameters[bg][i][1], BG.DrawTileParameters[bg][i][2],
				BG.DrawTileParameters[bg][i][3], BG.DrawTileParameters[bg][i][4]);
		}
		else if (BG.DrawTileParameters[bg][i][0] == 1)
		{
			// clipped tile.
			S9xDrawBGClippedTileHardwareInline (
                tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
				BG.DrawTileParameters[bg][i][1], BG.DrawTileParameters[bg][i][2], BG.DrawTileParameters[bg][i][3],
				BG.DrawTileParameters[bg][i][4], BG.DrawTileParameters[bg][i][5], BG.DrawTileParameters[bg][i][6]);
		}
		else if (BG.DrawTileParameters[bg][i][0] == 2)
		{
			// clipped tile.
			S9xDrawBGFullTileHardwareInline (
                tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
				BG.DrawTileParameters[bg][i][1], BG.DrawTileParameters[bg][i][2], BG.DrawTileParameters[bg][i][3],
				BG.DrawTileParameters[bg][i][4], BG.DrawTileParameters[bg][i][5]);
		}
	}

	gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsDrawVertexes();
}

//-------------------------------------------------------------------
// Draw non-offset-per-tile backgrounds
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawBackgroundHardwarePriority0Inline (
    int tileSize, int tileShift, int bitShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
    uint32 BGMode, uint32 bg, bool sub, int depth)
{
    GFX.PixSize = 1;

	S9xComputeAndEnableStencilFunction(bg, sub);

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
	BG.Depth = depth;

	BG.DrawTileCount[bg] = 0;

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
			uint32 s = Left * GFX.PixSize + Y * 256;		// Once hardcoded, Hires mode no longer supported.
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
								if (tpriority == 0)
                                    S9xDrawBGFullTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t2 + 1 - (Quot & 1), sX, sY, VirtAlign, Lines);
								else
									DrawFullTileLater (Tile + t2 + 1 - (Quot & 1), sX, sY, VirtAlign, Lines);
							}
							else
							{
								// Horizontal flip only
								if (tpriority == 0)
                                    S9xDrawBGFullTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t1 + 1 - (Quot & 1), sX, sY, VirtAlign, Lines);
								else
									DrawFullTileLater (Tile + t1 + 1 - (Quot & 1), sX, sY, VirtAlign, Lines);
							}
						}
						else
						{
							// No horizontal flip, but is there a vertical flip ?
							if (Tile & V_FLIP)
							{
								// Vertical flip only
								if (tpriority == 0)
                                    S9xDrawBGFullTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t2 + (Quot & 1), sX, sY, VirtAlign, Lines);
								else
									DrawFullTileLater (Tile + t2 + (Quot & 1), sX, sY, VirtAlign, Lines);
							}
							else
							{
								// Normal unflipped
								if (tpriority == 0)
                                    S9xDrawBGFullTileHardwareInline (
                                        tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                        Tile + t1 + (Quot & 1), sX, sY, VirtAlign, Lines);
								else
									DrawFullTileLater (Tile + t1 + (Quot & 1), sX, sY, VirtAlign, Lines);
							}
						}
					}
					else
					{
						if (tpriority == 0)
							S9xDrawBGFullTileHardwareInline (
                                tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
                                Tile, sX, sY, VirtAlign, Lines);
						else
							DrawFullTileLater (Tile, sX, sY, VirtAlign, Lines);
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
	gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsDrawVertexes();
}


//-------------------------------------------------------------------
// 4-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority0Inline_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}


void S9xDrawBackgroundHardwarePriority0Inline_4Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGMode != 0)
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawBackgroundHardwarePriority0Inline_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority0Inline_4Color_16x16(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority0Inline_Mode0_4Color_16x16(
                BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// 4-color BGs, priority 1
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority1Inline_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}


void S9xDrawBackgroundHardwarePriority1Inline_Mode0_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_Mode0_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}


void S9xDrawBackgroundHardwarePriority1Inline_4Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGMode != 0)
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawBackgroundHardwarePriority1Inline_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority1Inline_4Color_16x16(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawBackgroundHardwarePriority1Inline_Mode0_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority1Inline_Mode0_4Color_16x16(
                BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// 16-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority0Inline_16Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_16Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_16Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawBackgroundHardwarePriority0Inline_16Color_8x8(
            BGMode, bg, sub, depth);
    }
    else
    {
        S9xDrawBackgroundHardwarePriority0Inline_16Color_16x16(
            BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// 16-color BGs, priority 1
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority1Inline_16Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        8,              // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_16Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        16,             // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_16Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{

    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawBackgroundHardwarePriority1Inline_16Color_8x8(
            BGMode, bg, sub, depth);
    }
    else
    {
        S9xDrawBackgroundHardwarePriority1Inline_16Color_16x16(
            BGMode, bg, sub, depth);
    }
}



//-------------------------------------------------------------------
// 256-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        8,              // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority0Inline(
        16,             // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority0Inline_256Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        if (!(GFX.r2130 & 1))
            S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_8x8(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (!(GFX.r2130 & 1))
            S9xDrawBackgroundHardwarePriority0Inline_256NormalColor_16x16(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority0Inline_256DirectColor_16x16(
                BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// 256-color BGs, priority 1
//-------------------------------------------------------------------

void S9xDrawBackgroundHardwarePriority1Inline_256NormalColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        8,              // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_256NormalColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        16,             // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_256DirectColor_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        8,              // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_256DirectColor_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawBackgroundHardwarePriority1Inline(
        16,             // tileSize
        6,              // tileShift
		8,				// bitShift
        0,              // paletteShift
        0,              // paletteMask
        0,              // startPalette
        TRUE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawBackgroundHardwarePriority1Inline_256Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        if (GFX.r2130 & 1)
            S9xDrawBackgroundHardwarePriority1Inline_256NormalColor_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority1Inline_256DirectColor_8x8(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (GFX.r2130 & 1)
            S9xDrawBackgroundHardwarePriority1Inline_256NormalColor_16x16(
                BGMode, bg, sub, depth);
        else
            S9xDrawBackgroundHardwarePriority1Inline_256DirectColor_16x16(
                BGMode, bg, sub, depth);
    }
}



//-------------------------------------------------------------------
// Draw all hires priority 1 tiles.
//-------------------------------------------------------------------

inline void __attribute__((always_inline)) S9xDrawHiresBackgroundHardwarePriority1Inline (
    int tileSize, int tileShift, int bitShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
    uint32 BGMode, uint32 bg, bool sub, int depth)
{
    GFX.PixSize = 1;

	S9xComputeAndEnableStencilFunction(bg, sub);

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
	BG.Depth = depth;

	curBG = bg;

    if (BGMode == 0)
		BG.StartPalette = startPalette;
    else BG.StartPalette = 0;

	for (int i = 0; i < BG.DrawTileCount[bg]; i++)
	{
		// clipped tile.
		S9xDrawHiresBGFullTileHardwareInline (
			tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
			BG.DrawTileParameters[bg][i][1], BG.DrawTileParameters[bg][i][2], BG.DrawTileParameters[bg][i][3],
			BG.DrawTileParameters[bg][i][4], BG.DrawTileParameters[bg][i][5]);
	}

	gpu3dsBindTextureSnesTileCacheForHires(GPU_TEXUNIT0);
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsDrawVertexes();
}


//-------------------------------------------------------------------
// Draw hires backgrounds
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawHiresBackgroundHardwarePriority0Inline (
    int tileSize, int tileShift, int bitShift, int paletteShift, int paletteMask, int startPalette, bool directColourMode,
    uint32 BGMode, uint32 bg, bool sub, int depth)
{
    GFX.PixSize = 1;

	S9xComputeAndEnableStencilFunction(bg, sub);

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
	BG.Depth = depth;

	BG.DrawTileCount[bg] = 0;

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
							if (tpriority == 0)
								S9xDrawHiresBGFullTileHardwareInline (
									tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
									Tile + (Quot & 1), sX, sY, VirtAlign, actualLines);
							else
									DrawFullTileLater (Tile + (Quot & 1), sX, sY, VirtAlign, actualLines);
							
							// Normal, unflipped
							//(*DrawHiResTilePtr) (Tile + (Quot & 1),
							//	s, VirtAlign, Lines);
						}
						else
						{
							if (tpriority == 0)
								S9xDrawHiresBGFullTileHardwareInline (
									tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
									Tile + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);
							else
									DrawFullTileLater (Tile + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);

							// H flip
							//(*DrawHiResTilePtr) (Tile + 1 - (Quot & 1),
							//	s, VirtAlign, Lines);
						}
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							if (tpriority == 0)
								S9xDrawHiresBGFullTileHardwareInline (
									tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
									Tile + t1 + (Quot & 1), sX, sY, VirtAlign, actualLines);
							else
									DrawFullTileLater (Tile + t1 + (Quot & 1), sX, sY, VirtAlign, actualLines);
							
							// Normal, unflipped
							//(*DrawHiResTilePtr) (Tile + t1 + (Quot & 1),
							//	s, VirtAlign, Lines);
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									if (tpriority == 0)
										S9xDrawHiresBGFullTileHardwareInline (
											tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
											Tile + t2 + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);
									else
											DrawFullTileLater (Tile + t2 + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);
									
									// H & V flip
									//(*DrawHiResTilePtr) (Tile + t2 + 1 - (Quot & 1),
									//	s, VirtAlign, Lines);
								}
								else
								{
									if (tpriority == 0)
										S9xDrawHiresBGFullTileHardwareInline (
											tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
											Tile + t1 + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);
									else
											DrawFullTileLater (Tile + t1 + 1 - (Quot & 1), sX, sY, VirtAlign, actualLines);

									// H flip only
									//(*DrawHiResTilePtr) (Tile + t1 + 1 - (Quot & 1),
									//	s, VirtAlign, Lines);
								}
							}
							else
							{
								if (tpriority == 0)
									S9xDrawHiresBGFullTileHardwareInline (
										tileSize, tileShift, paletteShift, paletteMask, startPalette, directColourMode,
										Tile + t2 + (Quot & 1), sX, sY, VirtAlign, actualLines);
								else
										DrawFullTileLater (Tile + t2 + (Quot & 1), sX, sY, VirtAlign, actualLines);
								
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
	gpu3dsBindTextureSnesTileCacheForHires(GPU_TEXUNIT0);
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsDrawVertexes();
}




//-------------------------------------------------------------------
// 4-color BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawHiresBackgroundHardwarePriority0Inline_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawHiresBackgroundHardwarePriority0Inline_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawHiresBackgroundHardwarePriority0Inline_Mode0_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawHiresBackgroundHardwarePriority0Inline_Mode0_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}


void S9xDrawHiresBackgroundHardwarePriority0Inline_4Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGMode != 0)
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawHiresBackgroundHardwarePriority0Inline_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawHiresBackgroundHardwarePriority0Inline_4Color_16x16(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawHiresBackgroundHardwarePriority0Inline_Mode0_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawHiresBackgroundHardwarePriority0Inline_Mode0_4Color_16x16(
                BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// 4-color BGs, priority 1
//-------------------------------------------------------------------

void S9xDrawHiresBackgroundHardwarePriority1Inline_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority1Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawHiresBackgroundHardwarePriority1Inline_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority1Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}


void S9xDrawHiresBackgroundHardwarePriority1Inline_Mode0_4Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority1Inline(
        8,              // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawHiresBackgroundHardwarePriority1Inline_Mode0_4Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority1Inline(
        16,             // tileSize
        4,              // tileShift
		2,				// bitShift
        2,              // paletteShift
        7,              // paletteMask
        bg << 5,        // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}


void S9xDrawHiresBackgroundHardwarePriority1Inline_4Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGMode != 0)
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawHiresBackgroundHardwarePriority1Inline_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawHiresBackgroundHardwarePriority1Inline_4Color_16x16(
                BGMode, bg, sub, depth);
    }
    else
    {
        if (BGSizes [PPU.BG[bg].BGSize] == 8)
            S9xDrawHiresBackgroundHardwarePriority1Inline_Mode0_4Color_8x8(
                BGMode, bg, sub, depth);
        else
            S9xDrawHiresBackgroundHardwarePriority1Inline_Mode0_4Color_16x16(
                BGMode, bg, sub, depth);
    }
}



//-------------------------------------------------------------------
// 16-color Hires BGs, priority 0
//-------------------------------------------------------------------

void S9xDrawHiresBackgroundHardwarePriority0Inline_16Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        8,              // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawHiresBackgroundHardwarePriority0Inline_16Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority0Inline(
        16,             // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawHiresBackgroundHardwarePriority0Inline_16Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawHiresBackgroundHardwarePriority0Inline_16Color_8x8(
            BGMode, bg, sub, depth);
    }
    else
    {
        S9xDrawHiresBackgroundHardwarePriority0Inline_16Color_16x16(
            BGMode, bg, sub, depth);
    }
}


//-------------------------------------------------------------------
// 16-color Hires BGs, priority 1
//-------------------------------------------------------------------

void S9xDrawHiresBackgroundHardwarePriority1Inline_16Color_8x8
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority1Inline(
        8,              // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawHiresBackgroundHardwarePriority1Inline_16Color_16x16
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{
    S9xDrawHiresBackgroundHardwarePriority1Inline(
        16,             // tileSize
        5,              // tileShift
		4,				// bitShift
        4,              // paletteShift
        7,              // paletteMask
        0,              // startPalette
        FALSE,          // directColourMode
        BGMode, bg, sub, depth);
}

void S9xDrawHiresBackgroundHardwarePriority1Inline_16Color
    (uint32 BGMode, uint32 bg, bool sub, int depth)
{

    if (BGSizes [PPU.BG[bg].BGSize] == 8)
    {
        S9xDrawHiresBackgroundHardwarePriority1Inline_16Color_8x8(
            BGMode, bg, sub, depth);
    }
    else
    {
        S9xDrawHiresBackgroundHardwarePriority1Inline_16Color_16x16(
            BGMode, bg, sub, depth);
    }
}




//-------------------------------------------------------------------
// Draw a clipped OBJ tile using 3D hardware.
//-------------------------------------------------------------------
inline void __attribute__((always_inline)) S9xDrawOBJClippedTileHardware (
	uint32 snesTile, 
	uint32 screenX, uint32 screenY, 
	uint32 startX, uint32 width,
	uint32 textureYOffset, uint32 height)
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

        GFX.VRAMPaletteFrame[TileAddr][8] = 0;
        GFX.VRAMPaletteFrame[TileAddr][9] = 0;
        GFX.VRAMPaletteFrame[TileAddr][10] = 0;
        GFX.VRAMPaletteFrame[TileAddr][11] = 0;
        GFX.VRAMPaletteFrame[TileAddr][12] = 0;
        GFX.VRAMPaletteFrame[TileAddr][13] = 0;
        GFX.VRAMPaletteFrame[TileAddr][14] = 0;
        GFX.VRAMPaletteFrame[TileAddr][15] = 0;
    }

    if (BG.Buffered [TileNumber] == BLANK_TILE)
	    return;

	int texturePos = 0;

    uint32 l;
    uint8 pal;
	{
        pal = (snesTile >> 10) & 7;
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << 4) + 128];
		texturePos = cacheGetTexturePositionFast(TileAddr, pal);
		//printf ("  OBJ  addr:%x pal:%d %d\n", TileAddr, pal, texturePos);
        if (GFX.VRAMPaletteFrame[TileAddr][pal + 8] != GFX.PaletteFrame[pal + 8])
        {
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal);
            GFX.VRAMPaletteFrame[TileAddr][pal + 8] = GFX.PaletteFrame[pal + 8];

			//printf ("cache %d\n", texturePos);
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        }
    }

	// Render tile
	//
	int x0 = screenX + startX;
	int y0 = screenY + (pal <= 3 ? 0x4000 : BG.Depth);
	int x1 = x0 + width;
	int y1 = y0 + 1;

	int tx0 = startX;
	int ty0 = textureYOffset;
	int tx1 = tx0 + width;
	int ty1 = ty0 + 1;

	//printf ("Draw: %d %d %d, %d %d %d %d - %d %d %d %d (%d)\n", screenOffset, startX, startLine, x0, y0, x1, y1, tx0, ty0, tx1, ty1, texturePos);
	gpu3dsAddTileVertexes(
		x0, y0, x1, y1,
		tx0, ty0,
		tx1, ty1, (snesTile & (V_FLIP | H_FLIP)) + texturePos);
}


inline void __attribute__((always_inline)) S9xDrawOBJTileHardware2 (
	uint32 snesTile,
	int screenX, int screenY, uint32 textureYOffset)
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

        GFX.VRAMPaletteFrame[TileAddr][8] = 0;
        GFX.VRAMPaletteFrame[TileAddr][9] = 0;
        GFX.VRAMPaletteFrame[TileAddr][10] = 0;
        GFX.VRAMPaletteFrame[TileAddr][11] = 0;
        GFX.VRAMPaletteFrame[TileAddr][12] = 0;
        GFX.VRAMPaletteFrame[TileAddr][13] = 0;
        GFX.VRAMPaletteFrame[TileAddr][14] = 0;
        GFX.VRAMPaletteFrame[TileAddr][15] = 0;
    }

    if (BG.Buffered [TileNumber] == BLANK_TILE)
	    return;

	int texturePos = 0;

    uint32 l;
    uint8 pal;

	{
        pal = (snesTile >> 10) & 7;
        GFX.ScreenColors = &IPPU.ScreenColors [(pal << 4) + 128];
		texturePos = cacheGetTexturePositionFast(TileAddr, pal + 8);
		//printf ("%d\n", texturePos);
        if (GFX.VRAMPaletteFrame[TileAddr][pal + 8] != GFX.PaletteFrame[pal + 8])
        {
			texturePos = cacheGetSwapTexturePositionForAltFrameFast(TileAddr, pal + 8);
            GFX.VRAMPaletteFrame[TileAddr][pal + 8] = GFX.PaletteFrame[pal + 8];

			//printf ("cache %d\n", texturePos);
			gpu3dsCacheToTexturePosition(pCache, GFX.ScreenColors, texturePos);
        }
    }

	// Render tile
	//
	int x0 = screenX;
	int y0 = screenY + (pal <= 3 ? 0x4000 : BG.Depth);
	int x1 = x0 + 8;
	int y1 = y0 + 1;

	int tx0 = 0;
	int ty0 = textureYOffset;
	int tx1 = tx0 + 8;
	int ty1 = ty0 + 1;

	//printf ("Draw: %d %d %d, %d %d %d %d - %d %d %d %d (%d)\n", screenOffset, startX, startY, x0, y0, x1, y1, txBase + tx0, tyBase + ty0, txBase + tx1, tyBase + ty1, texturePos);
	gpu3dsAddTileVertexes(
		x0, y0, x1, y1,
		tx0, ty0,
		tx1, ty1, (snesTile & (V_FLIP | H_FLIP)) + texturePos);
}



//-------------------------------------------------------------------
// Draw the OBJ layers using 3D hardware.
//-------------------------------------------------------------------
void S9xDrawOBJSHardwarePriority (bool8 sub, int depth = 0, int priority = 0)
{
#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Entering DrawOBJS() for %d-%d\n", GFX.StartY, GFX.EndY);
#endif
	CHECK_SOUND();

	//if (Settings.HWOBJRenderingMode == 0)
	S9xComputeAndEnableStencilFunction(4, sub);

	int p = priority;

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

	//printf ("OBJ p%d count = %d\n", p, BG.DrawOBJTileLaterIndexCount[p]);
	for (int i = 0; i < BG.DrawOBJTileLaterIndexCount[p]; i++)
	{
		int index = BG.DrawOBJTileLaterIndex[p][i];
		//if (BG.DrawOBJTileLaterParameters[index][0] == 0)
		{
			S9xDrawOBJTileHardware2 (
				BG.DrawOBJTileLaterParameters[index][1], BG.DrawOBJTileLaterParameters[index][2], BG.DrawOBJTileLaterParameters[index][3],
				BG.DrawOBJTileLaterParameters[index][4]);
		}
		/*else if (BG.DrawOBJTileLaterParameters[index][0] == 1)
		{
			// clipped tile.
			//printf ("clip OBJ: %d %d %d %d %d %d\n", BG.DrawOBJTileParameters[p][i][1], BG.DrawOBJTileParameters[p][i][2],
			//	BG.DrawOBJTileParameters[p][i][3], BG.DrawOBJTileParameters[p][i][4],
			//	BG.DrawOBJTileParameters[p][i][5], BG.DrawOBJTileParameters[p][i][6]);

			S9xDrawOBJClippedTileHardware (
				BG.DrawOBJTileLaterParameters[index][1], BG.DrawOBJTileLaterParameters[index][2], BG.DrawOBJTileLaterParameters[index][3],
				BG.DrawOBJTileLaterParameters[index][4], BG.DrawOBJTileLaterParameters[index][5], BG.DrawOBJTileLaterParameters[index][6], 
				BG.DrawOBJTileLaterParameters[index][7]);
		}*/
	}

	gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsDrawVertexes();
}


//-------------------------------------------------------------------
// Draw the OBJ layers using 3D hardware.
//-------------------------------------------------------------------
void S9xDrawOBJSHardware (bool8 sub, int depth = 0, int priority = 0)
{
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
					DrawOBJTileLater (PPU.OBJ[S].Priority, BaseTile|TileX, X, Y, TileLine);

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





//---------------------------------------------------------------------------
// Update one of the 256 mode 7 tiles with the latest texture.
// (uses a 256 color palette)
//---------------------------------------------------------------------------
void S9xPrepareMode7UpdateCharTile(int tileNumber)
{
	uint8 *charMap = &Memory.VRAM[1];	
	gpu3dsCacheToMode7TexturePosition( 
		&charMap[tileNumber * 128], GFX.ScreenColors, tileNumber, &IPPU.Mode7CharPaletteMask[tileNumber]); 
}


//---------------------------------------------------------------------------
// Update one of the 256 mode 7 tiles with the latest texture.
// (uses a 128 color palette)
//---------------------------------------------------------------------------
void S9xPrepareMode7ExtBGUpdateCharTile(int tileNumber)
{
	uint8 *charMap = &Memory.VRAM[1];	
	gpu3dsCacheToMode7TexturePosition( \
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
	if (!(Memory.FillRAM [0x2133] & 0x40))
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
		for (int i = 0; i < 128; i++)
			GFX.ScreenColors128[i] = GFX.ScreenColors[i];
		for (int i = 0; i < 128; i++)
			GFX.ScreenColors128[i + 128] = GFX.ScreenColors[i];

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
	gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT0);
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


//---------------------------------------------------------------------------
// Draws the Mode 7 background.
//---------------------------------------------------------------------------
void S9xDrawBackgroundMode7Hardware(int bg, bool8 sub, int depth)
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
			uint32 m7Left;
			uint32 m7Right;

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
 
 			#define CLIP_10_BIT_SIGNED(a)  (((a) << 19) >> 19)
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
			//	printf ("Y=%d T=%d,%d %d,%d\n", Y, tx0 / 8, ty0 / 8, tx1 / 8, ty1 / 8);

			//gpu3dsAddMode7ScanlineVertexes(Left, Y+depth, Right, Y+1+depth, tx0, ty0, tx1, ty1, 0);
			gpu3dsAddMode7LineVertexes(Left, Y+depth, Right, Y+1+depth, tx0, ty0, tx1, ty1);
		}
	}

	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsDrawMode7LineVertexes();
	gpu3dsDrawVertexes();
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
 
 			#define CLIP_10_BIT_SIGNED(a)  (((a) << 19) >> 19)
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
			//	printf ("Y=%d T=%d,%d %d,%d\n", Y, tx0 / 8, ty0 / 8, tx1 / 8, ty1 / 8);

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

	gpu3dsEnableAlphaTestNotEqualsZero();
	gpu3dsDrawMode7LineVertexes();
	gpu3dsDrawVertexes();
	t3dsEndTiming(27);
}



//-----------------------------------------------------------
// Draw OBJs from OBJ layer to main/sub screen
//-----------------------------------------------------------
void S9xDrawOBJsFromOBJLayerToScreen(bool sub, int priority)
{
	
	if (!sub)
		gpu3dsSetRenderTargetToMainScreenTexture();
	else
		gpu3dsSetRenderTargetToSubScreenTexture();
	
	S9xComputeAndEnableStencilFunction(4, sub);

	gpu3dsSetTextureEnvironmentReplaceTexture0WithFullAlpha();
	gpu3dsBindTextureOBJLayer(GPU_TEXUNIT0);
	gpu3dsDisableDepthTest();
	gpu3dsEnableAlphaTestEquals(priority + 1);
	gpu3dsDisableAlphaBlending();

	int OBDepth = SUB_OR_ADD (4) ? 0 : 0x4000;
	int p = priority;

	
	for (int i = 0; i < BG.DrawOBJTileLaterIndexCount[p]; i++)
	{
		int index = BG.DrawOBJTileLaterIndex[p][i];
		int snesTile = BG.DrawOBJTileLaterParameters[index][1];

        int pal = (snesTile >> 10) & 7;

		// Render tile
		//
		int x = BG.DrawOBJTileLaterParameters[index][2];
		int y = BG.DrawOBJTileLaterParameters[index][3];
		int depth = (pal <= 3) ? 0x4000 : OBDepth;
		//printf ("OBJLayerToScreen: p%d @ %d,%d\n", p, x, y);
		gpu3dsAddTileVertexes (
			x, y + depth, 
			x + 8, y + 8 + depth, 
			x, y, x + 8, y + 8, 0);
	}
	/*	gpu3dsAddTileVertexes (
			0, 0, 256, 256,
			0, 0, 256, 256, 0);*/

	gpu3dsDrawVertexes();
	
	gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
	gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsDisableAlphaTest();
}


//-----------------------------------------------------------
// Draw OBJs to OBJ layer.
//-----------------------------------------------------------
void S9xDrawOBJStoOBJLayer()
{
	if (ON_MAIN (4) || ON_SUB(4))
	{
		BG.DrawOBJTileLaterParametersCount = 0;
		BG.DrawOBJTileLaterIndexCount[0] = 0;
		BG.DrawOBJTileLaterIndexCount[1] = 0;
		BG.DrawOBJTileLaterIndexCount[2] = 0;
		BG.DrawOBJTileLaterIndexCount[3] = 0;
		
		
		gpu3dsSetRenderTargetToOBJLayer();

		// clear to black first.
		gpu3dsSetTextureEnvironmentReplaceColor();
		gpu3dsDisableDepthTest();
		gpu3dsDisableStencilTest();
		gpu3dsDisableAlphaBlending();
		gpu3dsDisableAlphaTest();
		gpu3dsDrawRectangle(0, 0, 256, 240, 0, 0x00000000);

		// render OBJs
		//
		gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
		gpu3dsEnableAlphaTestNotEqualsZero();
		//gpu3dsDisableAlphaTest();
		gpu3dsDisableDepthTest();

		S9xDrawOBJSHardware(0, 0, 0);

		/*
		for (int p = 0; p < 4; p++)
		{
			gpu3dsSetTextureEnvironmentReplaceTexture0WithConstantAlpha(p + 1);
			S9xDrawOBJSHardwarePriority(0, 0, p);
		}
		*/

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
		BG.Depth = 0;

		printf ("OBJ count: %d\n", BG.DrawOBJTileLaterParametersCount);
		int currentPriority = -1;
		for (int i = 0; i < BG.DrawOBJTileLaterParametersCount; i++)
		{
			int priority = BG.DrawOBJTileLaterParameters[i][0]; 
			if (currentPriority != priority)
			{
				printf ("P%d\n", priority);
				gpu3dsSetTextureEnvironmentReplaceTexture0WithConstantAlpha(priority + 1);
				currentPriority = priority;
			}

			//printf ("  %d,%d\n", BG.DrawOBJTileLaterParameters[i][2], BG.DrawOBJTileLaterParameters[i][3]);
			S9xDrawOBJTileHardware2 (
				BG.DrawOBJTileLaterParameters[i][1], BG.DrawOBJTileLaterParameters[i][2], BG.DrawOBJTileLaterParameters[i][3],
				BG.DrawOBJTileLaterParameters[i][4]);
			
		}
		gpu3dsEnableAlphaTestNotEqualsZero();
		gpu3dsDrawVertexes();

	}
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

	int BGDepth0 = 0, BGDepth1 = 0, BGDepth2 = 0, BGDepth3 = 0, OBDepth = 0, BackDepth = 0;

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

		BGDepth0 = SUB_OR_ADD (0) ? 0 : 0x4000;
		BGDepth1 = SUB_OR_ADD (1) ? 0 : 0x4000;
		BGDepth2 = SUB_OR_ADD (2) ? 0 : 0x4000;
		BGDepth3 = SUB_OR_ADD (3) ? 0 : 0x4000;
		OBDepth = SUB_OR_ADD (4) ? 0 : 0x4000;
		BackDepth = SUB_OR_ADD (5) ? 0 : 0x4000;

		//printf ("Math Y:%d BGEnable:%d%d%d%d%d%d %d\n", GFX.StartY, 
		//	SUB_OR_ADD (0) ? 0 : 0, SUB_OR_ADD (1) ? 0 : 0, SUB_OR_ADD (2) ? 0 : 0, SUB_OR_ADD (3) ? 0 : 0, 
		//	SUB_OR_ADD (4) ? 0 : 0, SUB_OR_ADD (5) ? 0 : 0, BackDepth);
    }
    else
    {
		// Sub Screen
		GFX.pCurrentClip = &IPPU.Clip [1];
		BG0 = ON_SUB (0);
		BG1 = ON_SUB (1);
		BG2 = ON_SUB (2);
		BG3 = ON_SUB (3);
		OB  = ON_SUB (4);

		//printf ("Sub  Y:%d BGEnable:%d%d%d%d%d\n", GFX.StartY, BG0, BG1, BG2, BG3, OB);
		
    }

    sub |= force_no_add;

	int depth = 0;

	/*
	#define DRAW_OBJS(p)  \
		if (OB) \
		{ \
			t3dsStartTiming(26, "DrawOBJS"); \
			S9xDrawOBJSHardware (!sub, depth, p); \
			t3dsEndTiming(26); \
		}
		*/

	/*
	#define DRAW_OBJS(p)  \
		if (OB) \
		{ \
			t3dsStartTiming(26, "DrawOBJS"); \
			if (Settings.HWOBJRenderingMode == 0) \
			{ \
				if (p == 0) \
				{ \
					S9xDrawOBJSHardware (sub, OBDepth, p); \
					S9xDrawOBJSHardwarePriority (sub, OBDepth, p); \
				} \
				else \
					S9xDrawOBJSHardwarePriority (sub, OBDepth, p); \
			} \
			else \
			{ \
				S9xDrawOBJsFromOBJLayerToScreen(sub, p); \
			} \
			t3dsEndTiming(26); \
		} 
	*/

	#define DRAW_OBJS(p)  \
		if (OB) \
		{ \
			t3dsStartTiming(26, "DrawOBJS"); \
			if (p == 0) \
			{ \
				S9xDrawOBJSHardware (sub, OBDepth, p); \
				S9xDrawOBJSHardwarePriority (sub, OBDepth, p); \
			} \
			else \
				S9xDrawOBJSHardwarePriority (sub, OBDepth, p); \
			t3dsEndTiming(26); \
		} 


	#define DRAW_4COLOR_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawBackgroundHardwarePriority0Inline_4Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawBackgroundHardwarePriority1Inline_4Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_16COLOR_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawBackgroundHardwarePriority0Inline_16Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawBackgroundHardwarePriority1Inline_16Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_256COLOR_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawBackgroundHardwarePriority0Inline_256Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawBackgroundHardwarePriority1Inline_256Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_4COLOR_OFFSET_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawOffsetBackgroundHardwarePriority0Inline_4Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawBackgroundHardwarePriority1Inline_4Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_16COLOR_OFFSET_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawOffsetBackgroundHardwarePriority0Inline_16Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawBackgroundHardwarePriority1Inline_16Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_256COLOR_OFFSET_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawOffsetBackgroundHardwarePriority0Inline_256Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawBackgroundHardwarePriority1Inline_256Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}

	#define DRAW_4COLOR_HIRES_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawHiresBackgroundHardwarePriority0Inline_4Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawHiresBackgroundHardwarePriority1Inline_4Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}


	#define DRAW_16COLOR_HIRES_BG_INLINE(bg, p) \
		if (BG##bg) \
		{ \
			if (bg == 0) { t3dsStartTiming(21, "DrawBG0"); } \
			if (bg == 1) { t3dsStartTiming(22, "DrawBG1"); } \
			if (bg == 2) { t3dsStartTiming(23, "DrawBG2"); } \
			if (bg == 3) { t3dsStartTiming(24, "DrawBG3"); } \
			if (p == 0) \
				S9xDrawHiresBackgroundHardwarePriority0Inline_16Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			else \
				S9xDrawHiresBackgroundHardwarePriority1Inline_16Color (PPU.BGMode, bg, sub, BGDepth##bg); \
			t3dsEndTiming(21 + bg); \
		}


	// Initialize the draw later parameters
	// (Maybe creating the actual vertexes might be a better idea instead?)
	//
	//if (Settings.HWOBJRenderingMode == 0)
	{
		BG.DrawOBJTileLaterParametersCount = 0;
		BG.DrawOBJTileLaterIndexCount[0] = 0;
		BG.DrawOBJTileLaterIndexCount[1] = 0;
		BG.DrawOBJTileLaterIndexCount[2] = 0;
		BG.DrawOBJTileLaterIndexCount[3] = 0;
	}

	//printf ("BG Enable %d%d%d%d\n", BG0, BG1, BG2, BG3);
	
	
	switch (PPU.BGMode)
	{
		case 0:
			gpu3dsSetTextureEnvironmentReplaceColor();
			S9xDrawBackdropHardware(sub, BackDepth);

			gpu3dsSetTextureEnvironmentReplaceTexture0();
			DRAW_4COLOR_BG_INLINE (3, 0);
			DRAW_4COLOR_BG_INLINE (2, 0);
			DRAW_OBJS(0);

			DRAW_4COLOR_BG_INLINE (3, 1);
			DRAW_4COLOR_BG_INLINE (2, 1);
			DRAW_OBJS(1);

			DRAW_4COLOR_BG_INLINE (1, 0);
			DRAW_4COLOR_BG_INLINE (0, 0);
			DRAW_OBJS(2);

			DRAW_4COLOR_BG_INLINE (1, 1);
			DRAW_4COLOR_BG_INLINE (0, 1);
			DRAW_OBJS(3);
			gpu3dsDrawVertexes();
			break;
		case 1:
			gpu3dsSetTextureEnvironmentReplaceColor();
			S9xDrawBackdropHardware(sub, BackDepth);

			gpu3dsSetTextureEnvironmentReplaceTexture0();
			DRAW_4COLOR_BG_INLINE(2, 0);
			DRAW_OBJS(0);
			if (!PPU.BG3Priority)
			{
				DRAW_4COLOR_BG_INLINE(2, 1);
			}
			DRAW_OBJS(1);
			DRAW_16COLOR_BG_INLINE(1, 0);
			DRAW_16COLOR_BG_INLINE(0, 0);
			DRAW_OBJS(2);
			DRAW_16COLOR_BG_INLINE(1, 1);
			DRAW_16COLOR_BG_INLINE(0, 1);
			DRAW_OBJS(3);
			if (PPU.BG3Priority)
			{
				DRAW_4COLOR_BG_INLINE(2, 1);
			}
			gpu3dsDrawVertexes();

			break;
		case 2:
			gpu3dsSetTextureEnvironmentReplaceColor();
			S9xDrawBackdropHardware(sub, BackDepth);

			gpu3dsSetTextureEnvironmentReplaceTexture0();
			DRAW_16COLOR_OFFSET_BG_INLINE (1, 0);
			DRAW_OBJS(0);

			DRAW_16COLOR_OFFSET_BG_INLINE (0, 0);
			DRAW_OBJS(1);

			DRAW_16COLOR_OFFSET_BG_INLINE (1, 1);
			DRAW_OBJS(2);

			DRAW_16COLOR_OFFSET_BG_INLINE (0, 1);
			DRAW_OBJS(3);

			gpu3dsDrawVertexes();

			break;
		case 3:
			gpu3dsSetTextureEnvironmentReplaceColor();
			S9xDrawBackdropHardware(sub, BackDepth);

			gpu3dsSetTextureEnvironmentReplaceTexture0();
			DRAW_16COLOR_BG_INLINE (1, 0);
			DRAW_OBJS(0);

			DRAW_256COLOR_BG_INLINE (0, 0);
			DRAW_OBJS(1);

			DRAW_16COLOR_BG_INLINE (1, 1);
			DRAW_OBJS(2);

			DRAW_256COLOR_BG_INLINE (0, 1);
			DRAW_OBJS(3);

			gpu3dsDrawVertexes();

			break;
		case 4:
			gpu3dsSetTextureEnvironmentReplaceColor();
			S9xDrawBackdropHardware(sub, BackDepth);

			gpu3dsSetTextureEnvironmentReplaceTexture0();
			DRAW_4COLOR_OFFSET_BG_INLINE (1, 0);
			DRAW_OBJS(0);

			DRAW_256COLOR_OFFSET_BG_INLINE (0, 0);
			DRAW_OBJS(1);

			DRAW_4COLOR_OFFSET_BG_INLINE (1, 1);
			DRAW_OBJS(2);

			DRAW_256COLOR_OFFSET_BG_INLINE (0, 1);
			DRAW_OBJS(3);

			gpu3dsDrawVertexes();

			break;
		case 5:
			gpu3dsSetTextureEnvironmentReplaceColor();
			S9xDrawBackdropHardware(sub, BackDepth);

			gpu3dsSetTextureEnvironmentReplaceTexture0();
			DRAW_4COLOR_HIRES_BG_INLINE (1, 0);
			DRAW_OBJS(0);

			DRAW_16COLOR_HIRES_BG_INLINE (0, 0);
			DRAW_OBJS(1);

			DRAW_4COLOR_HIRES_BG_INLINE (1, 1);
			DRAW_OBJS(2);

			DRAW_16COLOR_HIRES_BG_INLINE (0, 1);
			DRAW_OBJS(3);

			gpu3dsDrawVertexes();

			break;
		case 6:
			gpu3dsSetTextureEnvironmentReplaceColor();
			S9xDrawBackdropHardware(sub, BackDepth);

			gpu3dsSetTextureEnvironmentReplaceTexture0();
			DRAW_OBJS(0);
			DRAW_16COLOR_BG_INLINE (0, 0);
			DRAW_OBJS(1);

			DRAW_OBJS(2);
			DRAW_16COLOR_BG_INLINE (0, 1);
			DRAW_OBJS(3);

			gpu3dsDrawVertexes();

			break;
		case 7:
			// TODO: Mode 7 graphics.
			//
			gpu3dsSetTextureEnvironmentReplaceColor();
			S9xDrawBackdropHardware(sub, BackDepth);

			gpu3dsSetTextureEnvironmentReplaceTexture0();
			S9xPrepareMode7(sub);

			#define DRAW_M7BG(bg) \
				if (BG##bg) \
				{ \
					if (PPU.Mode7Repeat == 0) \
					{ \
						gpu3dsBindTextureSnesMode7FullRepeat(GPU_TEXUNIT0); \
						S9xDrawBackgroundMode7Hardware(bg, sub, BGDepth##bg); \
					} \
					else if (PPU.Mode7Repeat == 2) \
					{ \
						gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT0); \
						S9xDrawBackgroundMode7Hardware(bg, sub, BGDepth##bg); \
					} \
					else \ 
					{ \
						gpu3dsBindTextureSnesMode7Tile0CacheRepeat(GPU_TEXUNIT0); \
						S9xDrawBackgroundMode7HardwareRepeatTile0(bg, sub, BGDepth##bg); \
						gpu3dsBindTextureSnesMode7Full(GPU_TEXUNIT0); \
						S9xDrawBackgroundMode7Hardware(bg, sub, BGDepth##bg); \
					} \
				}

			if ((Memory.FillRAM [0x2133] & 0x40) && !BG0)  
				DRAW_M7BG(1);
			
			gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
			DRAW_OBJS(0);
			gpu3dsDrawVertexes();

			DRAW_M7BG(0);

			gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
			DRAW_OBJS(1);
			DRAW_OBJS(2);
			DRAW_OBJS(3);		
			gpu3dsDrawVertexes();		

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
	
	if (GFX.r2130 & 2)
	{
		// Bug fix: We have to render the subscreen as long either of the
		//          212D and 2131 registers are set for any BGs.
		//
		//			This fixes Zelda's prologue's where the room is supposed to
		//			be dark.
		//
		if (ANYTHING_ON_SUB || ADD_OR_SUB_ON_ANYTHING)
		{
			gpu3dsEnableDepthTest();

			// Subscreen math
			//
			gpu3dsSetTextureEnvironmentReplaceTexture0();
			gpu3dsBindTextureSubScreen(GPU_TEXUNIT0);
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
		gpu3dsEnableDepthTest();

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
		
		for (int i = 0; i < IPPU.FixedColorSections.Count; i++)
		{
			uint32 fixedColour = IPPU.FixedColorSections.Section[i].Value;

			if (fixedColour != 0xff)
			{
				// debugging only
				/*if (GFX.r2131 & 0x80) printf ("  -"); else printf("  +");
				if (GFX.r2131 & 0x40) printf ("/2"); else printf("/1");
				printf (" cmath Y:%d-%d, 2131:%02x, %04x\n", IPPU.FixedColorSections.Section[i].StartY, IPPU.FixedColorSections.Section[i].EndY, GFX.r2131, fixedColour);*/

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
			
			// we only want to clip colors to black, so don't modify the depth
			gpu3dsDisableDepthTestAndWriteColorAlphaOnly();
			gpu3dsSetTextureEnvironmentReplaceColor();
			gpu3dsSetRenderTargetToMainScreenTexture();
			gpu3dsDisableAlphaTest();
			
			gpu3dsAddRectangleVertexes(
				0, GFX.StartY, 256, GFX.EndY + 1, 0, 0xff);
			gpu3dsDrawVertexes();
		}
	}

	if ((GFX.r2130 & 0x30) != 0x30)
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

#ifdef JP_FIX

    GFX.Pseudo = (Memory.FillRAM [0x2133] & 8) != 0 &&
				 (GFX.r212c & 15) != (GFX.r212d & 15) &&
				 (GFX.r2131 == 0x3f);

#else

    GFX.Pseudo = (Memory.FillRAM [0x2133] & 8) != 0 &&
		(GFX.r212c & 15) != (GFX.r212d & 15) &&
		(GFX.r2131 & 0x3f) == 0;

#endif

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


	gpu3dsDisableDepthTest();
	gpu3dsEnableAlphaBlending();

	/*if (Settings.HWOBJRenderingMode == 1)
	{
		S9xDrawOBJStoOBJLayer();

	}*/

	// Bug fix: We have to render as long as 
	// the 2130 register says that we have are
	// doing color math using the subscreen 
	// (instead of the fixed color)
	//
	// This is because the backdrop color will be
	// used for the color math.
	//
	if (ANYTHING_ON_SUB || (GFX.r2130 & 2))
	{
		// debugging only
		//printf ("SS Y:%d-%d M%d\n", GFX.StartY, GFX.EndY, PPU.BGMode);

		// Render the subscreen
		//
		gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
		gpu3dsSetRenderTargetToSubScreenTexture();
		S9xRenderScreenHardware (TRUE, TRUE, SUB_SCREEN_DEPTH);
	}

	// debugging only
	//printf ("MS Y:%d-%d M%d\n", GFX.StartY, GFX.EndY, PPU.BGMode);

	// Render the main screen.
	//
	gpu3dsSetRenderTargetToMainScreenTexture();
	gpu3dsBindTextureSnesTileCache(GPU_TEXUNIT0);
	S9xRenderScreenHardware (FALSE, FALSE, MAIN_SCREEN_DEPTH);

	// Do clip to black + color math here
	//
	S9xRenderClipToBlackAndColorMath();

	// Render the brightness
	//
	S9xRenderBrightness();

	/*
	// For debugging only	
	// 
	gpu3dsDisableStencilTest();
	gpu3dsDisableDepthTest();
	gpu3dsDisableAlphaTest();
	gpu3dsDisableAlphaBlending();
	gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsBindTextureSnesMode7TileCache(GPU_TEXUNIT0);
	gpu3dsSetRenderTargetToMainScreenTexture();
	gpu3dsAddTileVertexes(50, 170, 100, 220, 0, 0, 256, 256, 0);
	gpu3dsDrawVertexes();
	*/
	
	/*
	// For debugging only	
	// 
	gpu3dsDisableStencilTest();
	gpu3dsDisableDepthTest();
	gpu3dsDisableAlphaTest();
	gpu3dsDisableAlphaBlending();
	gpu3dsSetTextureEnvironmentReplaceTexture0();
	gpu3dsSetRenderTargetToMainScreenTexture();
	gpu3dsBindTextureSubScreen(GPU_TEXUNIT0);
	gpu3dsAddTileVertexes(150, 170, 200, 220, 0, 0, 256, 256, 0);
	gpu3dsDrawVertexes();
	gpu3dsBindTextureDepthForScreens(GPU_TEXUNIT0);
	gpu3dsAddTileVertexes(200, 170, 250, 220, 0, 0, 256, 256, 0);
	gpu3dsDrawVertexes();
	*/

	S9xResetVerticalSection(&IPPU.BrightnessSections);
	S9xResetVerticalSection(&IPPU.BackdropColorSections);
	S9xResetVerticalSection(&IPPU.FixedColorSections);
	S9xResetVerticalSection(&IPPU.WindowLRSections);

    IPPU.PreviousLine = IPPU.CurrentLine;
	t3dsEndTiming(11);
}
