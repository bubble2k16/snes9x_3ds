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
#include "..\directx.h"
#include "..\Render.h"
#include "memmap.h"
#include "debug.h"
#include "cpuexec.h"
#include "ppu.h"
#include "snapshot.h"
#include "apu.h"
#include "display.h"
#include "gfx.h"
#include "soundux.h"
#include "netplay.h"
#include "movie.h"
#include "..\AVIOutput.h"
#include "..\language.h"

#include <direct.h>

#include <io.h>
//#define DEBUGGER
#ifdef FMOD_SUPPORT
#include "fmod.h"
#endif

//#define GENERATE_OFFSETS_H

#ifdef GENERATE_OFFSETS_H
FILE *offsets_h = NULL;
#define main generate_offsets_h
#define S9xSTREAM offsets_h
#include "offsets.cpp"
#endif

struct SJoyState Joystick [16];
uint32 joypads [5];
bool8 do_frame_adjust=false;

// avi variables
static uint8* avi_buffer = NULL;
static uint8* avi_sound_buffer = NULL;
static int avi_sound_bytes_per_sample = 0;
static int avi_sound_samples_per_update = 0;
static int avi_width = 0;
static int avi_height = 0;
static uint32 avi_skip_frames = 0;
void Convert8To24Packed (SSurface *src, SSurface *dst, RECT *srect);
void Convert16To24Packed (SSurface *src, SSurface *dst, RECT *srect);
void DoAVIOpen(const char* filename);
void DoAVIClose(int reason);
static void DoAVIVideoFrame(SSurface* source_surface, int width, int height, bool8 sixteen_bit);

static void S9xWinScanJoypads ();

typedef struct
{
    uint8 red;
    uint8 green;
    uint8 blue;
} Colour;

void ConvertDepth (SSurface *src, SSurface *dst, RECT *);
static Colour FixedColours [256];
static uint8 palette [0x10000];

FILE *trace_fs = NULL;

int __fastcall Normalize (int cur, int min, int max)
{
    int Result = 0;

    if ((max - min) == 0)
        return (Result);

    Result = cur - min;
    Result = (Result * 200) / (max - min);
    Result -= 100;

    return (Result);
}

void S9xTextMode( void)
{
}

void S9xGraphicsMode ()
{
}

void S9xExit( void)
{
	if(Settings.SPC7110)
		(*CleanUp7110)();
    SendMessage (GUI.hWnd, WM_COMMAND, ID_FILE_EXIT, 0);
}

extern "C" const char *S9xGetFilename (const char *e)
{
    static char filename [_MAX_PATH + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];

    if (strlen (GUI.FreezeFileDir))
    {
        _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
        strcpy (filename, GUI.FreezeFileDir);
        strcat (filename, TEXT("\\"));
        strcat (filename, fname);
        strcat (filename, e);
    }
    else
    {
        _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
        _makepath (filename, drive, dir, fname, e);
    }

    return (filename);
}

