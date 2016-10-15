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
#include "display.h"
#include "gfx.h"
#include "tile.h"

//#include "3dsgpu.h"

#ifdef USE_GLIDE
#include "3d.h"
#endif 

extern uint32 HeadMask [4];
extern uint32 TailMask [5];

uint8 ConvertTile (uint8 *pCache, uint32 TileAddr)
{
    //printf ("Tile Addr: %04x\n", TileAddr);
    register uint8 *tp = &Memory.VRAM[TileAddr];
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
	    register uint8 pix;

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
	    register uint8 pix;
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
	    register uint8 pix;
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


uint8 ConvertTileTo16Bit (uint16 *palette, uint8 *tile8Bit, uint16 *tile16Bit)
{
    for (int i = 0; i < 8; i++)
    {
        #define SetPixel16Bit(n) \
            if (tile8Bit[n] != 0) tile16Bit[n] = (palette[tile8Bit[n]] | 1); else tile16Bit[n] = 0;
        
        SetPixel16Bit(0);
        SetPixel16Bit(1);
        SetPixel16Bit(2);
        SetPixel16Bit(3);
        SetPixel16Bit(4);
        SetPixel16Bit(5);
        SetPixel16Bit(6);
        SetPixel16Bit(7);
        
        #undef SetPixel16Bit
        
        tile16Bit += 8;
        tile8Bit += 8;
    }
}


inline void WRITE_4PIXELS (uint32 Offset, uint8 *Pixels)
{
    uint8 Pixel;
    uint8 *Screen = GFX.S + Offset;
    uint8 *Depth = GFX.DB + Offset;

#define FN(N) \
    if ((Pixel = Pixels[N]) && GFX.Z1 > Depth [N]) \
    { \
	TILE_SetPixel(N, Pixel); \
    }

/*
    uint32 FourPixels = *((uint32 *)Pixels);
    if (FourPixels == 0)
        return;
        
#define FN(N) \
    if (Pixel = ((FourPixels >> (N*4)) & 0xff)) \
    { \
	TILE_SetPixel(N, Pixel); \
    }
    */
    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}

inline void WRITE_4PIXELS_FLIPPED (uint32 Offset, uint8 *Pixels)
{
    uint8 Pixel;
    uint8 *Screen = GFX.S + Offset;
    uint8 *Depth = GFX.DB + Offset;


#define FN(N) \
    if ((Pixel = Pixels[3 - N]) && GFX.Z1 > Depth [N]) \
    { \
	TILE_SetPixel(N, Pixel); \
    }
/*
    uint32 FourPixels = *((uint32 *)Pixels);
    if (FourPixels == 0)
        return;
        
#define FN(N) \
    if (Pixel = ((FourPixels >> ((4-N)*4)) & 0xff)) \
    { \
	TILE_SetPixel(N, Pixel); \
    }
    */
    
    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}

inline void WRITE_4PIXELSx2 (uint32 Offset, uint8 *Pixels)
{
    uint8 Pixel;
    uint8 *Screen = GFX.S + Offset;
    uint8 *Depth = GFX.DB + Offset;

#define FN(N) \
    if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[N])) \
    { \
	TILE_SetPixel(N*2+1, Pixel); \
	TILE_SetPixel(N*2+1, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}

inline void WRITE_4PIXELS_FLIPPEDx2 (uint32 Offset, uint8 *Pixels)
{
    uint8 Pixel;
    uint8 *Screen = GFX.S + Offset;
    uint8 *Depth = GFX.DB + Offset;

#define FN(N) \
    if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[3 - N])) \
    { \
	TILE_SetPixel(N*2+1, Pixel); \
	TILE_SetPixel(N*2+1, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}

inline void WRITE_4PIXELSx2x2 (uint32 Offset, uint8 *Pixels)
{
    uint8 Pixel;
    uint8 *Screen = GFX.S + Offset;
    uint8 *Depth = GFX.DB + Offset;

#define FN(N) \
    if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[N])) \
    { \
	TILE_SetPixel(N*2, Pixel); \
	TILE_SetPixel(N*2+1, Pixel); \
	TILE_SetPixel(GFX.RealPitch+N*2, Pixel); \
	TILE_SetPixel(GFX.RealPitch+N*2+1, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}

inline void WRITE_4PIXELS_FLIPPEDx2x2 (uint32 Offset, uint8 *Pixels)
{
    uint8 Pixel;
    uint8 *Screen = GFX.S + Offset;
    uint8 *Depth = GFX.DB + Offset;

#define FN(N) \
    if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[3 - N])) \
    { \
	TILE_SetPixel(N*2, Pixel); \
	TILE_SetPixel(N*2+1, Pixel); \
	TILE_SetPixel(GFX.RealPitch+N*2, Pixel); \
	TILE_SetPixel(GFX.RealPitch+N*2+1, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}


#define WRITE_8PIXELS(n) \
    void Write8Pixels_##n(uint32 Offset, uint8 *Pixels) \
    { \
        uint32 Pixel; \
        uint16 *Screen = (uint16 *) GFX.S + Offset; \
        if (n & 0x01) { TILE_SetPixel16(0, Pixels[0]); } \
        if (n & 0x02) { TILE_SetPixel16(1, Pixels[1]); } \
        if (n & 0x04) { TILE_SetPixel16(2, Pixels[2]); } \
        if (n & 0x08) { TILE_SetPixel16(3, Pixels[3]); } \
        if (n & 0x10) { TILE_SetPixel16(4, Pixels[4]); } \
        if (n & 0x20) { TILE_SetPixel16(5, Pixels[5]); } \
        if (n & 0x40) { TILE_SetPixel16(6, Pixels[6]); } \
        if (n & 0x80) { TILE_SetPixel16(7, Pixels[7]); } \
    } 

#define WRITE_8PIXELS_FLIPPED(n) \
    void Write8PixelsFlipped_##n(uint32 Offset, uint8 *Pixels) \
    { \
        uint32 Pixel; \
        uint16 *Screen = (uint16 *) GFX.S + Offset; \
        if (n & 0x01) { TILE_SetPixel16(0, Pixels[7]); } \
        if (n & 0x02) { TILE_SetPixel16(1, Pixels[6]); } \
        if (n & 0x04) { TILE_SetPixel16(2, Pixels[5]); } \
        if (n & 0x08) { TILE_SetPixel16(3, Pixels[4]); } \
        if (n & 0x10) { TILE_SetPixel16(4, Pixels[3]); } \
        if (n & 0x20) { TILE_SetPixel16(5, Pixels[2]); } \
        if (n & 0x40) { TILE_SetPixel16(6, Pixels[1]); } \
        if (n & 0x80) { TILE_SetPixel16(7, Pixels[0]); } \
    }
    
WRITE_8PIXELS(0)  WRITE_8PIXELS_FLIPPED(0)
WRITE_8PIXELS(1)  WRITE_8PIXELS_FLIPPED(1)
WRITE_8PIXELS(2)  WRITE_8PIXELS_FLIPPED(2)
WRITE_8PIXELS(3)  WRITE_8PIXELS_FLIPPED(3)
WRITE_8PIXELS(4)  WRITE_8PIXELS_FLIPPED(4)
WRITE_8PIXELS(5)  WRITE_8PIXELS_FLIPPED(5)
WRITE_8PIXELS(6)  WRITE_8PIXELS_FLIPPED(6)
WRITE_8PIXELS(7)  WRITE_8PIXELS_FLIPPED(7)
WRITE_8PIXELS(8)  WRITE_8PIXELS_FLIPPED(8)
WRITE_8PIXELS(9)  WRITE_8PIXELS_FLIPPED(9)
WRITE_8PIXELS(10)  WRITE_8PIXELS_FLIPPED(10)
WRITE_8PIXELS(11)  WRITE_8PIXELS_FLIPPED(11)
WRITE_8PIXELS(12)  WRITE_8PIXELS_FLIPPED(12)
WRITE_8PIXELS(13)  WRITE_8PIXELS_FLIPPED(13)
WRITE_8PIXELS(14)  WRITE_8PIXELS_FLIPPED(14)
WRITE_8PIXELS(15)  WRITE_8PIXELS_FLIPPED(15)
WRITE_8PIXELS(16)  WRITE_8PIXELS_FLIPPED(16)
WRITE_8PIXELS(17)  WRITE_8PIXELS_FLIPPED(17)
WRITE_8PIXELS(18)  WRITE_8PIXELS_FLIPPED(18)
WRITE_8PIXELS(19)  WRITE_8PIXELS_FLIPPED(19)
WRITE_8PIXELS(20)  WRITE_8PIXELS_FLIPPED(20)
WRITE_8PIXELS(21)  WRITE_8PIXELS_FLIPPED(21)
WRITE_8PIXELS(22)  WRITE_8PIXELS_FLIPPED(22)
WRITE_8PIXELS(23)  WRITE_8PIXELS_FLIPPED(23)
WRITE_8PIXELS(24)  WRITE_8PIXELS_FLIPPED(24)
WRITE_8PIXELS(25)  WRITE_8PIXELS_FLIPPED(25)
WRITE_8PIXELS(26)  WRITE_8PIXELS_FLIPPED(26)
WRITE_8PIXELS(27)  WRITE_8PIXELS_FLIPPED(27)
WRITE_8PIXELS(28)  WRITE_8PIXELS_FLIPPED(28)
WRITE_8PIXELS(29)  WRITE_8PIXELS_FLIPPED(29)
WRITE_8PIXELS(30)  WRITE_8PIXELS_FLIPPED(30)
WRITE_8PIXELS(31)  WRITE_8PIXELS_FLIPPED(31)
WRITE_8PIXELS(32)  WRITE_8PIXELS_FLIPPED(32)
WRITE_8PIXELS(33)  WRITE_8PIXELS_FLIPPED(33)
WRITE_8PIXELS(34)  WRITE_8PIXELS_FLIPPED(34)
WRITE_8PIXELS(35)  WRITE_8PIXELS_FLIPPED(35)
WRITE_8PIXELS(36)  WRITE_8PIXELS_FLIPPED(36)
WRITE_8PIXELS(37)  WRITE_8PIXELS_FLIPPED(37)
WRITE_8PIXELS(38)  WRITE_8PIXELS_FLIPPED(38)
WRITE_8PIXELS(39)  WRITE_8PIXELS_FLIPPED(39)
WRITE_8PIXELS(40)  WRITE_8PIXELS_FLIPPED(40)
WRITE_8PIXELS(41)  WRITE_8PIXELS_FLIPPED(41)
WRITE_8PIXELS(42)  WRITE_8PIXELS_FLIPPED(42)
WRITE_8PIXELS(43)  WRITE_8PIXELS_FLIPPED(43)
WRITE_8PIXELS(44)  WRITE_8PIXELS_FLIPPED(44)
WRITE_8PIXELS(45)  WRITE_8PIXELS_FLIPPED(45)
WRITE_8PIXELS(46)  WRITE_8PIXELS_FLIPPED(46)
WRITE_8PIXELS(47)  WRITE_8PIXELS_FLIPPED(47)
WRITE_8PIXELS(48)  WRITE_8PIXELS_FLIPPED(48)
WRITE_8PIXELS(49)  WRITE_8PIXELS_FLIPPED(49)
WRITE_8PIXELS(50)  WRITE_8PIXELS_FLIPPED(50)
WRITE_8PIXELS(51)  WRITE_8PIXELS_FLIPPED(51)
WRITE_8PIXELS(52)  WRITE_8PIXELS_FLIPPED(52)
WRITE_8PIXELS(53)  WRITE_8PIXELS_FLIPPED(53)
WRITE_8PIXELS(54)  WRITE_8PIXELS_FLIPPED(54)
WRITE_8PIXELS(55)  WRITE_8PIXELS_FLIPPED(55)
WRITE_8PIXELS(56)  WRITE_8PIXELS_FLIPPED(56)
WRITE_8PIXELS(57)  WRITE_8PIXELS_FLIPPED(57)
WRITE_8PIXELS(58)  WRITE_8PIXELS_FLIPPED(58)
WRITE_8PIXELS(59)  WRITE_8PIXELS_FLIPPED(59)
WRITE_8PIXELS(60)  WRITE_8PIXELS_FLIPPED(60)
WRITE_8PIXELS(61)  WRITE_8PIXELS_FLIPPED(61)
WRITE_8PIXELS(62)  WRITE_8PIXELS_FLIPPED(62)
WRITE_8PIXELS(63)  WRITE_8PIXELS_FLIPPED(63)
WRITE_8PIXELS(64)  WRITE_8PIXELS_FLIPPED(64)
WRITE_8PIXELS(65)  WRITE_8PIXELS_FLIPPED(65)
WRITE_8PIXELS(66)  WRITE_8PIXELS_FLIPPED(66)
WRITE_8PIXELS(67)  WRITE_8PIXELS_FLIPPED(67)
WRITE_8PIXELS(68)  WRITE_8PIXELS_FLIPPED(68)
WRITE_8PIXELS(69)  WRITE_8PIXELS_FLIPPED(69)
WRITE_8PIXELS(70)  WRITE_8PIXELS_FLIPPED(70)
WRITE_8PIXELS(71)  WRITE_8PIXELS_FLIPPED(71)
WRITE_8PIXELS(72)  WRITE_8PIXELS_FLIPPED(72)
WRITE_8PIXELS(73)  WRITE_8PIXELS_FLIPPED(73)
WRITE_8PIXELS(74)  WRITE_8PIXELS_FLIPPED(74)
WRITE_8PIXELS(75)  WRITE_8PIXELS_FLIPPED(75)
WRITE_8PIXELS(76)  WRITE_8PIXELS_FLIPPED(76)
WRITE_8PIXELS(77)  WRITE_8PIXELS_FLIPPED(77)
WRITE_8PIXELS(78)  WRITE_8PIXELS_FLIPPED(78)
WRITE_8PIXELS(79)  WRITE_8PIXELS_FLIPPED(79)
WRITE_8PIXELS(80)  WRITE_8PIXELS_FLIPPED(80)
WRITE_8PIXELS(81)  WRITE_8PIXELS_FLIPPED(81)
WRITE_8PIXELS(82)  WRITE_8PIXELS_FLIPPED(82)
WRITE_8PIXELS(83)  WRITE_8PIXELS_FLIPPED(83)
WRITE_8PIXELS(84)  WRITE_8PIXELS_FLIPPED(84)
WRITE_8PIXELS(85)  WRITE_8PIXELS_FLIPPED(85)
WRITE_8PIXELS(86)  WRITE_8PIXELS_FLIPPED(86)
WRITE_8PIXELS(87)  WRITE_8PIXELS_FLIPPED(87)
WRITE_8PIXELS(88)  WRITE_8PIXELS_FLIPPED(88)
WRITE_8PIXELS(89)  WRITE_8PIXELS_FLIPPED(89)
WRITE_8PIXELS(90)  WRITE_8PIXELS_FLIPPED(90)
WRITE_8PIXELS(91)  WRITE_8PIXELS_FLIPPED(91)
WRITE_8PIXELS(92)  WRITE_8PIXELS_FLIPPED(92)
WRITE_8PIXELS(93)  WRITE_8PIXELS_FLIPPED(93)
WRITE_8PIXELS(94)  WRITE_8PIXELS_FLIPPED(94)
WRITE_8PIXELS(95)  WRITE_8PIXELS_FLIPPED(95)
WRITE_8PIXELS(96)  WRITE_8PIXELS_FLIPPED(96)
WRITE_8PIXELS(97)  WRITE_8PIXELS_FLIPPED(97)
WRITE_8PIXELS(98)  WRITE_8PIXELS_FLIPPED(98)
WRITE_8PIXELS(99)  WRITE_8PIXELS_FLIPPED(99)
WRITE_8PIXELS(100)  WRITE_8PIXELS_FLIPPED(100)
WRITE_8PIXELS(101)  WRITE_8PIXELS_FLIPPED(101)
WRITE_8PIXELS(102)  WRITE_8PIXELS_FLIPPED(102)
WRITE_8PIXELS(103)  WRITE_8PIXELS_FLIPPED(103)
WRITE_8PIXELS(104)  WRITE_8PIXELS_FLIPPED(104)
WRITE_8PIXELS(105)  WRITE_8PIXELS_FLIPPED(105)
WRITE_8PIXELS(106)  WRITE_8PIXELS_FLIPPED(106)
WRITE_8PIXELS(107)  WRITE_8PIXELS_FLIPPED(107)
WRITE_8PIXELS(108)  WRITE_8PIXELS_FLIPPED(108)
WRITE_8PIXELS(109)  WRITE_8PIXELS_FLIPPED(109)
WRITE_8PIXELS(110)  WRITE_8PIXELS_FLIPPED(110)
WRITE_8PIXELS(111)  WRITE_8PIXELS_FLIPPED(111)
WRITE_8PIXELS(112)  WRITE_8PIXELS_FLIPPED(112)
WRITE_8PIXELS(113)  WRITE_8PIXELS_FLIPPED(113)
WRITE_8PIXELS(114)  WRITE_8PIXELS_FLIPPED(114)
WRITE_8PIXELS(115)  WRITE_8PIXELS_FLIPPED(115)
WRITE_8PIXELS(116)  WRITE_8PIXELS_FLIPPED(116)
WRITE_8PIXELS(117)  WRITE_8PIXELS_FLIPPED(117)
WRITE_8PIXELS(118)  WRITE_8PIXELS_FLIPPED(118)
WRITE_8PIXELS(119)  WRITE_8PIXELS_FLIPPED(119)
WRITE_8PIXELS(120)  WRITE_8PIXELS_FLIPPED(120)
WRITE_8PIXELS(121)  WRITE_8PIXELS_FLIPPED(121)
WRITE_8PIXELS(122)  WRITE_8PIXELS_FLIPPED(122)
WRITE_8PIXELS(123)  WRITE_8PIXELS_FLIPPED(123)
WRITE_8PIXELS(124)  WRITE_8PIXELS_FLIPPED(124)
WRITE_8PIXELS(125)  WRITE_8PIXELS_FLIPPED(125)
WRITE_8PIXELS(126)  WRITE_8PIXELS_FLIPPED(126)
WRITE_8PIXELS(127)  WRITE_8PIXELS_FLIPPED(127)
WRITE_8PIXELS(128)  WRITE_8PIXELS_FLIPPED(128)
WRITE_8PIXELS(129)  WRITE_8PIXELS_FLIPPED(129)
WRITE_8PIXELS(130)  WRITE_8PIXELS_FLIPPED(130)
WRITE_8PIXELS(131)  WRITE_8PIXELS_FLIPPED(131)
WRITE_8PIXELS(132)  WRITE_8PIXELS_FLIPPED(132)
WRITE_8PIXELS(133)  WRITE_8PIXELS_FLIPPED(133)
WRITE_8PIXELS(134)  WRITE_8PIXELS_FLIPPED(134)
WRITE_8PIXELS(135)  WRITE_8PIXELS_FLIPPED(135)
WRITE_8PIXELS(136)  WRITE_8PIXELS_FLIPPED(136)
WRITE_8PIXELS(137)  WRITE_8PIXELS_FLIPPED(137)
WRITE_8PIXELS(138)  WRITE_8PIXELS_FLIPPED(138)
WRITE_8PIXELS(139)  WRITE_8PIXELS_FLIPPED(139)
WRITE_8PIXELS(140)  WRITE_8PIXELS_FLIPPED(140)
WRITE_8PIXELS(141)  WRITE_8PIXELS_FLIPPED(141)
WRITE_8PIXELS(142)  WRITE_8PIXELS_FLIPPED(142)
WRITE_8PIXELS(143)  WRITE_8PIXELS_FLIPPED(143)
WRITE_8PIXELS(144)  WRITE_8PIXELS_FLIPPED(144)
WRITE_8PIXELS(145)  WRITE_8PIXELS_FLIPPED(145)
WRITE_8PIXELS(146)  WRITE_8PIXELS_FLIPPED(146)
WRITE_8PIXELS(147)  WRITE_8PIXELS_FLIPPED(147)
WRITE_8PIXELS(148)  WRITE_8PIXELS_FLIPPED(148)
WRITE_8PIXELS(149)  WRITE_8PIXELS_FLIPPED(149)
WRITE_8PIXELS(150)  WRITE_8PIXELS_FLIPPED(150)
WRITE_8PIXELS(151)  WRITE_8PIXELS_FLIPPED(151)
WRITE_8PIXELS(152)  WRITE_8PIXELS_FLIPPED(152)
WRITE_8PIXELS(153)  WRITE_8PIXELS_FLIPPED(153)
WRITE_8PIXELS(154)  WRITE_8PIXELS_FLIPPED(154)
WRITE_8PIXELS(155)  WRITE_8PIXELS_FLIPPED(155)
WRITE_8PIXELS(156)  WRITE_8PIXELS_FLIPPED(156)
WRITE_8PIXELS(157)  WRITE_8PIXELS_FLIPPED(157)
WRITE_8PIXELS(158)  WRITE_8PIXELS_FLIPPED(158)
WRITE_8PIXELS(159)  WRITE_8PIXELS_FLIPPED(159)
WRITE_8PIXELS(160)  WRITE_8PIXELS_FLIPPED(160)
WRITE_8PIXELS(161)  WRITE_8PIXELS_FLIPPED(161)
WRITE_8PIXELS(162)  WRITE_8PIXELS_FLIPPED(162)
WRITE_8PIXELS(163)  WRITE_8PIXELS_FLIPPED(163)
WRITE_8PIXELS(164)  WRITE_8PIXELS_FLIPPED(164)
WRITE_8PIXELS(165)  WRITE_8PIXELS_FLIPPED(165)
WRITE_8PIXELS(166)  WRITE_8PIXELS_FLIPPED(166)
WRITE_8PIXELS(167)  WRITE_8PIXELS_FLIPPED(167)
WRITE_8PIXELS(168)  WRITE_8PIXELS_FLIPPED(168)
WRITE_8PIXELS(169)  WRITE_8PIXELS_FLIPPED(169)
WRITE_8PIXELS(170)  WRITE_8PIXELS_FLIPPED(170)
WRITE_8PIXELS(171)  WRITE_8PIXELS_FLIPPED(171)
WRITE_8PIXELS(172)  WRITE_8PIXELS_FLIPPED(172)
WRITE_8PIXELS(173)  WRITE_8PIXELS_FLIPPED(173)
WRITE_8PIXELS(174)  WRITE_8PIXELS_FLIPPED(174)
WRITE_8PIXELS(175)  WRITE_8PIXELS_FLIPPED(175)
WRITE_8PIXELS(176)  WRITE_8PIXELS_FLIPPED(176)
WRITE_8PIXELS(177)  WRITE_8PIXELS_FLIPPED(177)
WRITE_8PIXELS(178)  WRITE_8PIXELS_FLIPPED(178)
WRITE_8PIXELS(179)  WRITE_8PIXELS_FLIPPED(179)
WRITE_8PIXELS(180)  WRITE_8PIXELS_FLIPPED(180)
WRITE_8PIXELS(181)  WRITE_8PIXELS_FLIPPED(181)
WRITE_8PIXELS(182)  WRITE_8PIXELS_FLIPPED(182)
WRITE_8PIXELS(183)  WRITE_8PIXELS_FLIPPED(183)
WRITE_8PIXELS(184)  WRITE_8PIXELS_FLIPPED(184)
WRITE_8PIXELS(185)  WRITE_8PIXELS_FLIPPED(185)
WRITE_8PIXELS(186)  WRITE_8PIXELS_FLIPPED(186)
WRITE_8PIXELS(187)  WRITE_8PIXELS_FLIPPED(187)
WRITE_8PIXELS(188)  WRITE_8PIXELS_FLIPPED(188)
WRITE_8PIXELS(189)  WRITE_8PIXELS_FLIPPED(189)
WRITE_8PIXELS(190)  WRITE_8PIXELS_FLIPPED(190)
WRITE_8PIXELS(191)  WRITE_8PIXELS_FLIPPED(191)
WRITE_8PIXELS(192)  WRITE_8PIXELS_FLIPPED(192)
WRITE_8PIXELS(193)  WRITE_8PIXELS_FLIPPED(193)
WRITE_8PIXELS(194)  WRITE_8PIXELS_FLIPPED(194)
WRITE_8PIXELS(195)  WRITE_8PIXELS_FLIPPED(195)
WRITE_8PIXELS(196)  WRITE_8PIXELS_FLIPPED(196)
WRITE_8PIXELS(197)  WRITE_8PIXELS_FLIPPED(197)
WRITE_8PIXELS(198)  WRITE_8PIXELS_FLIPPED(198)
WRITE_8PIXELS(199)  WRITE_8PIXELS_FLIPPED(199)
WRITE_8PIXELS(200)  WRITE_8PIXELS_FLIPPED(200)
WRITE_8PIXELS(201)  WRITE_8PIXELS_FLIPPED(201)
WRITE_8PIXELS(202)  WRITE_8PIXELS_FLIPPED(202)
WRITE_8PIXELS(203)  WRITE_8PIXELS_FLIPPED(203)
WRITE_8PIXELS(204)  WRITE_8PIXELS_FLIPPED(204)
WRITE_8PIXELS(205)  WRITE_8PIXELS_FLIPPED(205)
WRITE_8PIXELS(206)  WRITE_8PIXELS_FLIPPED(206)
WRITE_8PIXELS(207)  WRITE_8PIXELS_FLIPPED(207)
WRITE_8PIXELS(208)  WRITE_8PIXELS_FLIPPED(208)
WRITE_8PIXELS(209)  WRITE_8PIXELS_FLIPPED(209)
WRITE_8PIXELS(210)  WRITE_8PIXELS_FLIPPED(210)
WRITE_8PIXELS(211)  WRITE_8PIXELS_FLIPPED(211)
WRITE_8PIXELS(212)  WRITE_8PIXELS_FLIPPED(212)
WRITE_8PIXELS(213)  WRITE_8PIXELS_FLIPPED(213)
WRITE_8PIXELS(214)  WRITE_8PIXELS_FLIPPED(214)
WRITE_8PIXELS(215)  WRITE_8PIXELS_FLIPPED(215)
WRITE_8PIXELS(216)  WRITE_8PIXELS_FLIPPED(216)
WRITE_8PIXELS(217)  WRITE_8PIXELS_FLIPPED(217)
WRITE_8PIXELS(218)  WRITE_8PIXELS_FLIPPED(218)
WRITE_8PIXELS(219)  WRITE_8PIXELS_FLIPPED(219)
WRITE_8PIXELS(220)  WRITE_8PIXELS_FLIPPED(220)
WRITE_8PIXELS(221)  WRITE_8PIXELS_FLIPPED(221)
WRITE_8PIXELS(222)  WRITE_8PIXELS_FLIPPED(222)
WRITE_8PIXELS(223)  WRITE_8PIXELS_FLIPPED(223)
WRITE_8PIXELS(224)  WRITE_8PIXELS_FLIPPED(224)
WRITE_8PIXELS(225)  WRITE_8PIXELS_FLIPPED(225)
WRITE_8PIXELS(226)  WRITE_8PIXELS_FLIPPED(226)
WRITE_8PIXELS(227)  WRITE_8PIXELS_FLIPPED(227)
WRITE_8PIXELS(228)  WRITE_8PIXELS_FLIPPED(228)
WRITE_8PIXELS(229)  WRITE_8PIXELS_FLIPPED(229)
WRITE_8PIXELS(230)  WRITE_8PIXELS_FLIPPED(230)
WRITE_8PIXELS(231)  WRITE_8PIXELS_FLIPPED(231)
WRITE_8PIXELS(232)  WRITE_8PIXELS_FLIPPED(232)
WRITE_8PIXELS(233)  WRITE_8PIXELS_FLIPPED(233)
WRITE_8PIXELS(234)  WRITE_8PIXELS_FLIPPED(234)
WRITE_8PIXELS(235)  WRITE_8PIXELS_FLIPPED(235)
WRITE_8PIXELS(236)  WRITE_8PIXELS_FLIPPED(236)
WRITE_8PIXELS(237)  WRITE_8PIXELS_FLIPPED(237)
WRITE_8PIXELS(238)  WRITE_8PIXELS_FLIPPED(238)
WRITE_8PIXELS(239)  WRITE_8PIXELS_FLIPPED(239)
WRITE_8PIXELS(240)  WRITE_8PIXELS_FLIPPED(240)
WRITE_8PIXELS(241)  WRITE_8PIXELS_FLIPPED(241)
WRITE_8PIXELS(242)  WRITE_8PIXELS_FLIPPED(242)
WRITE_8PIXELS(243)  WRITE_8PIXELS_FLIPPED(243)
WRITE_8PIXELS(244)  WRITE_8PIXELS_FLIPPED(244)
WRITE_8PIXELS(245)  WRITE_8PIXELS_FLIPPED(245)
WRITE_8PIXELS(246)  WRITE_8PIXELS_FLIPPED(246)
WRITE_8PIXELS(247)  WRITE_8PIXELS_FLIPPED(247)
WRITE_8PIXELS(248)  WRITE_8PIXELS_FLIPPED(248)
WRITE_8PIXELS(249)  WRITE_8PIXELS_FLIPPED(249)
WRITE_8PIXELS(250)  WRITE_8PIXELS_FLIPPED(250)
WRITE_8PIXELS(251)  WRITE_8PIXELS_FLIPPED(251)
WRITE_8PIXELS(252)  WRITE_8PIXELS_FLIPPED(252)
WRITE_8PIXELS(253)  WRITE_8PIXELS_FLIPPED(253)
WRITE_8PIXELS(254)  WRITE_8PIXELS_FLIPPED(254)
WRITE_8PIXELS(255)  WRITE_8PIXELS_FLIPPED(255)


void (*Write8Pixels[256])(uint32, uint8*) = 
{
    Write8Pixels_0,
    Write8Pixels_1,
    Write8Pixels_2,
    Write8Pixels_3,
    Write8Pixels_4,
    Write8Pixels_5,
    Write8Pixels_6,
    Write8Pixels_7,
    Write8Pixels_8,
    Write8Pixels_9,
    Write8Pixels_10,
    Write8Pixels_11,
    Write8Pixels_12,
    Write8Pixels_13,
    Write8Pixels_14,
    Write8Pixels_15,
    Write8Pixels_16,
    Write8Pixels_17,
    Write8Pixels_18,
    Write8Pixels_19,
    Write8Pixels_20,
    Write8Pixels_21,
    Write8Pixels_22,
    Write8Pixels_23,
    Write8Pixels_24,
    Write8Pixels_25,
    Write8Pixels_26,
    Write8Pixels_27,
    Write8Pixels_28,
    Write8Pixels_29,
    Write8Pixels_30,
    Write8Pixels_31,
    Write8Pixels_32,
    Write8Pixels_33,
    Write8Pixels_34,
    Write8Pixels_35,
    Write8Pixels_36,
    Write8Pixels_37,
    Write8Pixels_38,
    Write8Pixels_39,
    Write8Pixels_40,
    Write8Pixels_41,
    Write8Pixels_42,
    Write8Pixels_43,
    Write8Pixels_44,
    Write8Pixels_45,
    Write8Pixels_46,
    Write8Pixels_47,
    Write8Pixels_48,
    Write8Pixels_49,
    Write8Pixels_50,
    Write8Pixels_51,
    Write8Pixels_52,
    Write8Pixels_53,
    Write8Pixels_54,
    Write8Pixels_55,
    Write8Pixels_56,
    Write8Pixels_57,
    Write8Pixels_58,
    Write8Pixels_59,
    Write8Pixels_60,
    Write8Pixels_61,
    Write8Pixels_62,
    Write8Pixels_63,
    Write8Pixels_64,
    Write8Pixels_65,
    Write8Pixels_66,
    Write8Pixels_67,
    Write8Pixels_68,
    Write8Pixels_69,
    Write8Pixels_70,
    Write8Pixels_71,
    Write8Pixels_72,
    Write8Pixels_73,
    Write8Pixels_74,
    Write8Pixels_75,
    Write8Pixels_76,
    Write8Pixels_77,
    Write8Pixels_78,
    Write8Pixels_79,
    Write8Pixels_80,
    Write8Pixels_81,
    Write8Pixels_82,
    Write8Pixels_83,
    Write8Pixels_84,
    Write8Pixels_85,
    Write8Pixels_86,
    Write8Pixels_87,
    Write8Pixels_88,
    Write8Pixels_89,
    Write8Pixels_90,
    Write8Pixels_91,
    Write8Pixels_92,
    Write8Pixels_93,
    Write8Pixels_94,
    Write8Pixels_95,
    Write8Pixels_96,
    Write8Pixels_97,
    Write8Pixels_98,
    Write8Pixels_99,
    Write8Pixels_100,
    Write8Pixels_101,
    Write8Pixels_102,
    Write8Pixels_103,
    Write8Pixels_104,
    Write8Pixels_105,
    Write8Pixels_106,
    Write8Pixels_107,
    Write8Pixels_108,
    Write8Pixels_109,
    Write8Pixels_110,
    Write8Pixels_111,
    Write8Pixels_112,
    Write8Pixels_113,
    Write8Pixels_114,
    Write8Pixels_115,
    Write8Pixels_116,
    Write8Pixels_117,
    Write8Pixels_118,
    Write8Pixels_119,
    Write8Pixels_120,
    Write8Pixels_121,
    Write8Pixels_122,
    Write8Pixels_123,
    Write8Pixels_124,
    Write8Pixels_125,
    Write8Pixels_126,
    Write8Pixels_127,
    Write8Pixels_128,
    Write8Pixels_129,
    Write8Pixels_130,
    Write8Pixels_131,
    Write8Pixels_132,
    Write8Pixels_133,
    Write8Pixels_134,
    Write8Pixels_135,
    Write8Pixels_136,
    Write8Pixels_137,
    Write8Pixels_138,
    Write8Pixels_139,
    Write8Pixels_140,
    Write8Pixels_141,
    Write8Pixels_142,
    Write8Pixels_143,
    Write8Pixels_144,
    Write8Pixels_145,
    Write8Pixels_146,
    Write8Pixels_147,
    Write8Pixels_148,
    Write8Pixels_149,
    Write8Pixels_150,
    Write8Pixels_151,
    Write8Pixels_152,
    Write8Pixels_153,
    Write8Pixels_154,
    Write8Pixels_155,
    Write8Pixels_156,
    Write8Pixels_157,
    Write8Pixels_158,
    Write8Pixels_159,
    Write8Pixels_160,
    Write8Pixels_161,
    Write8Pixels_162,
    Write8Pixels_163,
    Write8Pixels_164,
    Write8Pixels_165,
    Write8Pixels_166,
    Write8Pixels_167,
    Write8Pixels_168,
    Write8Pixels_169,
    Write8Pixels_170,
    Write8Pixels_171,
    Write8Pixels_172,
    Write8Pixels_173,
    Write8Pixels_174,
    Write8Pixels_175,
    Write8Pixels_176,
    Write8Pixels_177,
    Write8Pixels_178,
    Write8Pixels_179,
    Write8Pixels_180,
    Write8Pixels_181,
    Write8Pixels_182,
    Write8Pixels_183,
    Write8Pixels_184,
    Write8Pixels_185,
    Write8Pixels_186,
    Write8Pixels_187,
    Write8Pixels_188,
    Write8Pixels_189,
    Write8Pixels_190,
    Write8Pixels_191,
    Write8Pixels_192,
    Write8Pixels_193,
    Write8Pixels_194,
    Write8Pixels_195,
    Write8Pixels_196,
    Write8Pixels_197,
    Write8Pixels_198,
    Write8Pixels_199,
    Write8Pixels_200,
    Write8Pixels_201,
    Write8Pixels_202,
    Write8Pixels_203,
    Write8Pixels_204,
    Write8Pixels_205,
    Write8Pixels_206,
    Write8Pixels_207,
    Write8Pixels_208,
    Write8Pixels_209,
    Write8Pixels_210,
    Write8Pixels_211,
    Write8Pixels_212,
    Write8Pixels_213,
    Write8Pixels_214,
    Write8Pixels_215,
    Write8Pixels_216,
    Write8Pixels_217,
    Write8Pixels_218,
    Write8Pixels_219,
    Write8Pixels_220,
    Write8Pixels_221,
    Write8Pixels_222,
    Write8Pixels_223,
    Write8Pixels_224,
    Write8Pixels_225,
    Write8Pixels_226,
    Write8Pixels_227,
    Write8Pixels_228,
    Write8Pixels_229,
    Write8Pixels_230,
    Write8Pixels_231,
    Write8Pixels_232,
    Write8Pixels_233,
    Write8Pixels_234,
    Write8Pixels_235,
    Write8Pixels_236,
    Write8Pixels_237,
    Write8Pixels_238,
    Write8Pixels_239,
    Write8Pixels_240,
    Write8Pixels_241,
    Write8Pixels_242,
    Write8Pixels_243,
    Write8Pixels_244,
    Write8Pixels_245,
    Write8Pixels_246,
    Write8Pixels_247,
    Write8Pixels_248,
    Write8Pixels_249,
    Write8Pixels_250,
    Write8Pixels_251,
    Write8Pixels_252,
    Write8Pixels_253,
    Write8Pixels_254,
    Write8Pixels_255 };

void (*Write8PixelsFlipped[256])(uint32, uint8*)  = 
{
    Write8PixelsFlipped_0,
    Write8PixelsFlipped_1,
    Write8PixelsFlipped_2,
    Write8PixelsFlipped_3,
    Write8PixelsFlipped_4,
    Write8PixelsFlipped_5,
    Write8PixelsFlipped_6,
    Write8PixelsFlipped_7,
    Write8PixelsFlipped_8,
    Write8PixelsFlipped_9,
    Write8PixelsFlipped_10,
    Write8PixelsFlipped_11,
    Write8PixelsFlipped_12,
    Write8PixelsFlipped_13,
    Write8PixelsFlipped_14,
    Write8PixelsFlipped_15,
    Write8PixelsFlipped_16,
    Write8PixelsFlipped_17,
    Write8PixelsFlipped_18,
    Write8PixelsFlipped_19,
    Write8PixelsFlipped_20,
    Write8PixelsFlipped_21,
    Write8PixelsFlipped_22,
    Write8PixelsFlipped_23,
    Write8PixelsFlipped_24,
    Write8PixelsFlipped_25,
    Write8PixelsFlipped_26,
    Write8PixelsFlipped_27,
    Write8PixelsFlipped_28,
    Write8PixelsFlipped_29,
    Write8PixelsFlipped_30,
    Write8PixelsFlipped_31,
    Write8PixelsFlipped_32,
    Write8PixelsFlipped_33,
    Write8PixelsFlipped_34,
    Write8PixelsFlipped_35,
    Write8PixelsFlipped_36,
    Write8PixelsFlipped_37,
    Write8PixelsFlipped_38,
    Write8PixelsFlipped_39,
    Write8PixelsFlipped_40,
    Write8PixelsFlipped_41,
    Write8PixelsFlipped_42,
    Write8PixelsFlipped_43,
    Write8PixelsFlipped_44,
    Write8PixelsFlipped_45,
    Write8PixelsFlipped_46,
    Write8PixelsFlipped_47,
    Write8PixelsFlipped_48,
    Write8PixelsFlipped_49,
    Write8PixelsFlipped_50,
    Write8PixelsFlipped_51,
    Write8PixelsFlipped_52,
    Write8PixelsFlipped_53,
    Write8PixelsFlipped_54,
    Write8PixelsFlipped_55,
    Write8PixelsFlipped_56,
    Write8PixelsFlipped_57,
    Write8PixelsFlipped_58,
    Write8PixelsFlipped_59,
    Write8PixelsFlipped_60,
    Write8PixelsFlipped_61,
    Write8PixelsFlipped_62,
    Write8PixelsFlipped_63,
    Write8PixelsFlipped_64,
    Write8PixelsFlipped_65,
    Write8PixelsFlipped_66,
    Write8PixelsFlipped_67,
    Write8PixelsFlipped_68,
    Write8PixelsFlipped_69,
    Write8PixelsFlipped_70,
    Write8PixelsFlipped_71,
    Write8PixelsFlipped_72,
    Write8PixelsFlipped_73,
    Write8PixelsFlipped_74,
    Write8PixelsFlipped_75,
    Write8PixelsFlipped_76,
    Write8PixelsFlipped_77,
    Write8PixelsFlipped_78,
    Write8PixelsFlipped_79,
    Write8PixelsFlipped_80,
    Write8PixelsFlipped_81,
    Write8PixelsFlipped_82,
    Write8PixelsFlipped_83,
    Write8PixelsFlipped_84,
    Write8PixelsFlipped_85,
    Write8PixelsFlipped_86,
    Write8PixelsFlipped_87,
    Write8PixelsFlipped_88,
    Write8PixelsFlipped_89,
    Write8PixelsFlipped_90,
    Write8PixelsFlipped_91,
    Write8PixelsFlipped_92,
    Write8PixelsFlipped_93,
    Write8PixelsFlipped_94,
    Write8PixelsFlipped_95,
    Write8PixelsFlipped_96,
    Write8PixelsFlipped_97,
    Write8PixelsFlipped_98,
    Write8PixelsFlipped_99,
    Write8PixelsFlipped_100,
    Write8PixelsFlipped_101,
    Write8PixelsFlipped_102,
    Write8PixelsFlipped_103,
    Write8PixelsFlipped_104,
    Write8PixelsFlipped_105,
    Write8PixelsFlipped_106,
    Write8PixelsFlipped_107,
    Write8PixelsFlipped_108,
    Write8PixelsFlipped_109,
    Write8PixelsFlipped_110,
    Write8PixelsFlipped_111,
    Write8PixelsFlipped_112,
    Write8PixelsFlipped_113,
    Write8PixelsFlipped_114,
    Write8PixelsFlipped_115,
    Write8PixelsFlipped_116,
    Write8PixelsFlipped_117,
    Write8PixelsFlipped_118,
    Write8PixelsFlipped_119,
    Write8PixelsFlipped_120,
    Write8PixelsFlipped_121,
    Write8PixelsFlipped_122,
    Write8PixelsFlipped_123,
    Write8PixelsFlipped_124,
    Write8PixelsFlipped_125,
    Write8PixelsFlipped_126,
    Write8PixelsFlipped_127,
    Write8PixelsFlipped_128,
    Write8PixelsFlipped_129,
    Write8PixelsFlipped_130,
    Write8PixelsFlipped_131,
    Write8PixelsFlipped_132,
    Write8PixelsFlipped_133,
    Write8PixelsFlipped_134,
    Write8PixelsFlipped_135,
    Write8PixelsFlipped_136,
    Write8PixelsFlipped_137,
    Write8PixelsFlipped_138,
    Write8PixelsFlipped_139,
    Write8PixelsFlipped_140,
    Write8PixelsFlipped_141,
    Write8PixelsFlipped_142,
    Write8PixelsFlipped_143,
    Write8PixelsFlipped_144,
    Write8PixelsFlipped_145,
    Write8PixelsFlipped_146,
    Write8PixelsFlipped_147,
    Write8PixelsFlipped_148,
    Write8PixelsFlipped_149,
    Write8PixelsFlipped_150,
    Write8PixelsFlipped_151,
    Write8PixelsFlipped_152,
    Write8PixelsFlipped_153,
    Write8PixelsFlipped_154,
    Write8PixelsFlipped_155,
    Write8PixelsFlipped_156,
    Write8PixelsFlipped_157,
    Write8PixelsFlipped_158,
    Write8PixelsFlipped_159,
    Write8PixelsFlipped_160,
    Write8PixelsFlipped_161,
    Write8PixelsFlipped_162,
    Write8PixelsFlipped_163,
    Write8PixelsFlipped_164,
    Write8PixelsFlipped_165,
    Write8PixelsFlipped_166,
    Write8PixelsFlipped_167,
    Write8PixelsFlipped_168,
    Write8PixelsFlipped_169,
    Write8PixelsFlipped_170,
    Write8PixelsFlipped_171,
    Write8PixelsFlipped_172,
    Write8PixelsFlipped_173,
    Write8PixelsFlipped_174,
    Write8PixelsFlipped_175,
    Write8PixelsFlipped_176,
    Write8PixelsFlipped_177,
    Write8PixelsFlipped_178,
    Write8PixelsFlipped_179,
    Write8PixelsFlipped_180,
    Write8PixelsFlipped_181,
    Write8PixelsFlipped_182,
    Write8PixelsFlipped_183,
    Write8PixelsFlipped_184,
    Write8PixelsFlipped_185,
    Write8PixelsFlipped_186,
    Write8PixelsFlipped_187,
    Write8PixelsFlipped_188,
    Write8PixelsFlipped_189,
    Write8PixelsFlipped_190,
    Write8PixelsFlipped_191,
    Write8PixelsFlipped_192,
    Write8PixelsFlipped_193,
    Write8PixelsFlipped_194,
    Write8PixelsFlipped_195,
    Write8PixelsFlipped_196,
    Write8PixelsFlipped_197,
    Write8PixelsFlipped_198,
    Write8PixelsFlipped_199,
    Write8PixelsFlipped_200,
    Write8PixelsFlipped_201,
    Write8PixelsFlipped_202,
    Write8PixelsFlipped_203,
    Write8PixelsFlipped_204,
    Write8PixelsFlipped_205,
    Write8PixelsFlipped_206,
    Write8PixelsFlipped_207,
    Write8PixelsFlipped_208,
    Write8PixelsFlipped_209,
    Write8PixelsFlipped_210,
    Write8PixelsFlipped_211,
    Write8PixelsFlipped_212,
    Write8PixelsFlipped_213,
    Write8PixelsFlipped_214,
    Write8PixelsFlipped_215,
    Write8PixelsFlipped_216,
    Write8PixelsFlipped_217,
    Write8PixelsFlipped_218,
    Write8PixelsFlipped_219,
    Write8PixelsFlipped_220,
    Write8PixelsFlipped_221,
    Write8PixelsFlipped_222,
    Write8PixelsFlipped_223,
    Write8PixelsFlipped_224,
    Write8PixelsFlipped_225,
    Write8PixelsFlipped_226,
    Write8PixelsFlipped_227,
    Write8PixelsFlipped_228,
    Write8PixelsFlipped_229,
    Write8PixelsFlipped_230,
    Write8PixelsFlipped_231,
    Write8PixelsFlipped_232,
    Write8PixelsFlipped_233,
    Write8PixelsFlipped_234,
    Write8PixelsFlipped_235,
    Write8PixelsFlipped_236,
    Write8PixelsFlipped_237,
    Write8PixelsFlipped_238,
    Write8PixelsFlipped_239,
    Write8PixelsFlipped_240,
    Write8PixelsFlipped_241,
    Write8PixelsFlipped_242,
    Write8PixelsFlipped_243,
    Write8PixelsFlipped_244,
    Write8PixelsFlipped_245,
    Write8PixelsFlipped_246,
    Write8PixelsFlipped_247,
    Write8PixelsFlipped_248,
    Write8PixelsFlipped_249,
    Write8PixelsFlipped_250,
    Write8PixelsFlipped_251,
    Write8PixelsFlipped_252,
    Write8PixelsFlipped_253,
    Write8PixelsFlipped_254,
    Write8PixelsFlipped_255 };



void DrawTile (uint32 Tile, uint32 Offset, uint32 StartLine,
	       uint32 LineCount)
{
    TILE_PREAMBLE

    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS, WRITE_4PIXELS_FLIPPED, 4)
}


