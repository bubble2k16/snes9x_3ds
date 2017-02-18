
#define _3DSSOUND_CPP_
#include <stdio.h>

#include "snes9x.h"
#include "spc700.h"
#include "apu.h"
#include "soundux.h"

#include "3ds.h"
#include "3dsgpu.h"
#include "3dssound.h"
#include "3dsopt.h"
#include "3dssnes9x.h"
#include "3dsimpl.h"

#define LEFT_CHANNEL        10
#define RIGHT_CHANNEL       11


SSND3DS snd3DS;

int debugSoundCounter = 0;
int csndTicksPerSecond = 268033000LL;           // use this for 32000 Khz

int snd3dsSampleRate = 44100;
int snd3dsSamplesPerLoop = 735;


//---------------------------------------------------------
// Gets the current playing sample position.
//
// The problem is that the existing CSND library
// is unable to provide the actual playing sample
// position from the hardware. So we have to compute
// this manually. 
//
// But computing it this way may cause skews in sound
// generation over time, so the csndTicksPerSecond
// has to be correct to minimize skewing.
//---------------------------------------------------------
u64 snd3dsGetSamplePosition() {
	u64 delta = (svcGetSystemTick() - snd3DS.startTick);
	u64 samplePosition = delta * snd3dsSampleRate / csndTicksPerSecond;

    snd3DS.samplePosition = samplePosition;
	return samplePosition;
}

int blockCount = 0;


//---------------------------------------------------------
// Mix the samples.
//
// This is usually called from within 3dssound.cpp.
// It should only be called externall from other 
// files when running in Citra.
//---------------------------------------------------------
void snd3dsMixSamples()
{
    #define MIN_FORWARD_BLOCKS          8
    #define MAX_FORWARD_BLOCKS          16

    t3dsStartTiming(44, "Mix-Replay+S9xMix");
    bool generateSound = false;
    if (snd3DS.isPlaying && !snd3DS.generateSilence)
    {
        impl3dsGenerateSoundSamples();
        generateSound = true;
    }
    t3dsEndTiming(44);

    t3dsStartTiming(41, "Mix-Timing");
    long generateAtSamplePosition = 0;
    while (true)
    {
        u64 nowSamplePosition = snd3dsGetSamplePosition();
        u64 deltaTimeAhead = snd3DS.upToSamplePosition - nowSamplePosition;
        long blocksAhead = deltaTimeAhead / snd3dsSamplesPerLoop;

        if (blocksAhead < MIN_FORWARD_BLOCKS)
        {
            // buffer is about to underrun.
            //
            generateAtSamplePosition =
                ((u64)((nowSamplePosition + snd3dsSamplesPerLoop - 1) / snd3dsSamplesPerLoop)) * snd3dsSamplesPerLoop +
                MIN_FORWARD_BLOCKS * snd3dsSamplesPerLoop;
            break;
        }
        else if (blocksAhead < MAX_FORWARD_BLOCKS)
        {
            // play head is still within acceptable range.
            // so we place the generated samples at where
            // we left off previously
            //
            generateAtSamplePosition = snd3DS.upToSamplePosition;
            break;
        }

        // blocksAhead >= MAX_FORWARD_BLOCKS
        // although we've already mixed the previous block,
        // we are too ahead of time, so let's wait for 0.1 millisecs.
        //
        // That may help to save some battery.
        //
        svcSleepThread(100000);
    }

    snd3DS.startSamplePosition = generateAtSamplePosition;
    snd3DS.upToSamplePosition = generateAtSamplePosition + snd3dsSamplesPerLoop;
    t3dsEndTiming(41);

    t3dsStartTiming(42, "Mix-ApplyMstVol");
    int p = generateAtSamplePosition % snd3dsSampleRate;

    if (snd3DS.audioType==1)
    {
        if (generateSound)
        {
            impl3dsOutputSoundSamples(&snd3DS.leftBuffer[p], &snd3DS.rightBuffer[p]);
        }
        else
        {
            for (int i = 0; i < snd3dsSamplesPerLoop; i++)
            {
                snd3DS.leftBuffer[p + i] = 0;
                snd3DS.rightBuffer[p + i] = 0;
            }
        }
    }
    t3dsEndTiming(42);

    // Now that we have the samples, we have to copy it back into our buffers
    // for the 3DS to playback
    //
    t3dsStartTiming(43, "Mix-Flush");
    blockCount++;
    if (blockCount % MIN_FORWARD_BLOCKS == 0)
        GSPGPU_FlushDataCache(snd3DS.fullBuffers, snd3dsSampleRate * 2 * 2);
    t3dsEndTiming(43);
}