const char *S9xGetFilenameInc (const char *e)
{
    static char filename [_MAX_PATH + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
    char *ptr;

    if (strlen (GUI.FreezeFileDir))
    {
        _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
        strcpy (filename, GUI.FreezeFileDir);
        strcat (filename, TEXT("\\"));
        strcat (filename, fname);
        ptr = filename + strlen (filename);
        strcat (filename, TEXT("00/"));
        strcat (filename, e);
    }
    else
    {
        _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
        strcat (fname, TEXT("00/"));
        _makepath (filename, drive, dir, fname, e);
        ptr = strstr (filename, TEXT("00/"));
    }

    do
    {
        if (++*(ptr + 2) > '9')
        {
            *(ptr + 2) = '0';
            if (++*(ptr + 1) > '9')
            {
                *(ptr + 1) = '0';
                if (++*ptr > '9')
                    break;
            }
        }
    } while (_access (filename, 0) == 0);

    return (filename);
}

bool8 S9xOpenSnapshotFile( const char *fname, bool8 read_only, STREAM *file)
{
    char filename [_MAX_PATH + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char fn [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];

    _splitpath( fname, drive, dir, fn, ext);
    _makepath( filename, drive, dir, fn, ext[0] == '\0' ? ".000" : ext);

    if (read_only)
    {
	if ((*file = OPEN_STREAM (filename, "rb")))
	    return (TRUE);
    }
    else
    {
	if ((*file = OPEN_STREAM (filename, "wb")))
	    return (TRUE);
        FILE *fs = fopen (filename, "rb");
        if (fs)
        {
            sprintf (String, "Freeze file \"%s\" exists but is read only",
                     filename);
            fclose (fs);
            S9xMessage (S9X_ERROR, S9X_FREEZE_FILE_NOT_FOUND, String);
        }
        else
        {
            sprintf (String, "Cannot create freeze file \"%s\". Directory is read-only or does not exist.", filename);
            
            S9xMessage (S9X_ERROR, S9X_FREEZE_FILE_NOT_FOUND, String);
        }
    }
    return (FALSE);
}

void S9xCloseSnapshotFile( STREAM file)
{
    CLOSE_STREAM (file);
}

void S9xMessage (int, int, const char *str)
{
#ifdef DEBUGGER
    static FILE *out = NULL;

    if (out == NULL)
        out = fopen ("out.txt", "w");

    fprintf (out, "%s\n", str);
#endif

    S9xSetInfoString (str);
}

unsigned long _interval = 10;
long _buffernos = 4;
long _blocksize = 4400;
long _samplecount = 440;
long _maxsamplecount = 0;
long _buffersize = 0;
uint8 *SoundBuffer = NULL;
bool StartPlaying = false;
DWORD _lastblock = 0;
bool8 pending_setup = false;
long pending_rate = 0;
bool8 pending_16bit = false;
bool8 pending_stereo = false;

bool8 SetupSound (long rate, bool8 sixteen_bit, bool8 stereo)
{
#ifdef FMOD_SUPPORT
    if (Settings.SoundDriver >= WIN_FMOD_DIRECT_SOUND_DRIVER)
    {
        so.mute_sound = TRUE;
        if (SoundBuffer)
        {
            SoundBuffer -= 32 * 1024;
            delete SoundBuffer;
            SoundBuffer = NULL;
        }
        
        _interval = 20;
        if (Settings.SoundBufferSize < 1)
            Settings.SoundBufferSize = 1;
        if (Settings.SoundBufferSize > 64)
            Settings.SoundBufferSize = 64;
        
        _buffernos = 4 * Settings.SoundBufferSize;
        int s = (rate * _interval *
                 (Settings.Stereo ? 2 : 1) * 
                 (Settings.SixteenBitSound ? 2 : 1)) / 1000;
        
        _blocksize = 64;
        while (_blocksize < s)
            _blocksize *= 2;
        
        _buffersize = _blocksize * _buffernos;
        _lastblock = 0;
        
        StartPlaying = false;
        
        so.playback_rate = Settings.SoundPlaybackRate;
        so.stereo = Settings.Stereo;
        so.sixteen_bit = Settings.SixteenBitSound;
        so.buffer_size = _blocksize;
        so.encoded = FALSE;
        
        if (DirectX.SetSoundMode ())
        {
            SoundBuffer = new uint8 [_blocksize * _buffernos + 1024 * 64];
            ZeroMemory (SoundBuffer, _blocksize * _buffernos + 1024 * 64);
            SoundBuffer += 32 * 1024;
            S9xSetPlaybackRate (so.playback_rate);
        }
        _samplecount = so.sixteen_bit ? _blocksize / 2 : _blocksize;
        _maxsamplecount = SOUND_BUFFER_SIZE;
        if (so.sixteen_bit)
            _maxsamplecount /= 2;
        if (so.stereo)
            _maxsamplecount /= 2;
        
        if (so.samples_mixed_so_far >= _maxsamplecount)
            so.samples_mixed_so_far = 0;
        so.mute_sound = FALSE;

        return (SoundBuffer != NULL);
    }
    else
    {
#endif
        pending_setup = TRUE;
        pending_rate = rate;
        pending_16bit = sixteen_bit;
        pending_stereo = stereo;
#ifdef FMOD_SUPPORT
    }
#endif
    return (TRUE);
}

bool8 RealSetupSound (long rate, bool8 sixteen_bit, bool8 stereo)
{
    so.mute_sound = TRUE;
    if (SoundBuffer)
    {
        SoundBuffer -= 32 * 1024;
        delete SoundBuffer;
        SoundBuffer = NULL;
    }

#if 0
    OSVERSIONINFO info;
    info.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    if (GetVersionEx (&info) && info.dwPlatformId == VER_PLATFORM_WIN32_NT &&
        info.dwMajorVersion == 4)
    {
        _interval = 20;
    }
    else
    {
        _interval = 10;
    }
#endif

    _interval = Settings.SoundMixInterval;

    if (Settings.SoundBufferSize < 1)
        Settings.SoundBufferSize = 1;
    if (Settings.SoundBufferSize > 64)
        Settings.SoundBufferSize = 64;

    _buffernos = 4 * Settings.SoundBufferSize;
    int s = (rate * _interval * 
             (Settings.Stereo ? 2 : 1) * 
             (Settings.SixteenBitSound ? 2 : 1)) / 1000;

    _blocksize = 64;
    while (_blocksize < s)
        _blocksize *= 2;

    _buffersize = _blocksize * _buffernos;
    _lastblock = 0;

    StartPlaying = false;

    so.playback_rate = Settings.SoundPlaybackRate;
    so.stereo = Settings.Stereo;
    so.sixteen_bit = Settings.SixteenBitSound;
    so.buffer_size = _blocksize;
    so.encoded = FALSE;

    if (!DirectX.DSAvailable)
        return (false);

    if (DirectX.SetSoundMode ())
    {
        SoundBuffer = new uint8 [_blocksize * _buffernos + 1024 * 64];
        ZeroMemory (SoundBuffer, _blocksize * _buffernos + 1024 * 64);
        SoundBuffer += 32 * 1024;
        S9xSetPlaybackRate (so.playback_rate);
    }

    _samplecount = so.sixteen_bit ? _blocksize / 2 : _blocksize;
    _maxsamplecount = SOUND_BUFFER_SIZE;
    if (so.sixteen_bit)
        _maxsamplecount /= 2;
    if (so.stereo)
        _maxsamplecount /= 2;

    if (so.samples_mixed_so_far >= _maxsamplecount)
        so.samples_mixed_so_far = 0;
    so.mute_sound = FALSE;

    return (SoundBuffer != NULL);
}

#define FIXED_POINT 0x10000
#define FIXED_POINT_SHIFT 16
#define FIXED_POINT_REMAINDER 0xffff

static bool8 block_signal = FALSE;
static volatile bool8 pending_signal = FALSE;

void ProcessSound ();

extern "C" void S9xGenerateSound(void)
{
    if (!SoundBuffer || so.samples_mixed_so_far >= _samplecount ||
        so.mute_sound
#ifdef FMOD_SUPPORT
        || Settings.SoundDriver >= WIN_FMOD_DIRECT_SOUND_DRIVER
#endif
		)
		return;
	
    block_signal = TRUE;
    so.err_counter += so.err_rate;
    if (so.err_counter >= FIXED_POINT)
    {
		int sample_count = so.err_counter >> FIXED_POINT_SHIFT;
		int byte_offset;
		int byte_count;
		
        so.err_counter &= FIXED_POINT_REMAINDER;
		if (so.stereo)
			sample_count <<= 1;
        if (so.samples_mixed_so_far + sample_count > _samplecount)
            sample_count = _samplecount - so.samples_mixed_so_far;
		
		byte_count = sample_count;
		
		if (so.sixteen_bit)
		{
			byte_offset = so.samples_mixed_so_far << 1;
			byte_count <<= 1;
		}
		else byte_offset = so.samples_mixed_so_far;
		
		while (sample_count > _maxsamplecount)
		{
			S9xMixSamplesO (SoundBuffer, _maxsamplecount, byte_offset);
			so.samples_mixed_so_far += _maxsamplecount;
			sample_count -= _maxsamplecount;
			byte_offset += SOUND_BUFFER_SIZE;
		}
		S9xMixSamplesO (SoundBuffer, sample_count, byte_offset);
		so.samples_mixed_so_far += sample_count;
    }
    block_signal = FALSE;
    if (pending_signal)
    {
		ProcessSound ();
		pending_signal = FALSE;
    }
}

// Interval ms has passed
void ProcessSound (void)
{
#ifdef DEBUG_MK_APU
	static FILE* fp;
#endif
    if (block_signal)
    {
        pending_signal = TRUE;
        return;
    }

    if (pending_setup)
    {
        pending_setup = false;
        RealSetupSound (pending_rate, pending_16bit, pending_stereo);
    }

    if (!DirectX.lpDSB || !SoundBuffer)
        return;

    bool8 mute = FALSE;

#if 0
    if (DirectX.IdleCount >= GUI.PausedFramesBeforeMutingSound)
    {
        if (!StartPlaying)
        {
            DirectX.lpDSB->Stop ();
            StartPlaying = TRUE;
        }
        return;
    }
#else
    mute = DirectX.IdleCount >= GUI.PausedFramesBeforeMutingSound;
#endif

    if (StartPlaying)
    {
        if (DirectX.lpDSB->Play (0, 0, DSBPLAY_LOOPING) != DS_OK)
            return;

        _lastblock = 0;
        StartPlaying = false;
    }
    DWORD play_pos = 0, write_pos = 0;
    HRESULT hResult;

    DirectX.lpDSB->GetCurrentPosition (&play_pos, &write_pos);

    DWORD curr_block = ((play_pos / _blocksize) + 1 * Settings.SoundBufferSize) % _buffernos;

//printf ("play_pos = %d, write_pos = %d, curr_block = %d, lastblock = %d\n",
//        play_pos, write_pos, curr_block, _lastblock);
//fflush (stdout);
    if (curr_block != _lastblock)
    {
	BYTE  *B1, *B2;
	DWORD S1, S2;

        write_pos = curr_block * _blocksize;
        _lastblock = curr_block;

        hResult = DirectX.lpDSB->Lock (write_pos, _blocksize, (void **)&B1, 
                                       &S1, (void **)&B2, &S2, 0);
        if (hResult == DSERR_BUFFERLOST)
        {
            DirectX.lpDSB->Restore ();
            hResult = DirectX.lpDSB->Lock (write_pos, _blocksize, 
                                           (void **)&B1, &S1, (void **)&B2, 
                                           &S2, 0);
        }

        if( hResult != DS_OK)
            return;

        if( mute || Settings.ForcedPause || 
            Settings.Paused || Settings.StopEmulation || GUI.AVIOut)
        {
            if (so.sixteen_bit)
            {
                if (B1)
                    ZeroMemory (B1, S1);
                if (B2)
                    ZeroMemory (B2, S2);
            }
            else
            {
                if (B1)
                    memset (B1, 128, S1);
                if (B2)
                    memset (B2, 128, S2);
            }
        }
        else
        {
            unsigned int sample_count = so.buffer_size;
            int byte_offset;

            if (so.sixteen_bit)
                sample_count >>= 1;

            if (so.samples_mixed_so_far < (int32) sample_count)
            {
                byte_offset = (so.sixteen_bit ? (so.samples_mixed_so_far << 1)
                                              : so.samples_mixed_so_far);

                if (Settings.SoundSync == 2)
                {
                    /*memset (SoundBuffer + (byte_offset & SOUND_BUFFER_SIZE_MASK), 0,
                      sample_count - so.samples_mixed_so_far);*/
                }
                else
                {
                    sample_count -= so.samples_mixed_so_far;
                    while ((long) sample_count > _maxsamplecount)
                    {
                        S9xMixSamplesO (SoundBuffer, _maxsamplecount,
                                        byte_offset);
                        sample_count -= _maxsamplecount;
                        byte_offset += SOUND_BUFFER_SIZE;
                    }
                    S9xMixSamplesO (SoundBuffer, sample_count, byte_offset);
                }
                so.samples_mixed_so_far = 0;
            }
            else so.samples_mixed_so_far -= sample_count;

            if (Settings.Mute)
            {
                if (so.sixteen_bit)
                {
                    if (B1)
                        ZeroMemory (B1, S1);
                    if (B2)
                        ZeroMemory (B2, S2);
                }
                else
                {
                    if (B1)
                        memset (B1, 128, S1);
                    if (B2)
                        memset (B2, 128, S2);
                }
            }
            else
            {
                if (B1)
				{
                    memmove (B1, SoundBuffer, S1);
#ifdef DEBUG_MK_APU
					if(fp==NULL)
					{
						fp=fopen("FF2DS6.raw", "ab");
					}
					fwrite(SoundBuffer,1, S1, fp);
#endif
				}
                if (B2)
				{
                    memmove (B2, SoundBuffer + S1, S2);
#ifdef DEBUG_MK_APU
					if(fp==NULL)
					{
						fp=fopen("FF2DS6.raw", "ab");
					}
					fwrite(SoundBuffer+S1,1, S2, fp);
#endif
				}
            }
        }
        hResult = DirectX.lpDSB -> Unlock (B1, S1, B2, S2);
        if( hResult != DS_OK)
            return;
    }
    DWORD Status;
    hResult = DirectX.lpDSB->GetStatus (&Status);
    StartPlaying = !Status;
#ifdef MK_APU
	if(SoundData.sample_cycles<0)
		SoundData.sample_cycles=0;
#endif
}

bool8 S9xOpenSoundDevice (int mode, bool8 pStereo, int BufferSize)
{
    return (TRUE);
}

int superscope_pause = 0;
int superscope_turbo = 0;
static RECT dstRect = { 0, 512, 0, 448 };

bool8 S9xReadSuperScopePosition (int &x, int &y, uint32 &buttons)
{
    x = (GUI.MouseX);
    //not all games are 224
	y = (GUI.MouseY) ;
    buttons = GUI.MouseButtons | (superscope_turbo << 2) | 
              (superscope_pause << 3);
    return (TRUE);
}

extern unsigned long START;

void S9xSyncSpeed( void)
{
#ifdef NETPLAY_SUPPORT
    if (Settings.NetPlay)
    {
#if defined (NP_DEBUG) && NP_DEBUG == 2
        printf ("CLIENT: SyncSpeed @%d\n", timeGetTime () - START);
#endif
        S9xWinScanJoypads ();
	// Send joypad position update to server
	S9xNPSendJoypadUpdate (joypads [0]);
        LONG prev;
        BOOL success;

	// Wait for heart beat from server
        if ((success = ReleaseSemaphore (GUI.ClientSemaphore, 1, &prev)) &&
            prev == 0)
        {
            // No heartbeats already arrived, have to wait for one.
            // Mop up the ReleaseSemaphore test above...
            WaitForSingleObject (GUI.ClientSemaphore, 0);

            // ... and then wait for the real sync-signal from the
            // client loop thread.
            NetPlay.PendingWait4Sync = WaitForSingleObject (GUI.ClientSemaphore, 100) != WAIT_OBJECT_0;
#if defined (NP_DEBUG) && NP_DEBUG == 2
            if (NetPlay.PendingWait4Sync)
                printf ("CLIENT: PendingWait4Sync1 @%d\n", timeGetTime () - START);
#endif
            IPPU.RenderThisFrame = TRUE;
            IPPU.SkippedFrames = 0;
        }
        else
        {
            if (success)
            {
                // Once for the ReleaseSemaphore above...
                WaitForSingleObject (GUI.ClientSemaphore, 0);
                if (prev == 4 && NetPlay.Waiting4EmulationThread)
                {
                    // Reached the lower behind count threshold - tell the
                    // server its safe to start sending sync pulses again.
                    NetPlay.Waiting4EmulationThread = FALSE;
                    S9xNPSendPause (FALSE);
                }

#if defined (NP_DEBUG) && NP_DEBUG == 2
                if (prev > 1)
                {
                    printf ("CLIENT: SyncSpeed prev: %d @%d\n", prev, timeGetTime () - START);
                }
#endif
            }
            else
            {
#ifdef NP_DEBUG
                printf ("*** CLIENT: SyncSpeed: Release failed @ %d\n", timeGetTime () - START);
#endif
            }

            // ... and again to mop up the already-waiting sync-signal
            NetPlay.PendingWait4Sync = WaitForSingleObject (GUI.ClientSemaphore, 200) != WAIT_OBJECT_0;
#if defined (NP_DEBUG) && NP_DEBUG == 2
            if (NetPlay.PendingWait4Sync)
                printf ("CLIENT: PendingWait4Sync2 @%d\n", timeGetTime () - START);
#endif

	    if (IPPU.SkippedFrames < NetPlay.MaxFrameSkip)
	    {
		IPPU.SkippedFrames++;
		IPPU.RenderThisFrame = FALSE;
	    }
	    else
	    {
		IPPU.RenderThisFrame = TRUE;
		IPPU.SkippedFrames = 0;
	    }
        }
        // Give up remainder of time-slice to any other waiting threads,
        // if they need any time, that is.
        Sleep (0);
        if (!NetPlay.PendingWait4Sync)
        {
            NetPlay.FrameCount++;
            S9xNPStepJoypadHistory ();
        }
    }
    else
#endif

    if (!Settings.TurboMode && Settings.SkipFrames == AUTO_FRAMERATE &&
		!GUI.AVIOut)
    {
        if (!do_frame_adjust)
        {
            IPPU.RenderThisFrame = TRUE;
            IPPU.SkippedFrames = 0;
        }
        else
        {
	    if (IPPU.SkippedFrames < Settings.AutoMaxSkipFrames)
	    {
		IPPU.SkippedFrames++;
		IPPU.RenderThisFrame = FALSE;
	    }
	    else
	    {
		IPPU.RenderThisFrame = TRUE;
		IPPU.SkippedFrames = 0;
	    }
        }
    }
    else
    {
	uint32 SkipFrames;
	if(Settings.TurboMode)
		SkipFrames = Settings.TurboSkipFrames;
	else
		SkipFrames = (Settings.SkipFrames == AUTO_FRAMERATE) ? 0 : Settings.SkipFrames;
	if (++IPPU.FrameSkip >= SkipFrames)
	{
	    IPPU.FrameSkip = 0;
	    IPPU.SkippedFrames = 0;
	    IPPU.RenderThisFrame = TRUE;
	}
	else
	{
	    IPPU.SkippedFrames++;
	    IPPU.RenderThisFrame = FALSE;
	}
    }
}

const char *S9xBasename (const char *f)
{
    const char *p;
    if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL)
	return (p + 1);

#ifdef __DJGPP
    if (p = strrchr (f, SLASH_CHAR))
	return (p + 1);
#endif

    return (f);
}

bool8 S9xReadMousePosition (int which, int &x, int &y, uint32 &buttons)
{
    if (which == 0)
    {
        x = GUI.MouseX;
        y = GUI.MouseY;
        buttons = GUI.MouseButtons;
        return (TRUE);
    }

    return (FALSE);
}

bool S9xGetState (WORD KeyIdent)
{
    if (KeyIdent & 0x8000)
    {
        int j = (KeyIdent >> 8) & 15;

        switch (KeyIdent & 0xff)
        {
            case 0: return !Joystick [j].Left;
            case 1: return !Joystick [j].Right;
            case 2: return !Joystick [j].Up;
            case 3: return !Joystick [j].Down;
            case 4: return !Joystick [j].PovLeft;
            case 5: return !Joystick [j].PovRight;
            case 6: return !Joystick [j].PovUp;
            case 7: return !Joystick [j].PovDown;
			case 49: return !Joystick [j].PovDnLeft;
			case 50:return !Joystick [j].PovDnRight;
			case 51:return !Joystick [j].PovUpLeft;
			case 52:return !Joystick [j].PovUpRight;
            case 41:return !Joystick [j].ZUp;
            case 42:return !Joystick [j].ZDown;
            case 43:return !Joystick [j].RUp;
            case 44:return !Joystick [j].RDown;
            case 45:return !Joystick [j].UUp;
            case 46:return !Joystick [j].UDown;
            case 47:return !Joystick [j].VUp;
            case 48:return !Joystick [j].VDown;
            
            default:
                if ((KeyIdent & 0xff) > 40)
                    return false;
                
                return !Joystick [j].Button [(KeyIdent & 0xff) - 8];
        }
    } 

    return ((GetKeyState (KeyIdent) & 0x80) == 0);
}

uint32 S9xReadJoypad (int which1)
{
    if (which1 > 4)
        return 0;

    if (which1 == 0 && !Settings.NetPlay)
        S9xWinScanJoypads ();

#ifdef NETPLAY_SUPPORT
    if (Settings.NetPlay)
	return (S9xNPGetJoypad (which1));
#endif

    return (joypads [which1]);
}

void CheckAxis (int val, int min, int max, bool &first, bool &second)
{
    if (Normalize (val, min, max) < -S9X_JOY_NEUTRAL)
    {
        second = false;
        first = true;
    }
    else
        first = false;
    
    if (Normalize (val, min, max) > S9X_JOY_NEUTRAL)
    {
        first = false;
        second = true;
    }
    else
        second = false;
}

static void S9xWinScanJoypads ()
{
    uint8 PadState[2];
    JOYINFOEX jie;
    
    for (int C = 0; C != 16; C ++)
    {
        if (Joystick[C].Attached)
        {
            jie.dwSize = sizeof (jie);
            jie.dwFlags = JOY_RETURNALL;
            
            if (joyGetPosEx (JOYSTICKID1+C, &jie) != JOYERR_NOERROR)
            {
                Joystick[C].Attached = false;
                continue;
            }
            
            CheckAxis (jie.dwXpos, 
                       Joystick[C].Caps.wXmin, Joystick[C].Caps.wXmax,
                       Joystick[C].Left, Joystick[C].Right);
            CheckAxis (jie.dwYpos,
                       Joystick[C].Caps.wYmin, Joystick[C].Caps.wYmax,
                       Joystick[C].Up, Joystick[C].Down);
            CheckAxis (jie.dwZpos,
                       Joystick[C].Caps.wZmin, Joystick[C].Caps.wZmax,
                       Joystick[C].ZUp, Joystick[C].ZDown);
            CheckAxis (jie.dwRpos,
                       Joystick[C].Caps.wRmin, Joystick[C].Caps.wRmax,
                       Joystick[C].RUp, Joystick[C].RDown);
            CheckAxis (jie.dwUpos,
                       Joystick[C].Caps.wUmin, Joystick[C].Caps.wUmax,
                       Joystick[C].UUp, Joystick[C].UDown);
            CheckAxis (jie.dwVpos,
                       Joystick[C].Caps.wVmin, Joystick[C].Caps.wVmax,
                       Joystick[C].VUp, Joystick[C].VDown);

            switch (jie.dwPOV)
            {
                case JOY_POVBACKWARD:
                    Joystick[C].PovDown = true;
                    Joystick[C].PovUp = false;
                    Joystick[C].PovLeft = false;
                    Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;	
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
					 break;
				case 4500:
					Joystick[C].PovDown = false;
					Joystick[C].PovUp = false;
					Joystick[C].PovLeft = false;
					Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = true;
					break;
				case 13500:
					Joystick[C].PovDown = false;
					Joystick[C].PovUp = false;
					Joystick[C].PovLeft = false;
					Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = true;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
					break;
				case 22500:
					Joystick[C].PovDown = false;
					Joystick[C].PovUp = false;
					Joystick[C].PovLeft = false;
					Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = true;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
					break;
				case 31500:
					Joystick[C].PovDown = false;
					Joystick[C].PovUp = false;
					Joystick[C].PovLeft = false;
					Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;
					Joystick[C].PovUpLeft = true;
					Joystick[C].PovUpRight = false;
					break;

                    
                case JOY_POVFORWARD:
                    Joystick[C].PovDown = false;
                    Joystick[C].PovUp = true;
                    Joystick[C].PovLeft = false;
                    Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;	
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
                    break;
                    
                case JOY_POVLEFT:
                    Joystick[C].PovDown = false;
                    Joystick[C].PovUp = false;
                    Joystick[C].PovLeft = true;
                    Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;	
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
                    break;
                    
                case JOY_POVRIGHT:
                    Joystick[C].PovDown = false;
                    Joystick[C].PovUp = false;
                    Joystick[C].PovLeft = false;
                    Joystick[C].PovRight = true;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;	
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
                    break;
                    
                default:
                    Joystick[C].PovDown = false;
                    Joystick[C].PovUp = false;
                    Joystick[C].PovLeft = false;
                    Joystick[C].PovRight = false;
					Joystick[C].PovDnLeft = false;
					Joystick[C].PovDnRight = false;	
					Joystick[C].PovUpLeft = false;
					Joystick[C].PovUpRight = false;
                    break;
            }
            
            for (int B = 0; B < 32; B ++)
                Joystick[C].Button[B] = (jie.dwButtons & (1 << B)) != 0;
        }
    }

    for (int J = 0; J < 5; J++)
    {
        if (Joypad [J].Enabled)
        {
            PadState[0]  = 0;
            PadState[0] |= !S9xGetState (Joypad[J].R)      ?  16 : 0;
            PadState[0] |= !S9xGetState (Joypad[J].L)      ?  32 : 0;
            PadState[0] |= !S9xGetState (Joypad[J].X)      ?  64 : 0;
            PadState[0] |= !S9xGetState (Joypad[J].A)      ? 128 : 0;
    
            PadState[1]  = 0;
            PadState[1] |= !S9xGetState (Joypad[J].Right)  ?   1 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].Right_Up)  ? 1 + 8 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].Right_Down)? 1 + 4 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].Left)   ?   2 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].Left_Up)?   2 + 8 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].Left_Down)?  2 + 4 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].Down)   ?   4 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].Up)     ?   8 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].Start)  ?  16 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].Select) ?  32 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].Y)      ?  64 : 0;
            PadState[1] |= !S9xGetState (Joypad[J].B)      ? 128 : 0;

			//handle turbo case!
			if((GUI.TurboMask&TURBO_A_MASK)&&(PadState[0]&128))
				PadState[0]^=(joypads[J]&128);
			if((GUI.TurboMask&TURBO_B_MASK)&&(PadState[1]&128))
				PadState[1]^=((joypads[J]&(128<<8))>>8);
			if((GUI.TurboMask&TURBO_Y_MASK)&&(PadState[1]&64))
				PadState[1]^=((joypads[J]&(64<<8))>>8);
			if((GUI.TurboMask&TURBO_X_MASK)&&(PadState[0]&64))
				PadState[0]^=(joypads[J]&64);
			if((GUI.TurboMask&TURBO_L_MASK)&&(PadState[0]&32))
				PadState[0]^=(joypads[J]&32);
			if((GUI.TurboMask&TURBO_R_MASK)&&(PadState[0]&16))
				PadState[0]^=(joypads[J]&16);
			if((GUI.TurboMask&TURBO_SEL_MASK)&&(PadState[1]&32))
				PadState[1]^=((joypads[J]&(32<<8))>>8);
			if((GUI.TurboMask&TURBO_STA_MASK)&&(PadState[1]&16))
				PadState[1]^=((joypads[J]&(16<<8))>>8);
			//end turbo case...

            joypads [J] = PadState [0] | (PadState [1] << 8) | 0x80000000;
        }
        else
            joypads [J] = 0;
    }
}

