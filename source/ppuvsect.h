
#ifndef _PPUVSECT_H_
#define _PPUVSECT_H_

typedef struct 
{
    int16               StartY;
    int16               EndY;
    union
    {
        uint32              Value;
        struct
        {
            uint8           V1;
            uint8           V2;
            uint8           V3;
            uint8           V4;
        };
    };
} VerticalSection;

typedef struct 
{
    int                 StartY;
    uint32              CurrentValue;
    int                 Count;
    VerticalSection     Section[241];
} VerticalSections;



// Methods related to managing vertical sections for any general SNES registers.
//
void S9xResetVerticalSection(VerticalSections *verticalSections);
void S9xResetVerticalSection(VerticalSections *verticalSections, uint32 currentValue);
void S9xCommitVerticalSection(VerticalSections *verticalSections);
void S9xUpdateVerticalSectionValue(VerticalSections *verticalSections, uint32 newValue);


#endif