//---------------------------------------------------------
// This function is the entry point to the sound mixing
// thread.
//---------------------------------------------------------
void snd3dsMixingThread(void *p)
{
    snd3DS.upToSamplePosition = snd3dsGetSamplePosition();
    snd3DS.startSamplePosition = snd3DS.upToSamplePosition;
    //svcExitThread();
    //return;

    while (!snd3DS.terminateMixingThread)
    {
        if (!GPU3DS.isReal3DS)
            svcSleepThread(1000000 * 1);
        snd3dsMixSamples();
    }
    snd3DS.terminateMixingThread = -1;
    svcExitThread();
}


//---------------------------------------------------------
// Triggers the CSND to play the sound from the
// buffers.
//---------------------------------------------------------
Result snd3dsPlaySound(int chn, u32 flags, u32 sampleRate, float vol, float pan, void* data0, void* data1, u32 size)
{
	u32 paddr0 = 0, paddr1 = 0;

	int encoding = (flags >> 12) & 3;
	int loopMode = (flags >> 10) & 3;

	if (!loopMode) flags |= SOUND_ONE_SHOT;

	if (encoding != CSND_ENCODING_PSG)
	{
		if (data0) paddr0 = osConvertVirtToPhys(data0);
		if (data1) paddr1 = osConvertVirtToPhys(data1);

		if (data0 && encoding == CSND_ENCODING_ADPCM)
		{
			int adpcmSample = ((s16*)data0)[-2];
			int adpcmIndex = ((u8*)data0)[-2];
			CSND_SetAdpcmState(chn, 0, adpcmSample, adpcmIndex);
		}
	}

	u32 timer = CSND_TIMER(sampleRate);
	if (timer < 0x0042) timer = 0x0042;
	else if (timer > 0xFFFF) timer = 0xFFFF;
	flags &= ~0xFFFF001F;
	flags |= SOUND_ENABLE | SOUND_CHANNEL(chn) | (timer << 16);

	u32 volumes = CSND_VOL(vol, pan);
	CSND_SetChnRegs(flags, paddr0, paddr1, size, volumes, volumes);

	if (loopMode == CSND_LOOPMODE_NORMAL && paddr1 > paddr0)
	{
		// Now that the first block is playing, configure the size of the subsequent blocks
		size -= paddr1 - paddr0;
		CSND_SetBlock(chn, 1, paddr1, size);
	}
    CSND_SetPlayState(chn, 1);
}


//---------------------------------------------------------
// Start playing the samples.
//---------------------------------------------------------
void snd3dsStartPlaying()
{
    if (!snd3DS.isPlaying)
    {
        // CSND
        // Fix: Copied libctru's csndPlaySound and modified it so that it will
        // not play immediately upon calling. This seems to solve the left
        // channel louder than right channel problem.
        //
        snd3dsPlaySound(LEFT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, snd3dsSampleRate, 1.0f, -1.0f, (u32*)snd3DS.leftBuffer, (u32*)snd3DS.leftBuffer, snd3dsSampleRate * 2);
        snd3dsPlaySound(RIGHT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, snd3dsSampleRate, 1.0f, 1.0f, (u32*)snd3DS.rightBuffer, (u32*)snd3DS.rightBuffer, snd3dsSampleRate * 2);

        // Flush CSND command buffers
        csndExecCmds(true);
        snd3DS.startTick = svcGetSystemTick();
        snd3DS.upToSamplePosition = 0;
        snd3DS.isPlaying = true;
        snd3DS.generateSilence = false;
    }
}


//---------------------------------------------------------
// Stop playing the samples.
//---------------------------------------------------------
void snd3dsStopPlaying()
{
    if (snd3DS.isPlaying)
    {
        CSND_SetPlayState(LEFT_CHANNEL, 0);
        CSND_SetPlayState(RIGHT_CHANNEL, 0);

        // Flush CSND command buffers
        csndExecCmds(true);
        snd3DS.isPlaying = false;
    }
}


//---------------------------------------------------------
// Set the sampling rate.
//
// This function should be called by the 
// impl3dsInitializeCore function. It CANNOT be called
// after the snd3dsInitialize function is called.
//---------------------------------------------------------
void snd3dsSetSampleRate(int sampleRate, int samplesPerLoop)
{
    snd3dsSampleRate = sampleRate;
    snd3dsSamplesPerLoop = samplesPerLoop;
}


