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
#include "cpuops.h"
#include "ppu.h"
#include "cpuexec.h"
#include "debug.h"
#include "snapshot.h"
#include "gfx.h"
#include "missing.h"
#include "apu.h"
#include "dma.h"
#include "fxemu.h"
#include "sa1.h"
#include "spc7110.h"

#include "3dsgpu.h"
#include "3dsopt.h"
#include "3dssnes9x.h"

extern struct SSA1 SA1;

#define CPUCYCLES_REGISTERS
#define OPCODE_REGISTERS


#ifdef OPCODE_REGISTERS
register SOpcodes *fastOpcodes asm ("r11");
#endif

#ifdef CPUCYCLES_REGISTERS

register long fastCPUCycles asm ("r10");
register uint8 *fastCPUPC asm ("r9");

#define CPU_Cycles 		fastCPUCycles
#define CPU_PC   		fastCPUPC

#else

#define CPU_Cycles   	CPU.Cycles
#define CPU_PC   		CPU.PC

#endif


#include "cpuexec-ops.cpp"

inline void __attribute__ ((always_inline)) S9xHandleFlags()
{
	if (CPU.Flags & NMI_FLAG)
	{
		if (--CPU.NMICycleCount == 0)
		{
			CPU.Flags &= ~NMI_FLAG;
			//if (CPU.WaitingForInterrupt)
			//{
			//	CPU.WaitingForInterrupt = FALSE;
			//	CPU_PC++;
			//}
			S9xOpcode_NMI ();
		}
	}	
	
	if (CPU.Flags & IRQ_PENDING_FLAG)
	{
		if (CPU.IRQCycleCount == 0)
		{
			//printf ("CPU.IRQActive:%d\n", CPU.IRQActive);
			//if (CPU.WaitingForInterrupt)
			//{
			//	CPU.WaitingForInterrupt = FALSE;
			//	CPU_PC++;
			//} 

			// Rightfully, CPU.IRQActive will be non zero if CPU.Flags & IRQ_PENDING_FLAG
			// is non-zero, so there is a small area of optimization here.
			//
			//if (CPU.IRQActive/* && !Settings.DisableIRQ*/)
			{
				if (!CheckFlag (IRQ))
				{
					S9xOpcode_IRQ ();
				}
			}
			//else		
			//	CPU.Flags &= ~IRQ_PENDING_FLAG;
		}
		else
		{
			// Since we don't have to check CPU.WaitingForInterrupt,
			// just go ahead and decrement the counter. At the very most
			// the IRQ will not jump to the handler if the IRQ flag is set.
			//
			//if(--CPU.IRQCycleCount==0 && CheckFlag (IRQ))
			//	CPU.IRQCycleCount=1;
			CPU.IRQCycleCount--;
		}
	}	
}


void S9xDoHBlankProcessingWithRegisters()
{
	CpuSaveFastRegisters();
	S9xDoHBlankProcessing();
	CpuLoadFastRegisters();	
}


// Doing the HBlank check before Adding MemSpeed seems to improve performance,
// but hopefully we do not break any timing problems...
//
//	printf ("PC:%x A:%x X:%x Y:%x OP:%x\n", CPU_PC - CPU.PCBase, Registers.A.W, Registers.X.W, Registers.Y.W, *CPU_PC); 
/*
	if (CPU_PC - CPU.PCBase == 0xd9ff) GPU3DS.enableDebug = true; \
	if (GPU3DS.enableDebug) \
	{ \
		printf ("PC :%x OP:%x%2x%2x%2x HV:%d,%d\n   A%04x X%04x Y%04x 2140:%02x\n", CPU_PC - CPU.PCBase, *CPU_PC, *(CPU_PC + 1), *(CPU_PC + 2), *(CPU_PC + 3), (int)CPU_Cycles, (int)CPU.V_Counter, Registers.A.W, Registers.X.W, Registers.Y.W, IAPU.RAM [0xf4] );  \
	} \
	if (GPU3DS.enableDebug) \
		goto S9xMainLoop_EndFrame; 

*/


//---------------------------------------------------------------
// For debugging purposes.
//---------------------------------------------------------------
#ifndef DEBUG_CPU

	#define DEBUG_OUTPUT

