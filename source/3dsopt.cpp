



#define _3DSOPT_CPP_
#include "3dsopt.h"
#include "3dssnes9x.h"

#define TICKS_PER_SEC (268123)

//--------------------------------------------------------------------------------
// Initialize optimizations
//--------------------------------------------------------------------------------



char *emptyString = "";

// Optimization variables
//


//struct ST3DSOpt t3dsOpt;

void t3dsResetTimings()
{
#ifndef RELEASE
	for (int i = 0; i < 100; i++)
    {
        t3dsTotalTicks[i] = 0; 
        t3dsTotalCount[i] = 0;
        t3dsClockName[i] = emptyString;
    }
#endif
}




void t3dsCount(int bucket, char *clockName)
{
#ifndef RELEASE
    t3dsStartTicks[bucket] = -1; 
    t3dsClockName[bucket] = clockName;
    t3dsTotalCount[bucket]++;
#endif
}



void t3dsShowTotalTiming(int bucket)
{
#ifndef RELEASE
    if (t3dsTotalTicks[bucket] > 0)
        printf ("%-20s: %2d %4d ms %d\n", t3dsClockName[bucket], bucket,
        (int)(t3dsTotalTicks[bucket] / TICKS_PER_SEC), 
        t3dsTotalCount[bucket]);
    else if (t3dsStartTicks[bucket] == -1 && t3dsTotalCount[bucket] > 0)
        printf ("%-20s: %2d %d\n", t3dsClockName[bucket], bucket,
        t3dsTotalCount[bucket]);
#endif
}