//---------------------------------------------------------
// Initialize the CSND library.
//---------------------------------------------------------
bool snd3dsInitialize()
{
    S9xSetPlaybackRate(snd3dsSampleRate);

    snd3DS.isPlaying = false;
    snd3DS.audioType = 0;
    Result ret = 0;
    ret = csndInit();

#ifndef RELEASE
    printf ("Trying to initialize CSND, ret = %x\n", ret);
#endif

	if (!R_FAILED(ret))
    {
        snd3DS.audioType = 1;
#ifndef RELEASE
        printf ("CSND Initialized\n");
#endif
    }
    else
    {
        printf ("Unable to initialize 3DS CSND service\n");
        return false;
    }

    // Initialize the sound buffers
    //
    snd3DS.fullBuffers = (short *)linearAlloc(snd3dsSampleRate * 2 * 2);
	snd3DS.leftBuffer = &snd3DS.fullBuffers[0];
	snd3DS.rightBuffer = &snd3DS.fullBuffers[snd3dsSampleRate];
    memset(snd3DS.fullBuffers, 0, sizeof(snd3dsSampleRate * 2 * 2));

    if (!snd3DS.fullBuffers)
    {
        printf ("Unable to allocate sound buffers\n");
        snd3dsFinalize();
        return false;
    }

#ifndef RELEASE
    printf ("snd3dsInit - Allocate L/R buffers\n");
#endif

    if (snd3DS.audioType == 1)
    {
        snd3dsStartPlaying();
#ifndef RELEASE
        printf ("snd3dsInit - Start playing CSND buffers\n");
#endif
    }
    else
    {
        float stereoMix[12] = { 1.0f, 1.0f, 0, 0, 0,   0, 0, 0, 0, 0,   0, 0 };

        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        ndspSetOutputCount(1);
        ndspSetMasterVol(1.0f);

        // Both left/right channels
        ndspChnReset(0);
        ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
        ndspChnSetRate(0, snd3dsSampleRate);
        ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
        ndspChnSetMix(0, stereoMix);
#ifndef RELEASE
        printf ("snd3dsInit - Set channel state\n");
#endif
        memset(&snd3DS.waveBuf, 0, sizeof(ndspWaveBuf));
        snd3DS.waveBuf.data_vaddr = (u32*)snd3DS.fullBuffers;
        snd3DS.waveBuf.nsamples = snd3dsSampleRate;
        snd3DS.waveBuf.looping  = true;
        snd3DS.waveBuf.status = NDSP_WBUF_FREE;

        ndspChnWaveBufAdd(0, &snd3DS.waveBuf);
#ifndef RELEASE
        printf ("snd3dsInit - Start playing NDSP buffers\n");
#endif
    }

    // SNES DSP thread
    snd3DS.terminateMixingThread = false;

    if (GPU3DS.isReal3DS)
    {
#ifdef LIBCTRU_1_0_0
        aptOpenSession();
        APT_SetAppCpuTimeLimit(30); // enables syscore usage
        aptCloseSession();   
#else
        APT_SetAppCpuTimeLimit(30); // enables syscore usage
#endif

#ifndef RELEASE
        printf ("snd3dsInit - DSP Stack: %x\n", snd3DS.mixingThreadStack);
        printf ("snd3dsInit - DSP ThreadFunc: %x\n", &snd3dsMixingThread);
#endif
        IAPU.DSPReplayIndex = 0;
        IAPU.DSPWriteIndex = 0;
        ret = svcCreateThread(&snd3DS.mixingThreadHandle, snd3dsMixingThread, 0,
            (u32*)(snd3DS.mixingThreadStack+0x4000), 0x18, 1);
        if (ret)
        {
            printf("Unable to start DSP thread: %x\n", ret);
            snd3dsFinalize();
            DEBUG_WAIT_L_KEY
            return false;
        }
#ifndef RELEASE
        printf ("snd3dsInit - Create DSP thread %x\n", snd3DS.mixingThreadHandle);
#endif
    }

#ifndef RELEASE
    printf ("snd3DSInit complete\n");
#endif

	return true;
}


//---------------------------------------------------------
// Finalize the CSND library.
//---------------------------------------------------------
void snd3dsFinalize()
{
     snd3DS.terminateMixingThread = true;

     if (snd3DS.mixingThreadHandle)
     {
         // Wait (at most 1 second) for the sound thread to finish,
#ifndef RELEASE
         printf ("Join mixingThreadHandle\n");
#endif
         svcWaitSynchronization(snd3DS.mixingThreadHandle, 1000 * 1000000);
         svcCloseHandle(snd3DS.mixingThreadHandle);
     }

    if (snd3DS.fullBuffers)  linearFree(snd3DS.fullBuffers);

    if (snd3DS.audioType == 1)
    {
        snd3dsStopPlaying();
        csndExit();
    }
    else if(snd3DS.audioType == 2)
    {
        ndspChnWaveBufClear(0);
        ndspExit();
    }
}