void DrawClippedTile (uint32 Tile, uint32 Offset,
		      uint32 StartPixel, uint32 Width,
		      uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS, WRITE_4PIXELS_FLIPPED, 4)
}

void DrawTilex2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		 uint32 LineCount)
{
    TILE_PREAMBLE

    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELSx2, WRITE_4PIXELS_FLIPPEDx2, 8)
}

void DrawClippedTilex2 (uint32 Tile, uint32 Offset,
			uint32 StartPixel, uint32 Width,
			uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELSx2, WRITE_4PIXELS_FLIPPEDx2, 8)
}

void DrawTilex2x2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		   uint32 LineCount)
{
    TILE_PREAMBLE

    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELSx2x2, WRITE_4PIXELS_FLIPPEDx2x2, 8)
}

void DrawClippedTilex2x2 (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Width,
			  uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELSx2x2, WRITE_4PIXELS_FLIPPEDx2x2, 8)
}

inline void __attribute__((always_inline)) WRITE_4PIXELS16 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Pixels16 = (uint16 *)Pixels;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;

#define FN(N) \
    if ((Pixel = Pixels16[N])) \
    { \
	TILE_AssignPixel(N, Pixel); \
    }
/*
#define FN(N) \
    if ((Pixel = Pixels[N])) \
    { \
	TILE_SetPixel16(N, Pixel); \
    }
*/
    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}

