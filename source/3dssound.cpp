
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

#define LEFT_CHANNEL        10
#define RIGHT_CHANNEL       11
//#define DUMMY_CHANNEL       12



int debugSoundCounter = 0;
int csndTicksPerSecond = 268033000LL;           // use this for 32000 Khz
//int csndTicksPerSecond = 268100000LL;           // use this for 21600 Khz


u64 snd3dsGetSamplePosition() {
	u64 delta = (svcGetSystemTick() - snd3DS.startTick);
	u64 samplePosition = delta * SAMPLE_RATE / csndTicksPerSecond;

    snd3DS.samplePosition = samplePosition;
	return samplePosition;
}

int blockCount = 0;

void snd3dsMixSamples()
{
    #define SAMPLES_TO_GENERATE         256

    #define MIN_FORWARD_BLOCKS          8
    #define MAX_FORWARD_BLOCKS          16

    t3dsStartTiming(44, "Mix-S9xMix");
    bool generateSound = false;
    if (snd3DS.isPlaying && !snd3DS.generateSilence)
    {
        S9xSetAPUDSPReplay ();
        S9xMixSamplesIntoTempBuffer(SAMPLES_TO_GENERATE * 2);
        generateSound = true;
    }
    t3dsEndTiming(44);

    t3dsStartTiming(41, "Mix-Timing");
    long generateAtSamplePosition = 0;
    while (true)
    {
        u64 nowSamplePosition = snd3dsGetSamplePosition();
        u64 deltaTimeAhead = snd3DS.upToSamplePosition - nowSamplePosition;
        long blocksAhead = deltaTimeAhead / SAMPLES_TO_GENERATE;

        if (blocksAhead < MIN_FORWARD_BLOCKS)
        {
            // buffer is about to underrun.
            //
            generateAtSamplePosition =
                ((u64)((nowSamplePosition + SAMPLES_TO_GENERATE - 1) / SAMPLES_TO_GENERATE)) * SAMPLES_TO_GENERATE +
                MIN_FORWARD_BLOCKS * SAMPLES_TO_GENERATE;
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
    snd3DS.upToSamplePosition = generateAtSamplePosition + SAMPLES_TO_GENERATE;
    t3dsEndTiming(41);



    t3dsStartTiming(42, "Mix-Copy+Vol");
    int p = generateAtSamplePosition % BUFFER_SIZE;

    if (snd3DS.audioType==1)
    {
        if (generateSound)
        {
            S9xApplyMasterVolumeOnTempBufferIntoLeftRightBuffers(&snd3DS.leftBuffer[p], &snd3DS.rightBuffer[p], SAMPLES_TO_GENERATE * 2);
        }
        else
        {
            for (int i = 0; i < SAMPLES_TO_GENERATE; i++)
            {
                snd3DS.leftBuffer[p + i] = 0;
                snd3DS.rightBuffer[p + i] = 0;
            }
        }
        /*FILE *fp = fopen("sample.dat", "ab");
        for (int i = 0; i < SAMPLES_TO_GENERATE; i++)
        {
            printf ("%7d ", snd3DS.leftBuffer[p + i]);
            fwrite (&snd3DS.leftBuffer[p + i], 2, 1, fp);
        }
        fclose(fp);*/
    }
    /*else
    {
        if (generateSound)
            S9xApplyMasterVolumeOnTempBufferIntoLeftRightBuffersNDSP(&snd3DS.fullBuffers[p], SAMPLES_TO_GENERATE * 2);
        else
        {
            for (int i = 0; i < SAMPLES_TO_GENERATE; i++)
            {
                snd3DS.leftBuffer[p + i] = 0;
                snd3DS.rightBuffer[p + i] = 0;
            }
        }
    }*/
    t3dsEndTiming(42);

    // Now that we have the samples, we have to copy it back into our buffers
    // for the 3DS to playback
    //
    t3dsStartTiming(43, "Mix-Flush");
    blockCount++;
    if (blockCount % MIN_FORWARD_BLOCKS == 0)
        GSPGPU_FlushDataCache(snd3DS.fullBuffers, BUFFER_SIZE * 2 * 2);
    t3dsEndTiming(43);
}

void snd3dsDSPThread(void *p)
{
    snd3DS.upToSamplePosition = snd3dsGetSamplePosition();
    snd3DS.startSamplePosition = snd3DS.upToSamplePosition;
    //svcExitThread();
    //return;

    while (!snd3DS.terminateDSPThread)
    {
        if (!GPU3DS.isReal3DS)
            svcSleepThread(1000000 * 1);
        snd3dsMixSamples();
    }
    snd3DS.terminateDSPThread = -1;
    svcExitThread();
}


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

void snd3dsStartPlaying()
{
    if (!snd3DS.isPlaying)
    {
        // CSND
        // Fix: Copied libctru's csndPlaySound and modified it so that it will
        // not play immediately upon calling. This seems to solve the left
        // channel louder than right channel problem.
        //
        snd3dsPlaySound(LEFT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, SAMPLE_RATE, 1.0f, -1.0f, (u32*)snd3DS.leftBuffer, (u32*)snd3DS.leftBuffer, BUFFER_SIZE * 2);
        snd3dsPlaySound(RIGHT_CHANNEL, SOUND_REPEAT | SOUND_FORMAT_16BIT, SAMPLE_RATE, 1.0f, 1.0f, (u32*)snd3DS.rightBuffer, (u32*)snd3DS.rightBuffer, BUFFER_SIZE * 2);

        // Flush CSND command buffers
        csndExecCmds(true);
        snd3DS.startTick = svcGetSystemTick();
        snd3DS.upToSamplePosition = 0;
        snd3DS.isPlaying = true;
        snd3DS.generateSilence = false;
    }
}

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




bool snd3dsInitialize()
{
    S9xSetPlaybackRate(32000);

    snd3DS.isPlaying = false;
    snd3DS.audioType = 0;
    Result ret = 0;
    ret = csndInit();
    printf ("Trying to initialize CSND, ret = %x\n", ret);
	if (!R_FAILED(ret))
    {
        snd3DS.audioType = 1;
        printf ("CSND Initialized\n");
    }
    else
    {
        printf ("Unable to initialize 3DS CSND service\n");
        return false;
        /*
        //-----------------------------------------------
          NDSP isn't really fully tested yet.
        -----------------------------------------------
        ret = ndspInit();
        printf ("Trying to initialize NDSP, ret = %x\n", ret);

        if (!R_FAILED(ret))
        {
            snd3DS.audioType = 2;
            printf ("NDSP Initialized\n");
        }
        else
        {
            printf ("Unable to initialize 3DS CSND/NDSP service\n");
            return false;
        }*/
    }

    // Initialize the sound buffers
    //
    snd3DS.fullBuffers = (short *)linearAlloc(BUFFER_SIZE * 2 * 2);
	snd3DS.leftBuffer = &snd3DS.fullBuffers[0];
	snd3DS.rightBuffer = &snd3DS.fullBuffers[BUFFER_SIZE];
    memset(snd3DS.fullBuffers, 0, sizeof(BUFFER_SIZE * 2 * 2));

    if (!snd3DS.fullBuffers)
    {
        printf ("Unable to allocate sound buffers\n");
        snd3dsFinalize();
        return false;
    }
    printf ("snd3dsInit - Allocate L/R buffers\n");


    if (snd3DS.audioType == 1)
    {
        snd3dsStartPlaying();
        printf ("snd3dsInit - Start playing CSND buffers\n");
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
        ndspChnSetRate(0, SAMPLE_RATE);
        ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
        ndspChnSetMix(0, stereoMix);
        printf ("snd3dsInit - Set channel state\n");

        memset(&snd3DS.waveBuf, 0, sizeof(ndspWaveBuf));
        snd3DS.waveBuf.data_vaddr = (u32*)snd3DS.fullBuffers;
        snd3DS.waveBuf.nsamples = BUFFER_SIZE;
        snd3DS.waveBuf.looping  = true;
        snd3DS.waveBuf.status = NDSP_WBUF_FREE;

        ndspChnWaveBufAdd(0, &snd3DS.waveBuf);
        printf ("snd3dsInit - Start playing NDSP buffers\n");
    }

    // SNES DSP thread
    snd3DS.terminateDSPThread = false;

    if (GPU3DS.isReal3DS)
    {
        APT_SetAppCpuTimeLimit(30); // enables syscore usage

        printf ("snd3dsInit - DSP Stack: %x\n", snd3DS.dspThreadStack);
        printf ("snd3dsInit - DSP ThreadFunc: %x\n", &snd3dsDSPThread);

        IAPU.DSPReplayIndex = 0;
        IAPU.DSPWriteIndex = 0;
        ret = svcCreateThread(&snd3DS.dspThreadHandle, snd3dsDSPThread, 0,
            (u32*)(snd3DS.dspThreadStack+0x4000), 0x18, 1);
        if (ret)
        {
            printf("Unable to start DSP thread:\n");
            snd3DS.dspThreadHandle = NULL;
            snd3dsFinalize();
            return false;
        }
        printf ("snd3dsInit - Create DSP thread %x\n", snd3DS.dspThreadHandle);
    }

    printf ("snd3DSInit complete\n");

	return true;
}



void snd3dsFinalize()
{
     snd3DS.terminateDSPThread = true;

     if (snd3DS.dspThreadHandle)
     {
         // Wait (at most 1 second) for the sound thread to finish,
         printf ("Join dspThreadHandle\n");
         svcWaitSynchronization(snd3DS.dspThreadHandle, 1000 * 1000000);
         svcCloseHandle(snd3DS.dspThreadHandle);
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
