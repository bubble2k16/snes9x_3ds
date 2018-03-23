
#include <stdio.h>
#include <cstring>

#include "3dsgpu.h"
#include "3dssound.h"
#include "3dsopt.h"

#include "3dsimpl.h"

#define LEFT_CHANNEL        10
#define RIGHT_CHANNEL       11


SSND3DS snd3DS;

int debugSoundCounter = 0;

int csndTicksPerSecond = 267951600;           // use this for 44100 Khz
int snd3dsSampleRate = 44100;
int snd3dsSamplesPerLoop = 735;
bool snd3dsIsStereo = true;
bool snd3dsSpawnMixingThread = true;
int snd3dsMinLoopBuffer = 1;
int snd3dsMaxLoopBuffer = 2;

#define SAMPLEBUFFER_SIZE 32000


//---------------------------------------------------------
// Computes the truncated number of samples per loop by
// dividing the the ideal sample rate by the total
// number of loops to be executed per second. 
//
// Usually loopsPerSecond is the frame rate. If you want
// to generate samples twice per frame, then this value
// will be the frame rate x 2.
//---------------------------------------------------------
int snd3dsComputeSamplesPerLoop(int idealSampleRate, int loopsPerSecond)
{
    return idealSampleRate / loopsPerSecond;
}


//---------------------------------------------------------
// Computes the final sample rate by taking the 
// samples generate per loop multiplying by the 
// number of loops in a second.
//---------------------------------------------------------
int snd3dsComputeSampleRate(int idealSampleRate, int loopsPerSecond)
{
	return (idealSampleRate / loopsPerSecond) * loopsPerSecond;
}


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

    t3dsStartTiming(44, "Mix");
    bool generateSound = false;
    if (snd3DS.isPlaying)
    {
        impl3dsGenerateSoundSamples(snd3dsSamplesPerLoop);
        generateSound = true;
    }
    t3dsEndTiming(44);

    t3dsStartTiming(41, "Mix-Timing");
    long generateAtSamplePosition = 0;
    while (true)
    {
        s64 nowSamplePosition = snd3dsGetSamplePosition();
        s64 deltaTimeAhead = snd3DS.upToSamplePosition - nowSamplePosition;
        long blocksAhead = deltaTimeAhead / snd3dsSamplesPerLoop;

        if (blocksAhead < snd3dsMinLoopBuffer)
        {
            // buffer is about to underrun.
            //
            generateAtSamplePosition =
                ((u64)((nowSamplePosition + snd3dsSamplesPerLoop - 1) / snd3dsSamplesPerLoop)) * snd3dsSamplesPerLoop +
                snd3dsMinLoopBuffer * snd3dsSamplesPerLoop;
            break;
        }
        else if (blocksAhead < snd3dsMaxLoopBuffer)
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
        //async3dsExecuteTasks();
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
            impl3dsOutputSoundSamples(snd3dsSamplesPerLoop, &snd3DS.leftBuffer[p], &snd3DS.rightBuffer[p]);
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

    // Doing GSPGPU_FlushDataCache may cause race conditions with
    // the battery check (and other stuff?) on the main thread.
    // causing the whole emulator to freeze. 
    //
    // So we only flush data cache if the snd3DS.isPlaying is true.
    //
    if (blockCount % MIN_FORWARD_BLOCKS == 0 && snd3DS.isPlaying)
        CSND_FlushDataCache(snd3DS.fullBuffers, SAMPLEBUFFER_SIZE * 2 * 2);
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
            svcSleepThread(100000 * 1);

        if (snd3DS.isPlaying)
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

    // compute the ticks per second.
    csndTicksPerSecond = (u64)timer * 4 * sampleRate;

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
        for (int i = 0; i < SAMPLEBUFFER_SIZE; i++)
        {
            snd3DS.leftBuffer[i] = 0;
            snd3DS.rightBuffer[i] = 0;
        }
        CSND_FlushDataCache(snd3DS.fullBuffers, SAMPLEBUFFER_SIZE * 2 * 2);
        
        // CSND
        // Fix: Copied libctru's csndPlaySound and modified it so that it will
        // not play immediately upon calling. This seems to solve the left
        // channel louder than right channel problem.
        //
        if (snd3dsIsStereo)
        {
            snd3dsPlaySound(LEFT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, snd3dsSampleRate, 1.0f, -1.0f, (u32*)snd3DS.leftBuffer, (u32*)snd3DS.leftBuffer, snd3dsSampleRate * 2);
            snd3dsPlaySound(RIGHT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, snd3dsSampleRate, 1.0f, 1.0f, (u32*)snd3DS.rightBuffer, (u32*)snd3DS.rightBuffer, snd3dsSampleRate * 2);
        }
        else
        {
            snd3dsPlaySound(LEFT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, snd3dsSampleRate, 1.0f, 0, (u32*)snd3DS.leftBuffer, (u32*)snd3DS.leftBuffer, snd3dsSampleRate * 2);
        }

        // Flush CSND command buffers
        csndExecCmds(true);
        snd3DS.startTick = svcGetSystemTick();

        // Fix for race condition for 64-bit access in the sound thread.
        snd3DS.upToSamplePosition = snd3dsGetSamplePosition();  
        snd3DS.isPlaying = true;
    }
}