inline void __attribute__((always_inline)) WRITE_4PIXELS16_FLIPPED (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Pixels16 = (uint16 *)Pixels;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;


#define FN(N) \
    if ((Pixel = Pixels16[3 - N])) \
    { \
	TILE_AssignPixel(N, Pixel); \
    }
/*
#define FN(N) \
    if ((Pixel = Pixels[3 - N])) \
    { \
	TILE_SetPixel16(N, Pixel); \
    }
*/    
    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}




inline void WRITE_4PIXELS16x2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;

#define FN(N) \
    if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[N])) \
    { \
	TILE_SetPixel16(N*2, Pixel); \
	TILE_SetPixel16(N*2+1, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}

inline void WRITE_4PIXELS16_FLIPPEDx2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;

#define FN(N) \
    if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[3 - N])) \
    { \
	TILE_SetPixel16(N*2, Pixel); \
	TILE_SetPixel16(N*2+1, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}

inline void WRITE_4PIXELS16x2x2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;

#define FN(N) \
    if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[N])) \
    { \
	TILE_SetPixel16(N*2, Pixel); \
	TILE_SetPixel16(N*2+1, Pixel); \
	TILE_SetPixel16((GFX.RealPitch>>1)+N*2, Pixel); \
	TILE_SetPixel16((GFX.RealPitch>>1)+N*2+1, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}

inline void WRITE_4PIXELS16_FLIPPEDx2x2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;

#define FN(N) \
    if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[3 - N])) \
    { \
	TILE_SetPixel16(N*2, Pixel); \
	TILE_SetPixel16(N*2+1, Pixel); \
	TILE_SetPixel16((GFX.RealPitch>>1)+N*2, Pixel); \
	TILE_SetPixel16((GFX.RealPitch>>1)+N*2+1, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)
#undef FN
}


