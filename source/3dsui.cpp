
#include <cstdio>
#include <cstring>

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include "snes9x.h"

#include "3dsfont.h"

int foreColor = 0xffff;
int backColor = 0x0000;
int midColor = 0x7bef;

void ui3dsDrawChar(uint16 *frameBuffer, int x, int y, uint8 c, int starCharX, int endCharX)
{
    // Draws a character to the screen at (x,y) 
    // (0,0) is at the top left of the screen.
    //
    int fbofs = (x) * 240 + (239 - y);
    int bmofs = (int)c * 256;
    int wid = fontWidth[c];
    //printf ("d %c (%d)\n", c, bmofs);
    
    for (int cx = starCharX; cx < wid && cx < endCharX; cx++)
    {
        #define WRITE_BITMAP(h) \
            frameBuffer[fbofs - h] = \ 
                (fontBitmap[bmofs + (h + 2)*16] == 0xff ? foreColor : \ 
                fontBitmap[bmofs + (h + 2)*16] == 0x00 ? backColor : midColor); 
            //if (c >= 91 && c <= 93) if (fontBitmap[bmofs + (h + 2)*16] != 0) printf ("X"); else printf(" ");

        WRITE_BITMAP(0);
        WRITE_BITMAP(1);
        WRITE_BITMAP(2);
        WRITE_BITMAP(3);
        WRITE_BITMAP(4);
        WRITE_BITMAP(5);
        WRITE_BITMAP(6);
        WRITE_BITMAP(7);
        WRITE_BITMAP(8);
        WRITE_BITMAP(9);
        WRITE_BITMAP(10);
        WRITE_BITMAP(11);

        fbofs += 240;
        bmofs += 1;
        //if (c >= 91 && c <= 93) printf ("\n");
    }
}


//---------------------------------------------------------------
// Computes width of the string
//---------------------------------------------------------------
int ui3dsGetStringWidth(char *s)
{
    int totalWidth = 0;
    int len = strlen(s);
    for (int i = 0; i < len; i++)
    {
        uint8 c = s[i];
        totalWidth += fontWidth[c];
    }   
    return totalWidth;
}


//---------------------------------------------------------------
// Colors are in the 888 (RGB) format.
//---------------------------------------------------------------
void ui3dsSetColor(int newForeColor, int newBackColor)
{
    #define CONVERT_TO_565(x)    (((x & 0xf8) >> 3) | (((x >> 8) & 0xf8) << 3) | (((x >> 16) & 0xf8) << 8))

    foreColor = CONVERT_TO_565(newForeColor);
    backColor = CONVERT_TO_565(newBackColor);
    
    midColor = ((foreColor & ~0x821) >> 1) + ((backColor & ~0x821) >> 1);
}

//---------------------------------------------------------------
// Draws a rectangle with the back colour.
// Note: x0,y0 are inclusive. x1,y1 are exclusive.
//---------------------------------------------------------------
void ui3dsDrawRect(int x0, int y0, int x1, int y1)
{
    uint16* fb = (uint16 *) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

    for (int x = x0; x < x1; x++)
    {
        int fbofs = (x) * 240 + (239 - y0);
        for (int y = y0; y < y1; y++)
        {
            fb[fbofs--] = backColor;
        }
    }
}


//---------------------------------------------------------------
// Draws a string with the fore + back colour.
//---------------------------------------------------------------
void ui3dsDrawString(int x0, int y, int x1, bool centreAligned, char *buffer)
{
    if (buffer != NULL)
    {
        int maxWidth = x1 - x0;
        int x = x0;

        if (centreAligned)
        {
            int sWidth = ui3dsGetStringWidth(buffer);
            if (sWidth < maxWidth)
            {
                x = (maxWidth - sWidth) / 2 + x0;
            }
        }

        uint16* fb = (uint16 *) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

        int len = strlen(buffer);
        int totalWidth = 0;
        int clip = 16;
        for (int i = 0; i < len; i++)
        {
            uint8 c = buffer[i];
            if (fontWidth[c] + totalWidth > maxWidth)
                clip = fontWidth[c] + totalWidth - maxWidth;

            ui3dsDrawChar(fb, x + totalWidth, y, c, 0, clip);

            totalWidth += fontWidth[c];
            if (clip < 16)
                break;
        }

        if (clip == 16)
        {
            ui3dsDrawRect(x + totalWidth, y, x1, y + 12);
        }
        if (x != x0)
        {
            ui3dsDrawRect(x0, y, x, y + 12);
        }
    }
    else
    {
        ui3dsDrawRect(x0, y, x1, y + 12);
    }
}