PALETTEENTRY S9xPaletteEntry[256];
void S9xSetPalette( void)
{
    LPDIRECTDRAWPALETTE lpDDTemp;
    uint16 Brightness = IPPU.MaxBrightness * 140;

    // Only update the palette structures if needed
    if( GUI.ScreenDepth == 8)
    {
        if (Settings.SixteenBit)
        {
            for (int i = 0; i < 256; i++)
            {
                S9xPaletteEntry[i].peRed   = FixedColours [i].red;
                S9xPaletteEntry[i].peGreen = FixedColours [i].green;
                S9xPaletteEntry[i].peBlue  = FixedColours [i].blue;
            }
        }
        else
        {
            for (int i = 0; i < 256; i ++)
            {	
                S9xPaletteEntry[i].peRed   = (((PPU.CGDATA [i] >>  0) & 0x1F) * Brightness) >> 8;
                S9xPaletteEntry[i].peGreen = (((PPU.CGDATA [i] >>  5) & 0x1F) * Brightness) >> 8;
                S9xPaletteEntry[i].peBlue  = (((PPU.CGDATA [i] >> 10) & 0x1F) * Brightness) >> 8;
            }
        }

        DirectX.lpDDSPrimary2->GetPalette (&lpDDTemp);
        if (lpDDTemp != DirectX.lpDDPalette)
            DirectX.lpDDSPrimary2->SetPalette (DirectX.lpDDPalette);
        DirectX.lpDDPalette->SetEntries (0, 0, 256, S9xPaletteEntry);
    }
}