void DrawTile16 (uint32 Tile, uint32 Offset, uint32 StartLine,
	         uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16, WRITE_4PIXELS16_FLIPPED, 4)
}



void DrawClippedTile16 (uint32 Tile, uint32 Offset,
			uint32 StartPixel, uint32 Width,
			uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16, WRITE_4PIXELS16_FLIPPED, 4)
}

void DrawTile16x2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		   uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16x2, WRITE_4PIXELS16_FLIPPEDx2, 8)
}

void DrawClippedTile16x2 (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Width,
			  uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16x2, WRITE_4PIXELS16_FLIPPEDx2, 8)
}

void DrawTile16x2x2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		     uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16x2x2, WRITE_4PIXELS16_FLIPPEDx2x2, 8)
}

void DrawClippedTile16x2x2 (uint32 Tile, uint32 Offset,
			    uint32 StartPixel, uint32 Width,
			    uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16x2x2, WRITE_4PIXELS16_FLIPPEDx2x2, 8)
}

inline void __attribute__((always_inline)) WRITE_4PIXELS16_ADD (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[N])) \
    { \
	TILE_SelectAddPixel16(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

inline void __attribute__((always_inline)) WRITE_4PIXELS16_FLIPPED_ADD (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[3 - N])) \
    { \
	TILE_SelectAddPixel16(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

inline void __attribute__((always_inline)) WRITE_4PIXELS16_ADD1_2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[N])) \
    { \
	TILE_SelectAddPixel16Half(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

inline void __attribute__((always_inline)) WRITE_4PIXELS16_FLIPPED_ADD1_2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[3 - N])) \
    { \
	TILE_SelectAddPixel16Half(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

inline void __attribute__((always_inline)) WRITE_4PIXELS16_SUB (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[N])) \
    { \
	TILE_SelectSubPixel16(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

inline void __attribute__((always_inline)) WRITE_4PIXELS16_FLIPPED_SUB (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[3 - N])) \
    { \
	TILE_SelectSubPixel16(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

inline void __attribute__((always_inline)) WRITE_4PIXELS16_SUB1_2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[N])) \
    { \
	TILE_SelectSubPixel16Half(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

inline void __attribute__((always_inline)) WRITE_4PIXELS16_FLIPPED_SUB1_2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[3 - N])) \
    { \
	TILE_SelectSubPixel16Half(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}


void DrawTile16Add (uint32 Tile, uint32 Offset, uint32 StartLine,
		    uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_ADD, WRITE_4PIXELS16_FLIPPED_ADD, 4)
}

void DrawClippedTile16Add (uint32 Tile, uint32 Offset,
			   uint32 StartPixel, uint32 Width,
			   uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_ADD, WRITE_4PIXELS16_FLIPPED_ADD, 4)
}

void DrawTile16Add1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		       uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_ADD1_2, WRITE_4PIXELS16_FLIPPED_ADD1_2, 4)
}

