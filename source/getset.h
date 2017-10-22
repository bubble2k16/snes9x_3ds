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

#ifndef _GETSET_H_
#define _GETSET_H_

#include "ppu.h"
#include "dsp1.h"
#include "cpuexec.h"
#include "sa1.h"
#include "spc7110.h"
#include "obc1.h"
#include "seta.h"
#include "bsx.h"

#include "3dsopt.h"
#include "hwregisters.h"

//extern "C"
//{
//	extern uint8 OpenBus;
//}
#define OpenBus OCPU.FastOpenBus


INLINE uint8 __attribute__((always_inline)) S9xGetByteNoCycles (uint32 Address)
{
		//t3dsCount (41, "S9xGetByte");
	
    int block;
    uint8 *GetAddress = CPU.MemoryMap [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];

		if ((intptr_t) GetAddress != Memory.MAP_CPU || !CPU.InDMA)
        CPU.Cycles += Memory.MemorySpeed [block];

    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
				return (*(GetAddress + (Address & 0xffff)));
		else 
				return S9xGetByteFromRegister(GetAddress, Address);
}




INLINE uint16 __attribute__((always_inline)) S9xGetWordNoCycles (uint32 Address)
{
		//t3dsCount (42, "S9xGetWord");
	
    if ((Address & 0x0fff) != 0x0fff)
		{
    		int block;
    		uint8 *GetAddress = CPU.MemoryMap [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];

				if ((intptr_t) GetAddress != Memory.MAP_CPU || !CPU.InDMA)
  	  	    CPU.Cycles += Memory.MemorySpeed [block];

				if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
  			{
#ifdef CPU_SHUTDOWN
						if (Memory.BlockIsRAM [block])
								CPU.WaitAddress = CPU.PCAtOpcodeStart;
#endif
#ifdef FAST_LSB_WORD_ACCESS
						return (*(uint16 *) (GetAddress + (Address & 0xffff)));
#else
						return (*(GetAddress + (Address & 0xffff)) | (*(GetAddress + (Address & 0xffff) + 1) << 8));
#endif	
    		}
				return S9xGetWordFromRegister(GetAddress, Address);
		}
		else
    {
				OpenBus = S9xGetByteNoCycles (Address);
				return (OpenBus | (S9xGetByteNoCycles (Address + 1) << 8));
    }
}



INLINE void __attribute__((always_inline)) S9xSetByteNoCycles (uint8 Byte, uint32 Address)
{
		//t3dsCount (43, "S9xSetByte");
	
#if defined(CPU_SHUTDOWN)
    CPU.WaitAddress = NULL;
#endif
    int block;
    uint8 *SetAddress = CPU.MemoryWriteMap [block = ((Address >> MEMMAP_SHIFT) & MEMMAP_MASK)];

		if ((intptr_t) SetAddress != Memory.MAP_CPU || !CPU.InDMA)
        CPU.Cycles += Memory.MemorySpeed [block];

    if (SetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef CPU_SHUTDOWN
				SetAddress += Address & 0xffff;
				if (SetAddress == SA1.WaitByteAddress1 ||
						SetAddress == SA1.WaitByteAddress2)
				{
						SA1.Executing = SA1.S9xOpcodes != NULL;
						SA1.WaitCounter = 0;
				}
				*SetAddress = Byte;
#else
				*(SetAddress + (Address & 0xffff)) = Byte;
#endif
				return;
    }
		S9xSetByteToRegister(Byte, SetAddress, Address);
}


INLINE void __attribute__((always_inline)) S9xSetWordNoCycles (uint16 Word, uint32 Address)
{
		//t3dsCount (44, "S9xSetWord");
	
		if((Address & 0x0FFF)==0x0FFF)
		{
				S9xSetByteNoCycles(Word&0x00FF, Address);
				S9xSetByteNoCycles(Word>>8, Address+1);
				return;
		}

#if defined(CPU_SHUTDOWN)
    CPU.WaitAddress = NULL;
#endif
    int block;
    uint8 *SetAddress = CPU.MemoryWriteMap [block = ((Address >> MEMMAP_SHIFT) & MEMMAP_MASK)];

		if ((intptr_t) SetAddress != Memory.MAP_CPU || !CPU.InDMA)
        CPU.Cycles += Memory.MemorySpeed [block];
				
    if (SetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef CPU_SHUTDOWN
				SetAddress += Address & 0xffff;
				if (SetAddress == SA1.WaitByteAddress1 ||
						SetAddress == SA1.WaitByteAddress2)
				{
						SA1.Executing = SA1.S9xOpcodes != NULL;
						SA1.WaitCounter = 0;
				}
#ifdef FAST_LSB_WORD_ACCESS
				*(uint16 *) SetAddress = Word;
#else
				*SetAddress = (uint8) Word;
				*(SetAddress + 1) = Word >> 8;
#endif
#else
#ifdef FAST_LSB_WORD_ACCESS
				*(uint16 *) (SetAddress + (Address & 0xffff)) = Word;
#else
				*(SetAddress + (Address & 0xffff)) = (uint8) Word;
				*(SetAddress + ((Address + 1) & 0xffff)) = Word >> 8;
#endif
#endif
				return;
    }	
		
		S9xSetWordToRegister(Word, SetAddress, Address);
}



