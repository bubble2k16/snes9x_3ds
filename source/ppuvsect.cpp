

#include "ppu.h"

//------------------------------------------------------------------
// Use this to manage vertical sections for brightness, and
// other stuff that change in-frame so as to minimize
// FLUSH_REDRAW
//------------------------------------------------------------------

// Reset vertical sections with a specific value.
// Call this: 
// 1. Inside S9xReset / S9xSoftReset  
//
void S9xResetVerticalSection(VerticalSections *verticalSections, uint32 currentValue)
{
    verticalSections->CurrentValue = currentValue;
    verticalSections->StartY = IPPU.CurrentLine;
    verticalSections->Count = 0;
}


// Reset all vertical sections with no change in the register value.
// Call this: 
// 1. After rendering to screen inside S9xUpdateScreenHardware 
// 2. Inside S9xStartScreenRefresh.
//
void S9xResetVerticalSection(VerticalSections *verticalSections)
{
    verticalSections->StartY = IPPU.CurrentLine;
    verticalSections->Count = 0;
}


// Commits the final value to the list.
// 
// Call this before rendering the screen inside S9xUpdateScreenHardware.
//
void S9xCommitVerticalSection(VerticalSections *verticalSections)
{
	if (IPPU.CurrentLine != verticalSections->StartY)
	{
		verticalSections->Section[verticalSections->Count].StartY = verticalSections->StartY;
		verticalSections->Section[verticalSections->Count].EndY = IPPU.CurrentLine - 1;
		verticalSections->Section[verticalSections->Count].Value = verticalSections->CurrentValue;
		verticalSections->Count ++;
		verticalSections->StartY = IPPU.CurrentLine;
	}
}

// Sets a new value to this section, and commits it
// if the current scanline is different from the last.
//
// This should be called when the corresponding SNES register
// value is updated. 
//
void S9xUpdateVerticalSectionValue(VerticalSections *verticalSections, uint32 newValue)
{
	if (IPPU.RenderThisFrame && 
		IPPU.CurrentLine != verticalSections->StartY && verticalSections->CurrentValue != newValue)
	{
		verticalSections->Section[verticalSections->Count].StartY = verticalSections->StartY;
		verticalSections->Section[verticalSections->Count].EndY = IPPU.CurrentLine - 1;
		verticalSections->Section[verticalSections->Count].Value = verticalSections->CurrentValue;
		verticalSections->Count ++;
		verticalSections->StartY = IPPU.CurrentLine;
	}
	verticalSections->CurrentValue = newValue;
}