bool LockSurface2 (LPDIRECTDRAWSURFACE2 lpDDSurface, SSurface *lpSurface)
{
    DDSURFACEDESC ddsd;
    HRESULT hResult;
    int retry;

    ddsd.dwSize = sizeof (ddsd);

    retry = 0;
    while (true)
    {
        hResult = lpDDSurface->Lock( NULL, &ddsd, DDLOCK_WAIT, NULL);

        if( hResult == DD_OK)
        {
            lpSurface->Width = ddsd.dwWidth;
            lpSurface->Height = ddsd.dwHeight;
            lpSurface->Pitch = ddsd.lPitch;
            lpSurface->Surface = (unsigned char *)ddsd.lpSurface;
            return (true);
        }

        if (hResult == DDERR_SURFACELOST)
        {
            retry++;
            if (retry > 5)
                return (false);

            hResult = lpDDSurface->Restore();
            GUI.ScreenCleared = true;
            if (hResult != DD_OK)
                return (false);

            continue;
        }

        if (hResult != DDERR_WASSTILLDRAWING)
            return (false);
    }
}

BYTE *ScreenBuf1 = NULL;
BYTE *ScreenBuf2 = NULL;
BYTE *ScreenBuffer = NULL;
BYTE *SubScreenBuffer = NULL;
BYTE *ZBuffer = NULL;
BYTE *SubZBuffer = NULL;

static bool8 locked_surface = FALSE;
static SSurface Dst;

bool8 S9xInitUpdate (void)
{
    GFX.SubScreen = SubScreenBuffer;
    GFX.ZBuffer = ZBuffer;
    GFX.SubZBuffer = SubZBuffer;

    if (GUI.Scale <= 1 &&
        (GUI.Stretch || !GUI.FullScreen) &&
        !GUI.NeedDepthConvert &&
        ((DirectX.lpDDSOffScreen2 &&
          LockSurface2 (DirectX.lpDDSOffScreen2, &Dst))))
    {
        GFX.RealPitch = GFX.Pitch = GFX.Pitch2 = Dst.Pitch;
        GFX.Screen = Dst.Surface;
        DirectX.lpDDSOffScreen2->Unlock (Dst.Surface);
        locked_surface = TRUE;
    }
    else
    {
        locked_surface = FALSE;
#ifdef USE_OPENGL
        // For OpenGL lock the screen buffer size width to 512 (might be
        // overriden in S9xStartScreenRefresh) so the buffer can be uploaded
        // into texture memory with a single OpenGL call.
        if (Settings.OpenGLEnable)
            GFX.RealPitch = GFX.Pitch = GFX.Pitch2 = 512 * sizeof (uint16);
        else
#endif
        GFX.RealPitch = GFX.Pitch = GFX.Pitch2 = EXT_PITCH;
        GFX.Screen = ScreenBuffer;
    }
    GFX.Delta = (GFX.SubScreen - GFX.Screen) >> 1;
    if (Settings.SixteenBit)
    {
        GFX.PPL = GFX.Pitch >> 1;
        GFX.PPLx2 = GFX.Pitch;
        GFX.ZPitch = GFX.Pitch >> 1;
    }
    else
    {
        GFX.PPL = GFX.Pitch;
        GFX.PPLx2 = GFX.Pitch * 2;
        GFX.ZPitch = GFX.Pitch;
    }
    return (TRUE);
}