void DrawClippedTile16Add1_2 (uint32 Tile, uint32 Offset,
			      uint32 StartPixel, uint32 Width,
			      uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_ADD1_2, WRITE_4PIXELS16_FLIPPED_ADD1_2, 4)
}

void DrawTile16Sub (uint32 Tile, uint32 Offset, uint32 StartLine,
		    uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_SUB, WRITE_4PIXELS16_FLIPPED_SUB, 4)
}

void DrawClippedTile16Sub (uint32 Tile, uint32 Offset,
			   uint32 StartPixel, uint32 Width,
			   uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_SUB, WRITE_4PIXELS16_FLIPPED_SUB, 4)
}

void DrawTile16Sub1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		       uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_SUB1_2, WRITE_4PIXELS16_FLIPPED_SUB1_2, 4)
}

void DrawClippedTile16Sub1_2 (uint32 Tile, uint32 Offset,
			      uint32 StartPixel, uint32 Width,
			      uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_SUB1_2, WRITE_4PIXELS16_FLIPPED_SUB1_2, 4)
}

inline void WRITE_4PIXELS16_ADDF1_2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[N])) \
    { \
	TILE_SelectFAddPixel16Half(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

inline void WRITE_4PIXELS16_FLIPPED_ADDF1_2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[3 - N])) \
    { \
	TILE_SelectFAddPixel16Half(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

inline void WRITE_4PIXELS16_SUBF1_2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[N])) \
    { \
	TILE_SelectFSubPixel16Half(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

inline void WRITE_4PIXELS16_FLIPPED_SUBF1_2 (uint32 Offset, uint8 *Pixels)
{
    uint32 Pixel;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint8  *SubDepth = GFX.SubZBuffer + Offset;

#define FN(N) \
    if ((Pixel = Pixels[3 - N])) \
    { \
	TILE_SelectFSubPixel16Half(N, Pixel); \
    }

    FN(0)
    FN(1)
    FN(2)
    FN(3)

#undef FN
}

void DrawTile16FixedAdd1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
			    uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_ADDF1_2, WRITE_4PIXELS16_FLIPPED_ADDF1_2, 4)
}