#else

	char debugLine[500];

/*
	#define DEBUG_OUTPUT \
		if (true) \
		{ \
			CpuSaveFastRegisters(); \
			S9xOPrintLong (debugLine, (uint8) Registers.PB, (uint16) (CPU_PC - CPU.PCBase)); \
			FILE *fp = fopen("cpu.log", "a"); \
			fprintf (fp, "%s\n", debugLine); \
			fclose (fp); \ 
			CpuLoadFastRegisters(); \
			goto S9xMainLoop_EndFrame; \ 
		} \
*/

	#define DEBUG_OUTPUT \
		if (GPU3DS.enableDebug && !Settings.Paused) \
		{ \
			CpuSaveFastRegisters(); \
			printf ("\n"); \
			S9xOPrint (debugLine, (uint8) Registers.PB, (uint16) (CPU_PC - CPU.PCBase)); \
			printf ("%s", debugLine); \
			CpuLoadFastRegisters(); \
			goto S9xMainLoop_EndFrame; \ 
		} \

#endif

#ifdef OPCODE_REGISTERS

#define EXECUTE_ONE_OPCODE(SupportSA1) \
	if (CPU_Cycles >= CPU.NextEvent) S9xDoHBlankProcessingWithRegisters(); \
	CPU_Cycles += CPU.MemSpeed; \
	if (!SupportSA1) \
	{ \
		(*fastOpcodes [*CPU_PC++].S9xOpcode) (); \
		if (CPU.Flags) goto S9xMainLoop_HandleFlags; \
	} \
	else \
	{ \
		(*ICPU.S9xOpcodes [*CPU_PC++].S9xOpcode) (); \
		if (SA1.Executing) \
		{ \
			if (SA1.Flags & IRQ_PENDING_FLAG) S9xSA1CheckIRQ(); \
			(*SA1.S9xOpcodes [*SA1.PC++].S9xOpcode) (); \
			(*SA1.S9xOpcodes [*SA1.PC++].S9xOpcode) (); \
			(*SA1.S9xOpcodes [*SA1.PC++].S9xOpcode) (); \
		} \
		if (CPU.Flags) goto S9xMainLoop_HandleFlags; \
	} \
	DEBUG_OUTPUT

#else

#define EXECUTE_ONE_OPCODE(SupportSA1) \
	if (CPU_Cycles >= CPU.NextEvent) S9xDoHBlankProcessingWithRegisters(); \
	CPU_Cycles += CPU.MemSpeed; \
	(*ICPU.S9xOpcodes [*CPU_PC++].S9xOpcode) (); \
	if (CPU.Flags) goto S9xMainLoop_HandleFlags; 

#endif

#define EXECUTE_TEN_OPCODES(SupportSA1) \
	{ \
		EXECUTE_ONE_OPCODE (SupportSA1) \
		EXECUTE_ONE_OPCODE (SupportSA1) \
		EXECUTE_ONE_OPCODE (SupportSA1) \
		EXECUTE_ONE_OPCODE (SupportSA1) \
		EXECUTE_ONE_OPCODE (SupportSA1) \
		EXECUTE_ONE_OPCODE (SupportSA1) \
		EXECUTE_ONE_OPCODE (SupportSA1) \
		EXECUTE_ONE_OPCODE (SupportSA1) \
		EXECUTE_ONE_OPCODE (SupportSA1) \
		EXECUTE_ONE_OPCODE (SupportSA1) \
	}