bool8 S9xDeinitUpdate (int Width, int Height, bool8 sixteen_bit)
{
	#define BLACK BUILD_PIXEL(0,0,0)
    SSurface Src;
    LPDIRECTDRAWSURFACE2 lpDDSurface2 = NULL;
    LPDIRECTDRAWSURFACE2 pDDSurface = NULL;
    bool PrimarySurfaceLockFailed = false;
    RECT srcRect;

#ifdef USE_DIRECTX3D
    HRESULT Render3DEnvironment ();

    Render3DEnvironment ();
    return true;
#endif

    Src.Width = Width;
	if(Height%SNES_HEIGHT)
	    Src.Height = Height;
	else
	{
		if(Height==SNES_HEIGHT)
			Src.Height=SNES_HEIGHT_EXTENDED;
		else Src.Height=SNES_HEIGHT_EXTENDED<<1;
	}
    Src.Pitch = GFX.Pitch;
    Src.Surface = GFX.Screen;

	// avi writing
	DoAVIVideoFrame(&Src, Width, Height, sixteen_bit);

	if(Settings.SixteenBit)
	{
		uint32 q;
		uint16* foo=(uint16*)(Src.Surface+(Height*Src.Pitch));
		for(q=0; q<(((Src.Height-Height)*Src.Pitch)>>1); q++)
		{
			foo[q]=BLACK;
		}
		Height = Src.Height;
	}
	else
	{
		uint32 q;
		uint8* foo=(uint8*)(Src.Surface+(Height*Src.Pitch));
		for(q=0; q<(((Src.Height-Height)*Src.Pitch)); q++)
		{
			foo[q]=BLACK;
		}
		Height = Src.Height;
	}

	GUI.ScreenCleared = true;

    SelectRenderMethod ();

    if (!VOODOO_MODE && !OPENGL_MODE)
    {
        if (locked_surface)
        {
            lpDDSurface2 = DirectX.lpDDSOffScreen2;
            PrimarySurfaceLockFailed = true;
            srcRect.top    = 0;
            srcRect.bottom = Height;
            srcRect.left   = 0;
            srcRect.right  = Width;
        }
        else
        {
            DDSCAPS caps;
            caps.dwCaps = DDSCAPS_BACKBUFFER;
            
            if (DirectX.lpDDSPrimary2->GetAttachedSurface (&caps, &pDDSurface) != DD_OK ||
                pDDSurface == NULL)
            {
                lpDDSurface2 = DirectX.lpDDSPrimary2;
            }
            else 
                lpDDSurface2 = pDDSurface;
            
            if (GUI.Stretch || GUI.Scale == 1 || !GUI.FullScreen ||
                !LockSurface2 (lpDDSurface2, &Dst))
            {
                lpDDSurface2 = DirectX.lpDDSOffScreen2;
                if (!LockSurface2 (lpDDSurface2, &Dst))
                    return (false);
                
                PrimarySurfaceLockFailed = true;
            }
            
            if (Settings.SixteenBit && GUI.Scale >= 3 && GUI.Scale <= 6)
            {
                // Clear the old areas of the SNES rendered image otherwise the
                // image processors might try to access some of the old rendered
                // data.
                static int LastWidth;
                static int LastHeight;
                
                if (Width < LastWidth)
                {
                    for (int i = 0; i < Height; i++)
                        memset (GFX.Screen + Width * 2 + i * GFX.Pitch, 0, 4);
                }
                if (Height < LastHeight)
                {
                    int ww = LastWidth > Width ? LastWidth : Width;
                    for (int i = Height; i < Height + 3; i++)
                        memset (GFX.Screen + i * GFX.Pitch, 0, ww * 2);
                }
                LastWidth = Width;
                LastHeight = Height;
            }
            if (GUI.NeedDepthConvert)
            {
                if (GUI.Scale <= 1)
                {
                    srcRect.left = srcRect.top = 0;
                    srcRect.right = Width;
                    srcRect.bottom = Height;
                    ConvertDepth (&Src, &Dst, &srcRect);
                }
                else
                {
                    SSurface tmp;
                    BYTE buf [MAX_SNES_WIDTH * MAX_SNES_HEIGHT * 4];
                    
                    tmp.Surface = buf;
                    tmp.Pitch = MAX_SNES_WIDTH * 4;
                    tmp.Width = MAX_SNES_WIDTH;
                    tmp.Height = MAX_SNES_HEIGHT;
                    RenderMethod (Src, tmp, &srcRect);
                    ConvertDepth (&tmp, &Dst, &srcRect);
                }
            }
            else RenderMethod (Src, Dst, &srcRect);
        }
        
        RECT lastRect = GUI.SizeHistory [GUI.FlipCounter % GUI.NumFlipFrames];
        if (PrimarySurfaceLockFailed)
        {
            POINT p;
            
            if (GUI.Stretch)
            {	
			/*	p.x = p.y = 0;
				
                ClientToScreen (GUI.hWnd, &p);
				GetClientRect (GUI.hWnd, &dstRect);
				OffsetRect(&dstRect, p.x, p.y);
				*/
				p.x = p.y = 0;
				ClientToScreen (GUI.hWnd, &p);
				GetClientRect (GUI.hWnd, &dstRect);
				dstRect.bottom = int(double(dstRect.bottom) * double(239.0 / 240.0));
				OffsetRect(&dstRect, p.x, p.y);
            }
            else
            {
                GetClientRect (GUI.hWnd, &dstRect);
                int width = srcRect.right - srcRect.left;
                int height = srcRect.bottom - srcRect.top;
                
                if (GUI.Scale == 1)
                {
                    width = MAX_SNES_WIDTH;
                    if (height < 240)
                        height *= 2;
                }
                p.x = ((dstRect.right - dstRect.left) - width) >> 1;
 			p.y = ((dstRect.bottom - dstRect.top) - height) >> 1;
                ClientToScreen (GUI.hWnd, &p);
                
                dstRect.top = p.y;
                dstRect.left = p.x;
                dstRect.bottom = dstRect.top + height;
                dstRect.right  = dstRect.left + width;
            }
        }
        else
            dstRect = srcRect;
	
        lpDDSurface2->Unlock (Dst.Surface);	
        if (PrimarySurfaceLockFailed)
        {
            DDSCAPS caps;
            caps.dwCaps = DDSCAPS_BACKBUFFER;
            
            if (DirectX.lpDDSPrimary2->GetAttachedSurface (&caps, &pDDSurface) != DD_OK ||
                pDDSurface == NULL)
            {
                lpDDSurface2 = DirectX.lpDDSPrimary2;
            }
            else 
                lpDDSurface2 = pDDSurface;
            
            while (lpDDSurface2->Blt (&dstRect, DirectX.lpDDSOffScreen2, &srcRect, DDBLT_WAIT, NULL) == DDERR_SURFACELOST)
                lpDDSurface2->Restore ();
        }
        
        RECT rect;
        DDBLTFX fx;
        
        memset (&fx, 0, sizeof (fx));
        fx.dwSize = sizeof (fx);
        
        if (GUI.FlipCounter >= GUI.NumFlipFrames)
        {
            if (lastRect.top < dstRect.top)
            {
                rect.top = lastRect.top;
                rect.bottom = dstRect.top;
                rect.left = min(lastRect.left, dstRect.left);
                rect.right = max(lastRect.right, dstRect.right);
                lpDDSurface2->Blt (&rect, NULL, &rect,
                                   DDBLT_WAIT | DDBLT_COLORFILL, &fx);
            }
            if (lastRect.bottom > dstRect.bottom)
            {
                rect.left = min(lastRect.left, dstRect.left);
                rect.right = max(lastRect.right, dstRect.right);
                rect.top = dstRect.bottom;
                rect.bottom = lastRect.bottom;
                lpDDSurface2->Blt (&rect, NULL, &rect,
                                   DDBLT_WAIT | DDBLT_COLORFILL, &fx);
            }
            if (lastRect.left < dstRect.left)
            {
                rect.left = lastRect.left;
                rect.right = dstRect.left;
                rect.top = dstRect.top;
                rect.bottom = dstRect.bottom;
                lpDDSurface2->Blt (&rect, NULL, &rect,
                                   DDBLT_WAIT | DDBLT_COLORFILL, &fx);
            }
            if (lastRect.right > dstRect.right)
            {
                rect.left = dstRect.right;
                rect.right = lastRect.right;
                rect.top = dstRect.top;
                rect.bottom = dstRect.bottom;
                lpDDSurface2->Blt (&rect, NULL, &rect,
                                   DDBLT_WAIT | DDBLT_COLORFILL, &fx);
            }
        }
        
        DirectX.lpDDSPrimary2->Flip (NULL, DDFLIP_WAIT);
    }
    else
    {
        srcRect.top    = 0;
        srcRect.bottom = Height;
        srcRect.left   = 0;
        srcRect.right  = Width;
        dstRect = srcRect;
	RenderMethod (Src, Dst, &srcRect);
    }
    
    GUI.SizeHistory [GUI.FlipCounter % GUI.NumFlipFrames] = dstRect;
    GUI.FlipCounter++;

    return (true);
}

void InitSnes9X( void)
{
#ifdef DEBUGGER
//    extern FILE *trace;

//    trace = fopen( "SNES9X.TRC", "wt");
    freopen( "SNES9X.OUT", "wt", stdout);
    freopen( "SNES9X.ERR", "wt", stderr);

//    CPU.Flags |= TRACE_FLAG;
//    APU.Flags |= TRACE_FLAG;
#endif

#ifdef GENERATE_OFFSETS_H
    offsets_h = fopen ("offsets.h", "wt");
    generate_offsets_h (0, NULL);
    fclose (offsets_h);
#endif

    Memory.Init();

    ScreenBuf1 = new BYTE [EXT_PITCH * EXT_HEIGHT];
    ScreenBuf2 = new BYTE [EXT_PITCH * EXT_HEIGHT];
    SubScreenBuffer = new BYTE [EXT_PITCH * EXT_HEIGHT];
    ZBuffer = new BYTE [EXT_WIDTH * EXT_HEIGHT];
    SubZBuffer = new BYTE [EXT_WIDTH * EXT_HEIGHT];

    ScreenBuffer = ScreenBuf1 + EXT_OFFSET;
    memset (ScreenBuf1, 0, EXT_PITCH * EXT_HEIGHT);
    memset (ScreenBuf2, 0, EXT_PITCH * EXT_HEIGHT);
    memset (SubScreenBuffer, 0, EXT_PITCH * EXT_HEIGHT);
    memset (ZBuffer, 0, EXT_WIDTH * EXT_HEIGHT);
    memset (SubZBuffer, 0, EXT_WIDTH * EXT_HEIGHT);

    GFX.Pitch = EXT_PITCH;
    GFX.RealPitch = EXT_PITCH;
    GFX.Screen = ScreenBuf1 + EXT_OFFSET;
    GFX.SubScreen = SubScreenBuffer;
    GFX.Delta = (GFX.SubScreen - GFX.Screen) >> 1;

    S9xSetWinPixelFormat ();
    S9xGraphicsInit();

    S9xInitAPU();
    S9xInitSound (7, true, _blocksize);

    so.playback_rate = Settings.SoundPlaybackRate;
    so.stereo = Settings.Stereo;
    so.sixteen_bit = Settings.SixteenBitSound;
    so.buffer_size = _blocksize;

    // Sound options
    Settings.SoundBufferSize = Settings.SoundBufferSize;

#ifdef USE_GLIDE
    Settings.GlideEnable = FALSE;
#endif

	S9xMovieInit ();

    for (int C = 0; C != 16; C ++)
        Joystick[C].Attached = joyGetDevCaps (JOYSTICKID1+C, &Joystick[C].Caps,
                                              sizeof( JOYCAPS)) == JOYERR_NOERROR;
}
extern "C"{
void DeinitS9x()
{
	if(ScreenBuf1)
		delete [] ScreenBuf1;
	if(ScreenBuf2)
		delete [] ScreenBuf2;
	if(SubScreenBuffer)
		delete [] SubScreenBuffer;
	if(ZBuffer)
		delete [] ZBuffer;
	if(SubZBuffer)
		delete [] SubZBuffer;
	if (SoundBuffer)
	{
		SoundBuffer -= 32 * 1024;
		delete [] SoundBuffer;
		SoundBuffer = NULL;
	}
	if(GUI.GunSight)
		DestroyCursor(GUI.GunSight);//= LoadCursor (hInstance, MAKEINTRESOURCE (IDC_CURSOR_SCOPE));
    if(GUI.Arrow)
		DestroyCursor(GUI.Arrow);// = LoadCursor (NULL, IDC_ARROW);
	if(GUI.Accelerators)
		DestroyAcceleratorTable(GUI.Accelerators);// = LoadAccelerators (hInstance, MAKEINTRESOURCE (IDR_SNES9X_ACCELERATORS));

	S9xMovieStop (TRUE);
}
}
int ffs (uint32 mask)
{
    int m = 0;
    if (mask)
    {
        while (!(mask & (1 << m)))
            m++;

        return (m);
    }

    return (0);
}

