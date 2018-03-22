#include "3ds.h"

#ifndef _3DSSOUND_H_
#define _3DSSOUND_H_



typedef struct 
{
    bool        isPlaying = false;
    bool        generateSilence = false;
    
    int         audioType = 0;              // 0 - no audio, 1 - CSND, 2 - DSP
    short       *fullBuffers;
    short       *leftBuffer;
    short       *rightBuffer;
    u64			startTick;
    u64         bufferPosition;
    u64         samplePosition;

    Handle      mixingThreadHandle;
    u8          mixingThreadStack[0x4000] __attribute__((aligned(8)));
    bool        terminateMixingThread;

    u64         startSamplePosition = 0;
    u64         upToSamplePosition = 0;

    CSND_ChnInfo*   channelInfo;

    ndspWaveBuf     waveBuf;        // structures for NDSP

} SSND3DS;


extern SSND3DS snd3DS;

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
    int maxLoopBuffer);

    
//---------------------------------------------------------
// Initialize the CSND library.
//---------------------------------------------------------
bool snd3dsInitialize();


//---------------------------------------------------------
// Finalize the CSND library.
//---------------------------------------------------------
void snd3dsFinalize();


//---------------------------------------------------------
// Mix the samples.
//
// This is usually called from within 3dssound.cpp.
// It should only be called externall from other 
// files when running in Citra.
//---------------------------------------------------------
void snd3dsMixSamples();


//---------------------------------------------------------
// Start playing the samples.
//---------------------------------------------------------
void snd3dsStartPlaying();


//---------------------------------------------------------
// Stop playing the samples.
//---------------------------------------------------------
void snd3dsStopPlaying();

#endif