// New Loop
// The unrolled loop runs faster.
void S9xMainLoop (void)
{
	asm ("stmfd	sp!, {r8, r9, r10, r11}");

	// For some reason, there seems to be a bug in GCC when
	// reserving global registers + pushing of those registers
	// on the stack (could it be due to some stack buffer overrrun?)
	// prevent registers from getting overwritten.
	// 
	asm ("sub sp, sp, #32");				

	CpuLoadFastRegisters();
	
#ifdef OPCODE_REGISTERS
	fastOpcodes = ICPU.S9xOpcodes;
#endif

    for (;;)
    {
S9xMainLoop_Execute:
		//EXECUTE_ONE_OPCODE
		EXECUTE_TEN_OPCODES (false)
		EXECUTE_TEN_OPCODES (false)
	} 

S9xMainLoop_HandleFlags:
	// Bug fix: Removed save/load fast registers.
	S9xHandleFlags(); 
	DEBUG_OUTPUT

	if (!(CPU.Flags & SCAN_KEYS_FLAG)) 
		goto S9xMainLoop_Execute;
	goto S9xMainLoop_EndFrame;

    
S9xMainLoop_EndFrame:
	// End of current frame, prepare to return to caller
	//
    Registers.PC = CPU_PC - CPU.PCBase;
    S9xPackStatus ();
    APURegisters.PC = IAPU.PC - IAPU.RAM;
    S9xAPUPackStatus ();
    if (CPU.Flags & SCAN_KEYS_FLAG)
    {
	    S9xSyncSpeed ();
		CPU.Flags &= ~SCAN_KEYS_FLAG;
    }
	
	CpuSaveFastRegisters();

	// For some reason, there seems to be a bug in GCC when
	// reserving global registers + pushing of those registers
	// on the stack (could it be due to some stack buffer overrrun?)
	// prevent registers from getting overwritten.
	// 
	asm ("add sp, sp, #32");

	asm ("ldmfd	sp!, {r8, r9, r10, r11}");
}



// Special loop for SA1 support.
//
void S9xMainLoopWithSA1 (void)
{
	asm ("stmfd	sp!, {r8, r9, r10, r11}");

	// For some reason, there seems to be a bug in GCC when
	// reserving global registers + pushing of those registers
	// on the stack (could it be due to some stack buffer overrrun?)
	// prevent registers from getting overwritten.
	// 
	asm ("sub sp, sp, #32");				

	CpuLoadFastRegisters();

    for (;;)
    {
S9xMainLoop_Execute:
		EXECUTE_TEN_OPCODES (true)
		EXECUTE_TEN_OPCODES (true)
	} 

S9xMainLoop_HandleFlags:
	// Bug fix: Removed save/load fast registers.
	S9xHandleFlags(); 

	DEBUG_OUTPUT
	if (!(CPU.Flags & SCAN_KEYS_FLAG)) 
		goto S9xMainLoop_Execute;
	goto S9xMainLoop_EndFrame;

    
S9xMainLoop_EndFrame:
	// End of current frame, prepare to return to caller
	//
    Registers.PC = CPU_PC - CPU.PCBase;
    S9xPackStatus ();
    APURegisters.PC = IAPU.PC - IAPU.RAM;
    S9xAPUPackStatus ();
    if (CPU.Flags & SCAN_KEYS_FLAG)
    {
	    S9xSyncSpeed ();
		CPU.Flags &= ~SCAN_KEYS_FLAG;
    }
	
	CpuSaveFastRegisters();

	// For some reason, there seems to be a bug in GCC when
	// reserving global registers + pushing of those registers
	// on the stack (could it be due to some stack buffer overrrun?)
	// prevent registers from getting overwritten.
	// 
	asm ("add sp, sp, #32");

	asm ("ldmfd	sp!, {r8, r9, r10, r11}");
}


void S9xSetIRQ (uint32 source)
{
    CPU.IRQActive |= source;
    CPU.Flags |= IRQ_PENDING_FLAG;

	// For most games, IRQCycleCount should be set to 3.
	// But for Mighty Morphin Power Rangers Fighting Edition, we
	// must set to 0.
	//
    CPU.IRQCycleCount = SNESGameFixes.IRQCycleCount;


	/*
    if (CPU.WaitingForInterrupt)
    {
		// Force IRQ to trigger immediately after WAI - 
		// Final Fantasy Mystic Quest crashes without this.
		CPU.IRQCycleCount = 0;

		// Bug Fix: Since the order of execution in the MainLoop has shifted, 
		// we can't do a CPU.PC++ here. Otherwise FF Mystic Quest will crash.
		//
		//CPU.WaitingForInterrupt = FALSE;
		//CPU.PC++;
    }*/
}

void S9xClearIRQ (uint32 source)
{
    CLEAR_IRQ_SOURCE (source);
}