void S9xSetWinPixelFormat ()
{
    extern int Init_2xSaI (uint32 BitFormat);

    S9xSetRenderPixelFormat (RGB565);
    Init_2xSaI (565);
    GUI.NeedDepthConvert = FALSE;

    if (VOODOO_MODE)
    {
        GUI.ScreenDepth = 16;
        GUI.RedShift = 11;
        GUI.GreenShift = 5;
        GUI.BlueShift = 0;
        Settings.SixteenBit = TRUE;
    }
    else
    if (OPENGL_MODE)
    {
        GUI.ScreenDepth = 16;
        GUI.RedShift = 11;
        GUI.GreenShift = 6;
        GUI.BlueShift = 1;
        Settings.SixteenBit = TRUE;
	S9xSetRenderPixelFormat (RGB5551);
	Init_2xSaI (555);
    }
    else
    {
        GUI.ScreenDepth = DirectX.DDPixelFormat.dwRGBBitCount;
        if (GUI.ScreenDepth == 15)
            GUI.ScreenDepth = 16;

        GUI.RedShift = ffs (DirectX.DDPixelFormat.dwRBitMask);
        GUI.GreenShift = ffs (DirectX.DDPixelFormat.dwGBitMask);
        GUI.BlueShift = ffs (DirectX.DDPixelFormat.dwBBitMask);

        if((DirectX.DDPixelFormat.dwFlags&DDPF_RGB) != 0 &&
           GUI.ScreenDepth == 16 &&
           DirectX.DDPixelFormat.dwRBitMask == 0xF800 &&
           DirectX.DDPixelFormat.dwGBitMask == 0x07E0 &&
           DirectX.DDPixelFormat.dwBBitMask == 0x001F)
        {
            S9xSetRenderPixelFormat (RGB565);
            Init_2xSaI (565);
        }
        else
            if( (DirectX.DDPixelFormat.dwFlags&DDPF_RGB) != 0 &&
                GUI.ScreenDepth == 16 &&
                DirectX.DDPixelFormat.dwRBitMask == 0x7C00 &&
                DirectX.DDPixelFormat.dwGBitMask == 0x03E0 &&
                DirectX.DDPixelFormat.dwBBitMask == 0x001F)
            {
                S9xSetRenderPixelFormat (RGB555);
                Init_2xSaI (555);
            }
            else
                if((DirectX.DDPixelFormat.dwFlags&DDPF_RGB) != 0 &&
                   GUI.ScreenDepth == 16 &&
                   DirectX.DDPixelFormat.dwRBitMask == 0x001F &&
                   DirectX.DDPixelFormat.dwGBitMask == 0x07E0 &&
                   DirectX.DDPixelFormat.dwBBitMask == 0xF800)
                {
                    S9xSetRenderPixelFormat (BGR565);
                    Init_2xSaI (565);
                }
                else
                    if( (DirectX.DDPixelFormat.dwFlags&DDPF_RGB) != 0 &&
                        GUI.ScreenDepth == 16 &&
                        DirectX.DDPixelFormat.dwRBitMask == 0x001F &&
                        DirectX.DDPixelFormat.dwGBitMask == 0x03E0 &&
                        DirectX.DDPixelFormat.dwBBitMask == 0x7C00)
                    {
                        S9xSetRenderPixelFormat (BGR555);
                        Init_2xSaI (555);
                    }
                    else
                        if (DirectX.DDPixelFormat.dwRGBBitCount == 8 ||
                            DirectX.DDPixelFormat.dwRGBBitCount == 24 ||
                            DirectX.DDPixelFormat.dwRGBBitCount == 32)
                        {
                            S9xSetRenderPixelFormat (RGB565);
                            Init_2xSaI (565);
                        }
        
        if (!VOODOO_MODE &&
            !OPENGL_MODE &&
            ((GUI.ScreenDepth == 8 && Settings.SixteenBit) ||
             (GUI.ScreenDepth == 16 && !Settings.SixteenBit) ||
             GUI.ScreenDepth == 24 || GUI.ScreenDepth == 32))
            GUI.NeedDepthConvert = TRUE;
        
        if (Settings.SixteenBit && 
            (GUI.ScreenDepth == 24 || GUI.ScreenDepth == 32))
        {
            GUI.RedShift += 3;
            GUI.GreenShift += 3;
            GUI.BlueShift += 3;
        }
    }

    int l = 0;
    int i;

    for (i = 0; i < 6; i++)
    {
	int r = (i * 31) / (6 - 1);
	for (int j = 0; j < 6; j++)
	{
	    int g = (j * 31) / (6 - 1);
	    for (int k = 0; k < 6; k++)
	    { 
		int b = (k * 31) / (6 - 1);

		FixedColours [l].red = r << 3;
		FixedColours [l].green = g << 3;
		FixedColours [l++].blue = b << 3;
	    }
	}
    }

    int *color_diff = new int [0x10000];
    int diffr, diffg, diffb, maxdiff = 0, won = 0, lost;
    int r, d = 8;
    for (r = 0; r <= (int) MAX_RED; r++)
    {
	int cr, g, q;
      
	int k = 6 - 1;
	cr = (r * k) / MAX_RED;
	q  = (r * k) % MAX_RED;
	if (q > d && cr < k) 
	    cr++;
	diffr = abs (cr * k - r);
	for (g = 0; g <= (int) MAX_GREEN; g++)
	{
	    int cg, b;
	  
	    k  = 6 - 1;
	    cg = (g * k) / MAX_GREEN;
	    q  = (g * k) % MAX_GREEN;
	    if(q > d && cg < k)
		cg++;
	    diffg = abs (cg * k - g);
	    for (b = 0; b <= (int) MAX_BLUE; b++) 
	    {
		int cb;
		int rgb = BUILD_PIXEL2(r, g, b);

		k  = 6 - 1;
		cb = (b * k) / MAX_BLUE;
		q  = (b * k) % MAX_BLUE;
		if (q > d && cb < k)
		    cb++;
		diffb = abs (cb * k - b);
		palette[rgb] = (cr * 6 + cg) * 6 + cb;
		color_diff[rgb] = diffr + diffg + diffb;
		if (color_diff[rgb] > maxdiff)
		    maxdiff = color_diff[rgb];
	    }
	}
    }

    while (maxdiff > 0 && l < 256)
    {
	int newmaxdiff = 0;
	lost = 0; won++;
	for (r = MAX_RED; r >= 0; r--)
	{
	    int g;
      
	    for (g = MAX_GREEN; g >= 0; g--)
	    {
		int b;
	  
		for (b = MAX_BLUE; b >= 0; b--) 
		{
		    int rgb = BUILD_PIXEL2(r, g, b);

		    if (color_diff[rgb] == maxdiff)
		    {
			if (l >= 256)
			    lost++;
			else
			{
			    FixedColours [l].red = r << 3;
			    FixedColours [l].green = g << 3;
			    FixedColours [l].blue = b << 3;
			    palette [rgb] = l++;
			}
			color_diff[rgb] = 0;
		    }
		    else
			if (color_diff[rgb] > newmaxdiff)
			    newmaxdiff = color_diff[rgb];
		    
		}
	    }
	}
	maxdiff = newmaxdiff;
    }
    delete [] color_diff;
}

void Convert8To24 (SSurface *src, SSurface *dst, RECT *srect)
{
    uint32 brightness = IPPU.MaxBrightness >> 1;
    uint32 conv [256];
    int height = srect->bottom - srect->top;
    int width = srect->right - srect->left;
    int offset1 = srect->top * src->Pitch + srect->left;
    int offset2 = ((dst->Height - height) >> 1) * dst->Pitch +
        ((dst->Width - width) >> 1) * sizeof (uint32);

    for (int p = 0; p < 256; p++)
    {
        uint32 pixel = PPU.CGDATA [p];
        conv [p] = (((pixel & 0x1f) * brightness) << GUI.RedShift) |
                   ((((pixel >> 5) & 0x1f) * brightness) << GUI.GreenShift) |
                   ((((pixel >> 10) & 0x1f) * brightness) << GUI.BlueShift);
    }
    for (register int y = 0; y < height; y++)
    {
        register uint8 *s = ((uint8 *) src->Surface + y * src->Pitch + offset1);
        register uint32 *d = (uint32 *) ((uint8 *) dst->Surface + 
                                         y * dst->Pitch + offset2);
        for (register int x = 0; x < width; x++)
            *d++ = conv [PPU.CGDATA [*s++]];
    }
}