INLINE uint8 *GetBasePointer (uint32 Address)
{
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
		return (GetAddress);
	//if(Settings.SPC7110&&((Address&0x7FFFFF)==0x4800))
	//{
	//	return s7r.bank50;
	//}
    switch ((int) GetAddress)
    {
	/*case CMemory::MAP_SPC7110_DRAM:
#ifdef SPC7110_DEBUG
		printf("Getting Base pointer to DRAM\n");
#endif
		{
			//return s7r.bank50;
		}*/
	case CMemory::MAP_SPC7110_ROM:
#ifdef SPC7110_DEBUG
		printf("Getting Base pointer to SPC7110ROM\n");
#endif
		return Get7110BasePtr(Address);
    case CMemory::MAP_PPU:
//just a guess, but it looks like this should match the CPU as a source.
		return (Memory.FillRAM);
//		return (Memory.FillRAM - 0x2000);
    case CMemory::MAP_CPU:
//fixes Ogre Battle's green lines
		return (Memory.FillRAM);
//		return (Memory.FillRAM - 0x4000);
    case CMemory::MAP_DSP:
		return (Memory.FillRAM - 0x6000);
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		return (Memory.SRAM);
    case CMemory::MAP_BWRAM:
		return (Memory.BWRAM - 0x6000);
    case CMemory::MAP_HIROM_SRAM:
		return (Memory.SRAM - 0x6000);
    case CMemory::MAP_C4:
		return (Memory.C4RAM - 0x6000);
	case CMemory::MAP_OBC_RAM:
		return GetBasePointerOBC1(Address);
	case CMemory::MAP_SETA_DSP:
		return Memory.SRAM;
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("GBP %06x\n", Address);
#endif
		
    default:
    case CMemory::MAP_NONE:
#if defined(MK_TRACE_BAD_READS) || defined(MK_TRACE_BAD_WRITES)
		char fsd[12];
		sprintf(fsd, TEXT("%06X"), Address);
		MessageBox(GUI.hWnd, fsd, TEXT("Rogue DMA"), MB_OK);
#endif

#ifdef DEBUGGER
		printf ("GBP %06x\n", Address);
#endif
		return (0);
    }
}

INLINE uint8 *S9xGetMemPointer (uint32 Address)
{
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
		return (GetAddress + (Address & 0xffff));
	
	//if(Settings.SPC7110&&((Address&0x7FFFFF)==0x4800))
	//	return s7r.bank50;

    switch ((int) GetAddress)
    {
	case CMemory::MAP_SPC7110_DRAM:
#ifdef SPC7110_DEBUG
		printf("Getting Base pointer to DRAM\n");
#endif
		//return &s7r.bank50[Address&0x0000FFFF];
    case CMemory::MAP_PPU:
		return (Memory.FillRAM + (Address & 0xffff));
    case CMemory::MAP_CPU:
		return (Memory.FillRAM + (Address & 0xffff));
    case CMemory::MAP_DSP:
		return (Memory.FillRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		return (Memory.SRAM + (Address & 0xffff));
    case CMemory::MAP_BWRAM:
		return (Memory.BWRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_HIROM_SRAM:
		return (Memory.SRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_C4:
		return (Memory.C4RAM - 0x6000 + (Address & 0xffff));
	case CMemory::MAP_OBC_RAM:
		return GetMemPointerOBC1(Address);
	case CMemory::MAP_SETA_DSP:
		return Memory.SRAM+ ((Address & 0xffff) & Memory.SRAMMask);
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("GMP %06x\n", Address);
#endif
    default:
    case CMemory::MAP_NONE:
#if defined(MK_TRACE_BAD_READS) || defined(MK_TRACE_BAD_WRITES)
		char fsd[12];
		sprintf(fsd, TEXT("%06X"), Address);
		MessageBox(GUI.hWnd, fsd, TEXT("Rogue DMA"), MB_OK);
#endif

#ifdef DEBUGGER
		printf ("GMP %06x\n", Address);
#endif
		return (0);
    }
}

INLINE void S9xSetPCBase (uint32 Address)
{
    int block;
    uint8 *GetAddress = Memory.Map [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];

	CPU.MemSpeed = Memory.MemorySpeed [block];
	CPU.MemSpeedx2 = CPU.MemSpeed << 1;
 
   if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
		CPU.PCBase = GetAddress;
		CPU.PC = GetAddress + (Address & 0xffff);
		return;
    }
	
    switch ((int) GetAddress)
    {
    case CMemory::MAP_PPU:
		CPU.PCBase = Memory.FillRAM;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_CPU:
		CPU.PCBase = Memory.FillRAM;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_DSP:
		CPU.PCBase = Memory.FillRAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		CPU.PCBase = Memory.SRAM;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_BWRAM:
		CPU.PCBase = Memory.BWRAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
    case CMemory::MAP_HIROM_SRAM:
		CPU.PCBase = Memory.SRAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_C4:
		CPU.PCBase = Memory.C4RAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
		case CMemory::MAP_BSX:
			CPU.PCBase = S9xGetBasePointerBSX(Address);
			return;
		
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("SBP %06x\n", Address);
#endif
		
    default:
    case CMemory::MAP_NONE:
#ifdef DEBUGGER
		printf ("SBP %06x\n", Address);
#endif
		CPU.PCBase = Memory.SRAM;
		CPU.PC = Memory.SRAM + (Address & 0xffff);
		return;
    }
}
#endif