int debugFrameCounter = 0;

void S9xDoHBlankProcessing ()
{
#ifdef CPU_SHUTDOWN
    CPU.WaitCounter++;
#endif
    switch (CPU.WhichEvent)
    {
		/*-----------------------------------------------------*/
		case HBLANK_START_EVENT:
			if (IPPU.HDMA && CPU.V_Counter <= PPU.ScreenHeight)
				IPPU.HDMA = S9xDoHDMA (IPPU.HDMA);

			break;

		/*-----------------------------------------------------*/
    	case HBLANK_END_EVENT:
			S9xSuperFXExec ();
			
			//t3dsStartTiming(21, "APU_EXECUTE");
			S9xUpdateAPUTimer();
			//APU_EXECUTE();
			//t3dsEndTiming(21);

			static const int addr[] = { 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31 };

			// Optimization, for a small number of PPU registers,
			// we will trigger the FLUSH_REDRAW here instead of
			// us doing it when the register values change. This is
			// because some games like FF3 and Ace o Nerae changes
			// the registers multiple times in the same scanline,
			// even though the end value is the same by the end of
			// the scanline. But because they modify the registers,
			// the rendering is forced to do a FLUSH_REDRAW.
			//
			// In this optimization, we simply defer the FLUSH_REDRAW
			// until this point here and only when we determine that 
			// at least one of the registers have changed from the
			// value in the previous scanline.
			//
			if (SNESGameFixes.AceONeraeHack)
			{
				for (int i = 0; i < sizeof(addr) / sizeof(int); i++)
				{
					int a = addr[i];
					if (IPPU.DeferredRegisterWrite[a] != 0xff00 &&
						IPPU.DeferredRegisterWrite[a] != Memory.FillRAM[a + 0x2100])
					{
						//printf ("$21%02x = %02x, Window 2126-29: %02x %02x %02x %02x\n", a, IPPU.DeferredRegisterWrite[a], 
						//	Memory.FillRAM[0x2126], Memory.FillRAM[0x2127], Memory.FillRAM[0x2128], Memory.FillRAM[0x2129]);

						if (a == 0x31 &&
							Memory.FillRAM[0x2126] == 0xFF && Memory.FillRAM[0x2127] == 0x00 &&
							Memory.FillRAM[0x2128] == 0xFF && Memory.FillRAM[0x2129] == 0x00)
						{
							Memory.FillRAM[a + 0x2100] = IPPU.DeferredRegisterWrite[a];
							continue;
						}
						DEBUG_FLUSH_REDRAW(a + 0x2100, IPPU.DeferredRegisterWrite[a]);
						FLUSH_REDRAW();
						Memory.FillRAM[a + 0x2100] = IPPU.DeferredRegisterWrite[a];
					}
					IPPU.DeferredRegisterWrite[a] = 0xff00;
				}
			}
			else
			{
				for (int i = 0; i < sizeof(addr) / sizeof(int); i++)
				{
					int a = addr[i];
					if (IPPU.DeferredRegisterWrite[a] != 0xff00 &&
						IPPU.DeferredRegisterWrite[a] != Memory.FillRAM[a + 0x2100])
					{
						DEBUG_FLUSH_REDRAW(a + 0x2100, IPPU.DeferredRegisterWrite[a]);
						FLUSH_REDRAW();
						Memory.FillRAM[a + 0x2100] = IPPU.DeferredRegisterWrite[a];
					}
					IPPU.DeferredRegisterWrite[a] = 0xff00;
				}
			}

		#ifndef STORM
			if (Settings.SoundSync)
				S9xGenerateSound ();
		#endif

			CPU.PrevCycles -= Settings.H_Max;
			CPU.Cycles -= Settings.H_Max;
			IAPU.NextAPUTimerPos -= (Settings.H_Max * 1000L);
			IAPU.NextAPUTimerPosDiv10000 = IAPU.NextAPUTimerPos / 1000;
			if (IAPU.APUExecuting)
			{
				APU.Cycles -= Settings.H_Max;
		#ifdef MK_APU
				S9xCatchupCount();
		#endif
			}
			else
				APU.Cycles = 0;

			CPU.NextEvent = -1;
			ICPU.Scanline++;

			if (++CPU.V_Counter >= (Settings.PAL ? SNES_MAX_PAL_VCOUNTER : SNES_MAX_NTSC_VCOUNTER))
			{
				CPU.V_Counter = 0;
				Memory.FillRAM[0x213F]^=0x80;
				PPU.RangeTimeOver = 0;
				CPU.NMIActive = FALSE;
				ICPU.Frame++;
				PPU.HVBeamCounterLatched = 0;
				CPU.Flags |= SCAN_KEYS_FLAG;
				S9xStartHDMA ();
			}

			if (PPU.VTimerEnabled && !PPU.HTimerEnabled &&
				CPU.V_Counter == PPU.IRQVBeamPos)
			{
				S9xSetIRQ (PPU_V_BEAM_IRQ_SOURCE);
			}

			// Hack for committing palettes at a specific line.
			//
			if (CPU.V_Counter == SNESGameFixes.PaletteCommitLine)
			{
				S9xUpdateVerticalSectionValue(&IPPU.BackdropColorSections, IPPU.ScreenColors[0]);
				S9xUpdatePalettes();				
			}

			if (CPU.V_Counter == PPU.ScreenHeight + FIRST_VISIBLE_LINE)
			{
#ifdef DEBUG_CPU
				//printf ("debug frame counter: %d\n", debugFrameCounter);
				debugFrameCounter++;
#endif				
				// Start of V-blank
				S9xEndScreenRefresh ();
				IPPU.HDMA = 0;
				// Bits 7 and 6 of $4212 are computed when read in S9xGetPPU.
				missing.dma_this_frame = 0;
				IPPU.MaxBrightness = PPU.Brightness;
				PPU.ForcedBlanking = (Memory.FillRAM [0x2100] >> 7) & 1;

				if(!PPU.ForcedBlanking){
					PPU.OAMAddr = PPU.SavedOAMAddr;
					{
						uint8 tmp = 0;
						if(PPU.OAMPriorityRotation)
							tmp = (PPU.OAMAddr&0xFE)>>1;
						if((PPU.OAMFlip&1) || PPU.FirstSprite!=tmp){
							PPU.FirstSprite=tmp;
							IPPU.OBJChanged=TRUE;
						}
					}
					PPU.OAMFlip = 0;
				}

				Memory.FillRAM[0x4210] = 0x80 |Model->_5A22;
				if (Memory.FillRAM[0x4200] & 0x80)
				{
					CPU.NMIActive = TRUE;
					CPU.Flags |= NMI_FLAG;
					CPU.NMICycleCount = CPU.NMITriggerPoint;
				}

		#ifdef OLD_SNAPSHOT_CODE
				if (CPU.Flags & SAVE_SNAPSHOT_FLAG)
				{
					CPU.Flags &= ~SAVE_SNAPSHOT_FLAG;
					Registers.PC = CPU.PC - CPU.PCBase;
					S9xPackStatus ();
					S9xAPUPackStatus ();
					Snapshot (NULL);
				}
		#endif
			}

			if (CPU.V_Counter == PPU.ScreenHeight + 3)
				S9xUpdateJoypads ();

			if (CPU.V_Counter == FIRST_VISIBLE_LINE)
			{
				Memory.FillRAM[0x4210] = Model->_5A22;
				CPU.Flags &= ~NMI_FLAG;
				S9xStartScreenRefresh ();
			}

			if (CPU.V_Counter >= FIRST_VISIBLE_LINE &&
				CPU.V_Counter < PPU.ScreenHeight + FIRST_VISIBLE_LINE)
			{
				RenderLine (CPU.V_Counter - FIRST_VISIBLE_LINE);
			}
			break;

		/*-----------------------------------------------------*/
		case HTIMER_BEFORE_EVENT:
		case HTIMER_AFTER_EVENT:
			if (PPU.HTimerEnabled &&
				(!PPU.VTimerEnabled || CPU.V_Counter == PPU.IRQVBeamPos))
			{
				S9xSetIRQ (PPU_H_BEAM_IRQ_SOURCE);
			}
			break;

	}
    S9xReschedule ();
}