void DrawClippedTile16FixedAdd1_2 (uint32 Tile, uint32 Offset,
				   uint32 StartPixel, uint32 Width,
				   uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_ADDF1_2, 
			WRITE_4PIXELS16_FLIPPED_ADDF1_2, 4)
}

void DrawTile16FixedSub1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
			    uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_SUBF1_2, WRITE_4PIXELS16_FLIPPED_SUBF1_2, 4)
}

void DrawClippedTile16FixedSub1_2 (uint32 Tile, uint32 Offset,
				   uint32 StartPixel, uint32 Width,
				   uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_SUBF1_2, 
			WRITE_4PIXELS16_FLIPPED_SUBF1_2, 4)
}

void DrawLargePixel (uint32 Tile, uint32 Offset,
		     uint32 StartPixel, uint32 Pixels,
		     uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint8 *sp = GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;
    uint8 pixel;
#define PLOT_PIXEL(screen, pixel) (pixel)

    RENDER_TILE_LARGE (((uint8) GFX.ScreenColors [pixel]), PLOT_PIXEL)
}

void DrawLargePixel16 (uint32 Tile, uint32 Offset,
		       uint32 StartPixel, uint32 Pixels,
		       uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;
    uint16 pixel;

    RENDER_TILE_LARGE (GFX.ScreenColors [pixel], PLOT_PIXEL)
}