//---------------------------------------------------------
// Stop playing the samples.
//---------------------------------------------------------
void snd3dsStopPlaying()
{
    if (snd3DS.isPlaying)
    {
        snd3DS.isPlaying = false;
        CSND_SetPlayState(LEFT_CHANNEL, 0);
        CSND_SetPlayState(RIGHT_CHANNEL, 0);

        // Flush CSND command buffers
        csndExecCmds(true);
    }
}


//---------------------------------------------------------
// Set the sampling rate.
//
// This function should be called by the 
// impl3dsInitializeCore function. It CANNOT be called
// after the snd3dsInitialize function is called.
//---------------------------------------------------------
void snd3dsSetSampleRate(
    bool isStereo, 
    int idealSampleRate, 
    int loopsPerSecond, 
    bool spawnMixingThread,
    int minLoopBuffer, 
    int maxLoopBuffer)
{
    snd3dsIsStereo = isStereo;
    if (idealSampleRate > 44100)
        idealSampleRate = 44100;
    snd3dsSampleRate = snd3dsComputeSampleRate(idealSampleRate, loopsPerSecond);
    snd3dsSamplesPerLoop = snd3dsComputeSamplesPerLoop(idealSampleRate, loopsPerSecond);
    snd3dsSpawnMixingThread = spawnMixingThread;
    snd3dsMinLoopBuffer = minLoopBuffer;
    snd3dsMaxLoopBuffer = maxLoopBuffer;
}


//---------------------------------------------------------
// Initialize the CSND library.
//---------------------------------------------------------
bool snd3dsInitialize()
{
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
    snd3DS.fullBuffers = (short *)linearAlloc(SAMPLEBUFFER_SIZE * 2 * 2);
	snd3DS.leftBuffer = &snd3DS.fullBuffers[0];
	snd3DS.rightBuffer = &snd3DS.fullBuffers[SAMPLEBUFFER_SIZE];
    memset(snd3DS.fullBuffers, 0, sizeof(SAMPLEBUFFER_SIZE * 2 * 2));

    if (!snd3DS.fullBuffers)
    {
        printf ("Unable to allocate sound buffers\n");
        snd3dsFinalize();
        return false;
    }

#ifndef RELEASE
    printf ("snd3dsInit - Allocate L/R buffers\n");
#endif

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

        snd3DS.mixingThreadHandle = NULL;
        
        if (snd3dsSpawnMixingThread)
        {
    #ifndef RELEASE
            printf ("snd3dsInit - Mix Stack: %x\n", snd3DS.mixingThreadStack);
            printf ("snd3dsInit - Mix ThreadFunc: %x\n", &snd3dsMixingThread);
    #endif
            ret = svcCreateThread(&snd3DS.mixingThreadHandle, snd3dsMixingThread, 0,
                (u32*)(snd3DS.mixingThreadStack+0x4000), 0x18, 1);
            if (ret)
            {
                printf("Unable to start Mix thread: %x\n", ret);
                snd3dsFinalize();
                return false;
            }
#ifndef RELEASE
            printf ("snd3dsInit - Create Mix thread %x\n", snd3DS.mixingThreadHandle);
#endif
        }
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