void Convert16To24 (SSurface *src, SSurface *dst, RECT *srect)
{
    int height = srect->bottom - srect->top;
    int width = srect->right - srect->left;
    int offset1 = srect->top * src->Pitch + srect->left * 2;
    int offset2 = ((dst->Height - height) >> 1) * dst->Pitch +
        ((dst->Width - width) >> 1) * sizeof (uint32);

    for (register int y = 0; y < height; y++)
    {
        register uint16 *s = (uint16 *) ((uint8 *) src->Surface + y * src->Pitch + offset1);
        register uint32 *d = (uint32 *) ((uint8 *) dst->Surface + 
                                         y * dst->Pitch + offset2);
        for (register int x = 0; x < width; x++)
        {
            uint32 pixel = *s++;
            *d++ = (((pixel >> 11) & 0x1f) << GUI.RedShift) |
                   (((pixel >> 6) & 0x1f) << GUI.GreenShift) |
                   ((pixel & 0x1f) << GUI.BlueShift);
        }
    }
}

void Convert8To24Packed (SSurface *src, SSurface *dst, RECT *srect)
{
    uint32 brightness = IPPU.MaxBrightness >> 1;
    uint8 levels [32];
    int height = srect->bottom - srect->top;
    int width = srect->right - srect->left;
    int offset1 = srect->top * src->Pitch + srect->left;
    int offset2 = ((dst->Height - height) >> 1) * dst->Pitch +
        ((dst->Width - width) >> 1) * 3;

    for (int l = 0; l < 32; l++)
	levels [l] = l * brightness;
	
    for (register int y = 0; y < height; y++)
    {
        register uint8 *s = ((uint8 *) src->Surface + y * src->Pitch + offset1);
        register uint8 *d = ((uint8 *) dst->Surface + y * dst->Pitch + offset2);
        
#ifdef LSB_FIRST
        if (GUI.RedShift < GUI.BlueShift)
#else	    
	if (GUI.RedShift > GUI.BlueShift)
#endif
        {
            // Order is RGB
            for (register int x = 0; x < width; x++)
            {
                uint16 pixel = PPU.CGDATA [*s++];
                *(d + 0) = levels [(pixel & 0x1f)];
                *(d + 1) = levels [((pixel >> 5) & 0x1f)];
                *(d + 2) = levels [((pixel >> 10) & 0x1f)];
                d += 3;
            }
        }
        else
        {
            // Order is BGR
            for (register int x = 0; x < width; x++)
            {
                uint16 pixel = PPU.CGDATA [*s++];
                *(d + 0) = levels [((pixel >> 10) & 0x1f)];
                *(d + 1) = levels [((pixel >> 5) & 0x1f)];
                *(d + 2) = levels [(pixel & 0x1f)];
                d += 3;
            }
        }
    }
}

void Convert16To24Packed (SSurface *src, SSurface *dst, RECT *srect)
{
    int height = srect->bottom - srect->top;
    int width = srect->right - srect->left;
    int offset1 = srect->top * src->Pitch + srect->left * 2;
    int offset2 = ((dst->Height - height) >> 1) * dst->Pitch +
        ((dst->Width - width) >> 1) * 3;

    for (register int y = 0; y < height; y++)
    {
        register uint16 *s = (uint16 *) ((uint8 *) src->Surface + y * src->Pitch + offset1);
        register uint8 *d = ((uint8 *) dst->Surface + y * dst->Pitch + offset2);
        
#ifdef LSB_FIRST
        if (GUI.RedShift < GUI.BlueShift)
#else	    
	if (GUI.RedShift > GUI.BlueShift)
#endif
        {
            // Order is RGB
            for (register int x = 0; x < width; x++)
            {
                uint32 pixel = *s++;
                *(d + 0) = (pixel >> (11 - 3)) & 0xf8;
                *(d + 1) = (pixel >> (6 - 3)) & 0xf8;
                *(d + 2) = (pixel & 0x1f) << 3;
                d += 3;
            }
        }
        else
        {
            // Order is BGR
            for (register int x = 0; x < width; x++)
            {
                uint32 pixel = *s++;
                *(d + 0) = (pixel & 0x1f) << 3;
                *(d + 1) = (pixel >> (6 - 3)) & 0xf8;
                *(d + 2) = (pixel >> (11 - 3)) & 0xf8;
                d += 3;
            }
        }
    }
}

void Convert16To8 (SSurface *src, SSurface *dst, RECT *srect)
{
    int height = srect->bottom - srect->top;
    int width = srect->right - srect->left;
    int offset1 = srect->top * src->Pitch + srect->left * 2;
    int offset2 = ((dst->Height - height) >> 1) * dst->Pitch +
        ((dst->Width - width) >> 1);

    for (register int y = 0; y < height; y++)
    {
        register uint16 *s = (uint16 *) ((uint8 *) src->Surface + y * src->Pitch + offset1);
        register uint8 *d = ((uint8 *) dst->Surface + y * dst->Pitch + offset2);

        for (register int x = 0; x < width; x++)
            *d++ = palette [*s++];
    }
}

void Convert8To16 (SSurface *src, SSurface *dst, RECT *srect)
{
    uint32 levels [32];
    uint32 conv [256];
    int height = srect->bottom - srect->top;
    int width = srect->right - srect->left;
    int offset1 = srect->top * src->Pitch + srect->left;
    int offset2 = ((dst->Height - height) >> 1) * dst->Pitch +
        ((dst->Width - width) >> 1) * sizeof (uint16);

    for (int l = 0; l < 32; l++)
	levels [l] = (l * IPPU.MaxBrightness) >> 4;
	
    for (int p = 0; p < 256; p++)
    {
        uint32 pixel = PPU.CGDATA [p];
        
        conv [p] = (levels [pixel & 0x1f] << GUI.RedShift) |
                   (levels [(pixel >> 5) & 0x1f] << GUI.GreenShift) |
                   (levels [(pixel >> 10) & 0x1f] << GUI.BlueShift);
    }
    for (register int y = 0; y < height; y++)
    {
        register uint8 *s = ((uint8 *) src->Surface + y * src->Pitch + offset1);
        register uint16 *d = (uint16 *) ((uint8 *) dst->Surface + 
                                         y * dst->Pitch + offset2);
        for (register int x = 0; x < width; x += 2)
        {
            *(uint32 *) d = conv [*s] | (conv [*(s + 1)] << 16);
            s += 2;
            d += 2;
        }
    }
}

void ConvertDepth (SSurface *src, SSurface *dst, RECT *srect)
{
    if (Settings.SixteenBit)
    {
        // SNES image has been rendered in 16-bit, RGB565 format
        switch (GUI.ScreenDepth)
        {
            case 8:
                Convert16To8 (src, dst, srect);
                break;
            case 15:
            case 16:
                break;
            case 24:
                Convert16To24Packed (src, dst, srect);
                break;
            case 32:
                Convert16To24 (src, dst, srect);
                break;
        }
    }
    else
    {
        // SNES image has been rendered only in 8-bits
        switch (GUI.ScreenDepth)
        {
            case 8:
                break;
            case 15:
            case 16:
                Convert8To16 (src, dst, srect);
                break;
            case 24:
                Convert8To24Packed (src, dst, srect);
                break;
            case 32:
                Convert8To24 (src, dst, srect);
                break;
        }
    }
    srect->left = (dst->Width - src->Width) >> 1;
    srect->right = srect->left + src->Width;
    srect->top = (dst->Height - src->Height) >> 1;
    srect->bottom = srect->top + src->Height;
}

void S9xAutoSaveSRAM ()
{
    Memory.SaveSRAM (S9xGetFilename (".srm"));
}

void S9xSetPause (uint32 mask)
{
    Settings.ForcedPause |= mask;
}

void S9xClearPause (uint32 mask)
{
    Settings.ForcedPause &= ~mask;
    if (!Settings.ForcedPause)
    {
        // Wake up the main loop thread just if its blocked in a GetMessage
        // call.
        PostMessage (GUI.hWnd, WM_NULL, 0, 0);
    }
}

static int S9xCompareSDD1IndexEntries (const void *p1, const void *p2)
{
    return (*(uint32 *) p1 - *(uint32 *) p2);
}

void S9xLoadSDD1Data ()
{
    char filename [_MAX_PATH + 1];
    char index [_MAX_PATH + 1];
    char data [_MAX_PATH + 1];

	Settings.SDD1Pack=FALSE;
    Memory.FreeSDD1Data ();

    if (strncmp (Memory.ROMName, TEXT("Star Ocean"), 10) == 0)
	{
		if(strlen(GUI.StarOceanPack)!=0)
			strcpy(filename, GUI.StarOceanPack);
		else Settings.SDD1Pack=TRUE;
	}
    else if(strncmp(Memory.ROMName, TEXT("STREET FIGHTER ALPHA2"), 21)==0)
	{
		if(Memory.ROMRegion==1)
		{
			if(strlen(GUI.SFA2NTSCPack)!=0)
				strcpy(filename, GUI.SFA2NTSCPack);
			else Settings.SDD1Pack=TRUE;
		}
		else
		{
			if(strlen(GUI.SFA2PALPack)!=0)
				strcpy(filename, GUI.SFA2PALPack);
			else Settings.SDD1Pack=TRUE;
		}
	}
	else
	{
		if(strlen(GUI.SFZ2Pack)!=0)
			strcpy(filename, GUI.SFZ2Pack);
		else Settings.SDD1Pack=TRUE;
	}

	if(Settings.SDD1Pack==TRUE)
		return;

    strcpy (index, filename);
    strcat (index, "\\SDD1GFX.IDX");
    strcpy (data, filename);
    strcat (data, "\\SDD1GFX.DAT");

    FILE *fs = fopen (index, "rb");
    int len = 0;
    
    if (fs)
    {
        // Index is stored as a sequence of entries, each entry being
        // 12 bytes consisting of:
        // 4 byte key: (24bit address & 0xfffff * 16) | translated block
        // 4 byte ROM offset
        // 4 byte length
        fseek (fs, 0, SEEK_END);
        len = ftell (fs);
        rewind (fs);
        Memory.SDD1Index = (uint8 *) malloc (len);
        fread (Memory.SDD1Index, 1, len, fs);
        fclose (fs);
        Memory.SDD1Entries = len / 12;
        
        if (!(fs = fopen (data, "rb")))
        {
            free ((char *) Memory.SDD1Index);
            Memory.SDD1Index = NULL;
            Memory.SDD1Entries = 0;
        }
        else
        {
            fseek (fs, 0, SEEK_END);
            len = ftell (fs);
            rewind (fs);
            Memory.SDD1Data = (uint8 *) malloc (len);
            fread (Memory.SDD1Data, 1, len, fs);
            fclose (fs);
            
            qsort (Memory.SDD1Index, Memory.SDD1Entries, 12,
                   S9xCompareSDD1IndexEntries);
        }
    }
}