void DrawLargePixel16Add (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Pixels,
			  uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint16 pixel;

#define LARGE_ADD_PIXEL(s, p) \
(Depth [z + GFX.DepthDelta] ? (Depth [z + GFX.DepthDelta] != 1 ? \
			       COLOR_ADD (p, *(s + GFX.Delta))    : \
			       COLOR_ADD (p, GFX.FixedColour)) \
			    : p)
			      
    RENDER_TILE_LARGE (GFX.ScreenColors [pixel], LARGE_ADD_PIXEL)
}

void DrawLargePixel16Add1_2 (uint32 Tile, uint32 Offset,
			     uint32 StartPixel, uint32 Pixels,
			     uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint16 pixel;

#define LARGE_ADD_PIXEL1_2(s, p) \
((uint16) (Depth [z + GFX.DepthDelta] ? (Depth [z + GFX.DepthDelta] != 1 ? \
			       COLOR_ADD1_2 (p, *(s + GFX.Delta))    : \
			       COLOR_ADD (p, GFX.FixedColour)) \
			    : p))
			      
    RENDER_TILE_LARGE (GFX.ScreenColors [pixel], LARGE_ADD_PIXEL1_2)
}

void DrawLargePixel16Sub (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Pixels,
			  uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint16 pixel;

#define LARGE_SUB_PIXEL(s, p) \
(Depth [z + GFX.DepthDelta] ? (Depth [z + GFX.DepthDelta] != 1 ? \
			       COLOR_SUB (p, *(s + GFX.Delta))    : \
			       COLOR_SUB (p, GFX.FixedColour)) \
			    : p)
			      
    RENDER_TILE_LARGE (GFX.ScreenColors [pixel], LARGE_SUB_PIXEL)
}

void DrawLargePixel16Sub1_2 (uint32 Tile, uint32 Offset,
			     uint32 StartPixel, uint32 Pixels,
			     uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint16 pixel;

#define LARGE_SUB_PIXEL1_2(s, p) \
(Depth [z + GFX.DepthDelta] ? (Depth [z + GFX.DepthDelta] != 1 ? \
			       COLOR_SUB1_2 (p, *(s + GFX.Delta))    : \
			       COLOR_SUB (p, GFX.FixedColour)) \
			    : p)
			      
    RENDER_TILE_LARGE (GFX.ScreenColors [pixel], LARGE_SUB_PIXEL1_2)
}

#ifdef USE_GLIDE
#if 0
void DrawTile3dfx (uint32 Tile, uint32 Offset, uint32 StartLine,
		   uint32 LineCount)
{
    TILE_PREAMBLE

    float x = Offset % GFX.Pitch;
    float y = Offset / GFX.Pitch;

    Glide.sq [0].x = Glide.x_offset + x * Glide.x_scale;
    Glide.sq [0].y = Glide.y_offset + y * Glide.y_scale;
    Glide.sq [1].x = Glide.x_offset + (x + 8.0) * Glide.x_scale;
    Glide.sq [1].y = Glide.y_offset + y * Glide.y_scale;
    Glide.sq [2].x = Glide.x_offset + (x + 8.0) * Glide.x_scale;
    Glide.sq [2].y = Glide.y_offset + (y + LineCount) * Glide.y_scale;
    Glide.sq [3].x = Glide.x_offset + x * Glide.x_scale;
    Glide.sq [3].y = Glide.y_offset + (y + LineCount) * Glide.y_scale;

    if (!(Tile & (V_FLIP | H_FLIP)))
    {
	// Normal
	Glide.sq [0].tmuvtx [0].sow = 0.0;
	Glide.sq [0].tmuvtx [0].tow = StartLine;
	Glide.sq [1].tmuvtx [0].sow = 8.0;
	Glide.sq [1].tmuvtx [0].tow = StartLine;
	Glide.sq [2].tmuvtx [0].sow = 8.0;
	Glide.sq [2].tmuvtx [0].tow = StartLine + LineCount;
	Glide.sq [3].tmuvtx [0].sow = 0.0;
	Glide.sq [3].tmuvtx [0].tow = StartLine + LineCount;
    }
    else
    if (!(Tile & V_FLIP))
    {
	// Flipped
	Glide.sq [0].tmuvtx [0].sow = 8.0;
	Glide.sq [0].tmuvtx [0].tow = StartLine;
	Glide.sq [1].tmuvtx [0].sow = 0.0;
	Glide.sq [1].tmuvtx [0].tow = StartLine;
	Glide.sq [2].tmuvtx [0].sow = 0.0;
	Glide.sq [2].tmuvtx [0].tow = StartLine + LineCount;
	Glide.sq [3].tmuvtx [0].sow = 8.0;
	Glide.sq [3].tmuvtx [0].tow = StartLine + LineCount;
    }
    else
    if (Tile & H_FLIP)
    {
	// Horizontal and vertical flip
	Glide.sq [0].tmuvtx [0].sow = 8.0;
	Glide.sq [0].tmuvtx [0].tow = StartLine + LineCount;
	Glide.sq [1].tmuvtx [0].sow = 0.0;
	Glide.sq [1].tmuvtx [0].tow = StartLine + LineCount;
	Glide.sq [2].tmuvtx [0].sow = 0.0;
	Glide.sq [2].tmuvtx [0].tow = StartLine;
	Glide.sq [3].tmuvtx [0].sow = 8.0;
	Glide.sq [3].tmuvtx [0].tow = StartLine;
    }
    else
    {
	// Vertical flip only
	Glide.sq [0].tmuvtx [0].sow = 0.0;
	Glide.sq [0].tmuvtx [0].tow = StartLine + LineCount;
	Glide.sq [1].tmuvtx [0].sow = 8.0;
	Glide.sq [1].tmuvtx [0].tow = StartLine + LineCount;
	Glide.sq [2].tmuvtx [0].sow = 8.0;
	Glide.sq [2].tmuvtx [0].tow = StartLine;
	Glide.sq [3].tmuvtx [0].sow = 0.0;
	Glide.sq [3].tmuvtx [0].tow = StartLine;
    }
    grTexDownloadMipMapLevel (GR_TMU0, Glide.texture_mem_start,
			      GR_LOD_8, GR_LOD_8, GR_ASPECT_1x1,
			      GR_TEXFMT_RGB_565,
			      GR_MIPMAPLEVELMASK_BOTH,
			      (void *) pCache);
    grTexSource (GR_TMU0, Glide.texture_mem_start,
		 GR_MIPMAPLEVELMASK_BOTH, &Glide.texture);
    grDrawTriangle (&Glide.sq [0], &Glide.sq [3], &Glide.sq [2]);
    grDrawTriangle (&Glide.sq [0], &Glide.sq [1], &Glide.sq [2]);
}