bool JustifierOffscreen()
{
	return (bool)((GUI.MouseButtons&2)!=0);
}

void JustifierButtons(uint32& justifiers)
{
	if(IPPU.Controller==SNES_JUSTIFIER_2)
	{
		if((GUI.MouseButtons&1)||(GUI.MouseButtons&2))
		{
			justifiers|=0x00200;
		}
		if(GUI.MouseButtons&4)
		{
			justifiers|=0x00800;
		}
	}
	else
	{
		if((GUI.MouseButtons&1)||(GUI.MouseButtons&2))
		{
			justifiers|=0x00100;
		}
		if(GUI.MouseButtons&4)
		{
			justifiers|=0x00400;
		}
	}
}

#ifdef MK_APU_RESAMPLE
void ResampleTo16000HzM16(uint16* input, uint16*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i++)
	{
		output[i]=(input[i*2]+input[(2*i)+1])>>1;
	}
}

void ResampleTo16000HzS16(uint16* input, uint16*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i+=2)
	{
		output[i]=(input[i*2]+input[(2*(i+1))])>>1;
		output[i+1]=(input[(i*2)+1]+input[(2*(i+1))+1])>>1;
	}
}
void ResampleTo8000HzM16(uint16* input, uint16*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i++)
	{
		output[i]=(input[i*4]+input[(4*i)+1]+input[(4*i)+2]+input[(4*i)+3])>>2;
	}
}

void ResampleTo8000HzS16(uint16* input, uint16*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i+=2)
	{
		output[i]=(input[i*4]+input[(4*i)+2]+input[(4*(i+1))]+input[(4*(i+1))+2])>>2;
		output[i+1]=(input[(i*4)+1]+input[(4*i)+3]+input[(4*(i+1))+1]+input[(4*(i+1))+3])>>2;
	}
}

void ResampleTo16000HzM8(uint8* input, uint8*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i++)
	{
		output[i]=(input[i*2]+input[(2*i)+1])>>1;
	}
}

void ResampleTo16000HzS8(uint8* input, uint8*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i+=2)
	{
		output[i]=(input[i*2]+input[(2*(i+1))])>>1;
		output[i+1]=(input[(i*2)+1]+input[(2*(i+1))+1])>>1;
	}
}
void ResampleTo8000HzM8(uint8* input, uint8*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i++)
	{
		output[i]=(input[i*4]+input[(4*i)+1]+input[(4*i)+2]+input[(4*i)+3])>>2;
	}
}

void ResampleTo8000HzS8(uint8* input, uint8*output,int output_samples)
{
	int i=0;
	for(i=0;i<output_samples;i+=2)
	{
		output[i]=(input[i*4]+input[(4*i)+2]+input[(4*(i+1))]+input[(4*(i+1))+2])>>2;
		output[i+1]=(input[(i*4)+1]+input[(4*i)+3]+input[(4*(i+1))+1]+input[(4*(i+1))+3])>>2;
	}
}

#endif

void DoAVIOpen(const char* filename)
{
	// close current instance
	if(GUI.AVIOut)
	{
		AVIClose(&GUI.AVIOut);
		GUI.AVIOut = NULL;
	}

	// create new writer
	AVICreate(&GUI.AVIOut);

	int framerate = (Settings.PAL) ? 50 : 60;
	int frameskip = Settings.SkipFrames;
	if(frameskip == 0 || frameskip == AUTO_FRAMERATE)
		frameskip = 1;

	AVISetFramerate(framerate, frameskip, GUI.AVIOut);

	BITMAPINFOHEADER bi;
	memset(&bi, 0, sizeof(bi));
	bi.biSize = 0x28;    
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biWidth = IPPU.RenderedScreenWidth;
	bi.biHeight = IPPU.RenderedScreenHeight;
	bi.biSizeImage = 3*bi.biWidth*bi.biHeight;

	AVISetVideoFormat(&bi, GUI.AVIOut);

	if(!Settings.Mute && SoundBuffer)
	{
		AVISetSoundFormat(&DirectX.wfx, GUI.AVIOut);
	}

	if(!AVIBegin(filename, GUI.AVIOut))
	{
		AVIClose(&GUI.AVIOut);
		GUI.AVIOut = NULL;
		return;
	}

	// init buffers
	avi_sound_samples_per_update = (DirectX.wfx.nSamplesPerSec * frameskip) / framerate;
	avi_sound_bytes_per_sample = DirectX.wfx.nBlockAlign;

	avi_buffer = new uint8[3*IPPU.RenderedScreenWidth*IPPU.RenderedScreenHeight];
	avi_sound_buffer = new uint8[avi_sound_samples_per_update * avi_sound_bytes_per_sample];

	avi_width = IPPU.RenderedScreenWidth;
	avi_height = IPPU.RenderedScreenHeight;
	avi_skip_frames = Settings.SkipFrames;
}

void DoAVIClose(int reason)
{
	if(!GUI.AVIOut)
	{
		return;
	}

	AVIClose(&GUI.AVIOut);
	GUI.AVIOut = NULL;

	delete [] avi_buffer;
	delete [] avi_sound_buffer;

	avi_buffer = NULL;
	avi_sound_buffer = NULL;

	switch(reason)
	{
	case 1:
		// emu settings changed
		S9xMessage(S9X_INFO, S9X_AVI_INFO, AVI_CONFIGURATION_CHANGED);
		break;
	default:
		// print nothing
		break;
	}
}

static void DoAVIVideoFrame(SSurface* source_surface, int Width, int Height, bool8 sixteen_bit)
{
	if(!GUI.AVIOut)
	{
		return;
	}

	// check configuration
	const WAVEFORMATEX* pwfex = NULL;
	if(avi_width != Width ||
		avi_height != Height ||
		avi_skip_frames != Settings.SkipFrames ||
		(AVIGetSoundFormat(GUI.AVIOut, &pwfex) && memcmp(pwfex, &DirectX.wfx, sizeof(WAVEFORMATEX))))
	{
		DoAVIClose(1);
		return;
	}
	
	// convert to bitdepth 24
	SSurface avi_dest_surface;
	RECT full_rect;
	avi_dest_surface.Surface = avi_buffer;
	avi_dest_surface.Pitch = Width * 3;
	avi_dest_surface.Width = Width;
	avi_dest_surface.Height = Height;
	full_rect.top = 0;
	full_rect.left = 0;
	full_rect.bottom = Height;
	full_rect.right = Width;
	if(sixteen_bit)
	{
		Convert16To24Packed(source_surface, &avi_dest_surface, &full_rect);
	}
	else
	{
		Convert8To24Packed(source_surface, &avi_dest_surface, &full_rect);
	}

	// flip the image vertically
	const int pitch = 3*Width;
	int y;
	for(y=0; y<Height>>1; ++y)
	{
		uint8* lo_8 = avi_buffer+y*pitch;
		uint8* hi_8 = avi_buffer+(Height-1-y)*pitch;
		uint32* lo_32=(uint32*)lo_8;
		uint32* hi_32=(uint32*)hi_8;

		int q;
		{
			register uint32 a, b;
			for(q=pitch>>4; q>0; --q)
			{
				a=*lo_32;  b=*hi_32;  *lo_32=b;  *hi_32=a;  ++lo_32;  ++hi_32;
				a=*lo_32;  b=*hi_32;  *lo_32=b;  *hi_32=a;  ++lo_32;  ++hi_32;
				a=*lo_32;  b=*hi_32;  *lo_32=b;  *hi_32=a;  ++lo_32;  ++hi_32;
				a=*lo_32;  b=*hi_32;  *lo_32=b;  *hi_32=a;  ++lo_32;  ++hi_32;
			}
		}

		{
			register uint8 c, d;
			for(q=(pitch&0x0f); q>0; --q)
			{
				c=*lo_8;  d=*hi_8;  *lo_8=d;  *hi_8=c;
			}
		}
	}

	// write to AVI
	AVIAddVideoFrame(avi_buffer, GUI.AVIOut);

	// generate sound
	if(pwfex)
	{
		const int _maxsamplecount = SOUND_BUFFER_SIZE / avi_sound_bytes_per_sample;
		const int stereo_multiplier = (Settings.Stereo) ? 2 : 1;
		int samples_written = 0;
		int sample_count = avi_sound_samples_per_update;
		while(sample_count > _maxsamplecount)
		{
			S9xMixSamples(avi_sound_buffer+(samples_written*avi_sound_bytes_per_sample), _maxsamplecount*stereo_multiplier);
			sample_count -= _maxsamplecount;
			samples_written += _maxsamplecount;
		}
		S9xMixSamples(avi_sound_buffer+(samples_written*avi_sound_bytes_per_sample), sample_count*stereo_multiplier);

		AVIAddSoundSamples(avi_sound_buffer, avi_sound_samples_per_update, GUI.AVIOut);
	}
}