void DrawClippedTile3dfx (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Width,
			  uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_SUB, WRITE_4PIXELS16_FLIPPED_SUB, 4)
}
#endif
#endif





//-------------------------------------------------------------------
// Optimizations
//-------------------------------------------------------------------

void GenerateTileMask(SBG* BG, uint8 *pCache, uint32 TileNumber)
{
    bool fullTile = TRUE;
    uint8 tileMask = 0;
    uint8 tileMaskFlipped = 0;
    
    for (int y = 0; y < 64; y += 8)
    {
        tileMask = 0;
        //tileMaskFlipped = 0;        
        for (int x = 0; x < 8; x ++)    
        {
            if (pCache[x + y])
            {
                tileMask |= (0x01 << x);
                //tileMaskFlipped |= (0x80 >> x);
            }
        }
        // BG->TileMask[TileNumber][y / 8] = tileMask;
        //BG->TileMaskFlipped[TileNumber][y / 8] = tileMaskFlipped;
        
        if (tileMask != 0xFF)
            fullTile = FALSE;
        /*
        BG->EightPixelWriter[TileNumber][y / 8] = Write8Pixels[tileMask];
        BG->EightPixelWriterFlipped[TileNumber][y / 8] = Write8PixelsFlipped[tileMaskFlipped];*/
    }
    BG->TileFull[TileNumber] = fullTile;
    
}



inline void __attribute__((always_inline)) WRITE_8PIXELS16 (uint32 Offset, uint8 *Pixels)
{
    register uint32 Pixel1;
    register uint32 Pixel2;
    register uint32 Pixel3;
    register uint32 Pixel4;
    
    uint16 *Pixels16 = (uint16 *)Pixels;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;

#define FN(N)\
    Pixel1 = Pixels16[N];\ 
    Pixel2 = Pixels16[N+1];\
    Pixel3 = Pixels16[N+2];\
    Pixel4 = Pixels16[N+3];\
    if (Pixel1) TILE_AssignPixel(N, Pixel1);\ 
    if (Pixel2) TILE_AssignPixel(N+1, Pixel2); \
    if (Pixel3) TILE_AssignPixel(N+2, Pixel3); \
    if (Pixel4) TILE_AssignPixel(N+3, Pixel4);
    
    FN(0)
    FN(4)
    
#undef FN
}

inline void __attribute__((always_inline)) WRITE_8PIXELS16_FULL (uint32 Offset, uint8 *Pixels)
{
    register uint32 PixelPair1;
    register uint32 PixelPair2;
    register uint32 PixelPair3;
    register uint32 PixelPair4;
    
    uint32 *Pixels32 = (uint32 *)Pixels;
    uint32 *Screen32 = (uint32 *)((uint16 *) GFX.S + Offset);
    uint8  *Depth = GFX.DB + Offset;

    PixelPair1 = Pixels32[0];
    PixelPair2 = Pixels32[1];
    PixelPair3 = Pixels32[2];
    PixelPair4 = Pixels32[3];
    
    TILE_AssignTwoPixels(0, PixelPair1);
    TILE_AssignTwoPixels(1, PixelPair2); 
    TILE_AssignTwoPixels(2, PixelPair3); 
    TILE_AssignTwoPixels(3, PixelPair4); 
}

inline void __attribute__((always_inline)) WRITE_8PIXELS16_FLIPPED (uint32 Offset, uint8 *Pixels)
{
    register uint32 Pixel1;
    register uint32 Pixel2;
    register uint32 Pixel3;
    register uint32 Pixel4;
    
    uint16 *Pixels16 = (uint16 *)Pixels;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;


#define FN(N)\
    Pixel1 = Pixels16[7-N];\ 
    Pixel2 = Pixels16[7-N-1];\
    Pixel3 = Pixels16[7-N-2];\
    Pixel4 = Pixels16[7-N-3];\
    if (Pixel1) TILE_AssignPixel(N, Pixel1);\ 
    if (Pixel2) TILE_AssignPixel(N+1, Pixel2); \
    if (Pixel3) TILE_AssignPixel(N+2, Pixel3); \
    if (Pixel4) TILE_AssignPixel(N+3, Pixel4);
    
    FN(0)
    FN(4)
#undef FN
}


inline void __attribute__((always_inline)) WRITE_8PIXELS16_FULL_FLIPPED (uint32 Offset, uint8 *Pixels)
{
    register uint32 PixelPair1;
    register uint32 PixelPair2;
    register uint32 PixelPair3;
    register uint32 PixelPair4;
    
    uint32 *Pixels32 = (uint32 *)Pixels;
    uint32 *Screen32 = (uint32 *)((uint16 *) GFX.S + Offset);
    uint8  *Depth = GFX.DB + Offset;

    PixelPair1 = (Pixels32[0] << 16) | (Pixels32[0] >> 16);
    PixelPair2 = (Pixels32[1] << 16) | (Pixels32[1] >> 16);
    PixelPair3 = (Pixels32[2] << 16) | (Pixels32[2] >> 16);
    PixelPair4 = (Pixels32[3] << 16) | (Pixels32[3] >> 16);
    
    TILE_AssignTwoPixels(0, PixelPair4);
    TILE_AssignTwoPixels(1, PixelPair3); 
    TILE_AssignTwoPixels(2, PixelPair2); 
    TILE_AssignTwoPixels(3, PixelPair1); 
}



inline void __attribute__((always_inline)) WRITE_8PIXELS16_CLIPPED (uint32 Offset, uint8 *Pixels, uint8 StartPixel, uint8 Width)
{
    register uint32 Pixel1;
    uint16 *Pixels16 = &((uint16 *)Pixels)[StartPixel];
    uint16 *Screen = (uint16 *) GFX.S + Offset + StartPixel;

#define FN(N) Pixel1 = Pixels16[N]; if (Pixel1) TILE_AssignPixel(N, Pixel1); 
    
    switch (Width)
    {
        case 8: FN(7)
        case 7: FN(6)
        case 6: FN(5)
        case 5: FN(4)
        case 4: FN(3)
        case 3: FN(2)
        case 2: FN(1)
        case 1: FN(0)
    }
#undef FN
}


inline void __attribute__((always_inline)) WRITE_8PIXELS16_CLIPPED_FLIPPED (uint32 Offset, uint8 *Pixels, uint8 StartPixel, uint8 Width)
{
    register uint32 Pixel1;
    uint16 *Pixels16 = &((uint16 *)Pixels)[7 - StartPixel];
    uint16 *Screen = (uint16 *) GFX.S + Offset + StartPixel;

#define FN(N) Pixel1 = Pixels16[- N]; if (Pixel1) TILE_AssignPixel(N, Pixel1); 
    
    switch (Width)
    {
        case 8: FN(7)       
        case 7: FN(6)
        case 6: FN(5)
        case 5: FN(4)
        case 4: FN(3)
        case 3: FN(2)
        case 2: FN(1)
        case 1: FN(0)
    }
#undef FN
}



void DrawTileBG16 (uint32 Tile, uint32 Offset, uint32 StartLine,
	         uint32 LineCount)
{
    TILE_PREAMBLE2(true)
    register uint8 *bp;

    RENDER_TILE_FAST(WRITE_8PIXELS16, WRITE_8PIXELS16_FLIPPED, WRITE_8PIXELS16_FULL, WRITE_8PIXELS16_FULL_FLIPPED)
}

void DrawClippedTileBG16 (uint32 Tile, uint32 Offset,
			uint32 StartPixel, uint32 Width,
			uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE2(true)
    register uint8 *bp;

    RENDER_CLIPPED_TILE_FAST(WRITE_8PIXELS16_CLIPPED, WRITE_8PIXELS16_CLIPPED_FLIPPED)
}

void DrawTileOBJ16 (uint32 Tile, uint32 Offset, uint32 StartLine,
	         uint32 LineCount)
{
    TILE_PREAMBLE2(false)
    register uint8 *bp;

    RENDER_TILE_FAST(WRITE_8PIXELS16, WRITE_8PIXELS16_FLIPPED, WRITE_8PIXELS16_FULL, WRITE_8PIXELS16_FULL_FLIPPED)
}

void DrawClippedTileOBJ16 (uint32 Tile, uint32 Offset,
			uint32 StartPixel, uint32 Width,
            uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE2(false)
    register uint8 *bp;

    RENDER_CLIPPED_TILE_FAST(WRITE_8PIXELS16_CLIPPED, WRITE_8PIXELS16_CLIPPED_FLIPPED)
}
