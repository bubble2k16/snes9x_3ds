
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

int translateX = 0;
int translateY = 0;
int viewportX1 = 0;
int viewportY1 = 0;
int viewportX2 = 320;
int viewportY2 = 240;

int viewportStackCount = 0;
int viewportStack[20][4];

typedef struct
{
    int red[11][32];
    int green[11][32];
    int blue[11][32];
} SAlpha;

SAlpha alphas;

//---------------------------------------------------------------
// Initialize this library
//---------------------------------------------------------------
void ui3dsInitialize()
{
    // Precompute alphas
    //
    for (int i = 0; i < 32; i++)
    {
        for (int a = 0; a <= 10; a++)
        {
            int f = i * 10 / a;
            alphas.red[a][i] = f << 11;
            alphas.green[a][i] = f << 5;
            alphas.blue[a][i] = f;
        }
    }
}

//---------------------------------------------------------------
// Sets the global viewport for all drawing
//---------------------------------------------------------------
void ui3dsSetViewport(int x1, int y1, int x2, int y2)
{
    viewportX1 = x1;
    viewportX2 = x2;
    viewportY1 = y1;
    viewportY2 = y2;
}

//---------------------------------------------------------------
// Push the global viewport for all drawing
//---------------------------------------------------------------
void ui3dsPushViewport(int x1, int y1, int x2, int y2)
{
    if (viewportStackCount < 10)
    {
        viewportStack[viewportStackCount][0] = viewportX1;
        viewportStack[viewportStackCount][1] = viewportX2;
        viewportStack[viewportStackCount][2] = viewportY1;
        viewportStack[viewportStackCount][3] = viewportY2;
        viewportStackCount++;

        viewportX1 = x1;
        viewportX2 = x2;
        viewportY1 = y1;
        viewportY2 = y2;
    }
}

//---------------------------------------------------------------
// Pop the global viewport 
//---------------------------------------------------------------
void ui3dsPopViewport(int x1, int y1, int x2, int y2)
{
    if (viewportStackCount > 0)
    {
        viewportX1 = viewportStack[viewportStackCount][0];
        viewportX2 = viewportStack[viewportStackCount][1];
        viewportY1 = viewportStack[viewportStackCount][2];
        viewportY2 = viewportStack[viewportStackCount][3];
        viewportStackCount--;
    }
}


//---------------------------------------------------------------
// Applies alpha to a given colour.
// NOTE: Alpha is a value from 0 to 10. (0 = transparent, 10 = opaque)
//---------------------------------------------------------------
inline uint16 __attribute__((always_inline)) ui3dsApplyAlphaToColour(uint16 color, int alpha)
{
    int red = (color >> 11) & 0x1f;
    int blue = (color >> 6) & 0x1f; // drop the LSB of the blue colour
    int green = (color) & 0x1f;

    return alphas.red[alpha][red] + alphas.blue[alpha][blue] + alphas.green[alpha][green];
}


//---------------------------------------------------------------
// Sets the global translate for all drawing
//---------------------------------------------------------------
void ui3dsSetTranslate(int tx, int ty)
{
    translateX = tx;
    translateY = ty;
}


//---------------------------------------------------------------
// Computes the frame buffer offset given the x, y
// coordinates.
//---------------------------------------------------------------
inline int __attribute__((always_inline)) ui3dsComputeFrameBufferOffset(int x, int y)
{
    return ((x) * 240 + (239 - y));
}


//---------------------------------------------------------------
// Sets a pixel colour.
//---------------------------------------------------------------
inline void __attribute__((always_inline)) ui3dsSetPixelInline(uint16 *frameBuffer, int x, int y, int c)
{
    if (c < 0) return;
    if ((x) >= viewportX1 && (x) < viewportX2 && (y) >= viewportY1 && (y) < viewportY2) 
    { 
        frameBuffer[ui3dsComputeFrameBufferOffset((x), (y))] = c;  
    }
}


//---------------------------------------------------------------
// Draws a single character to the screen
//---------------------------------------------------------------
void ui3dsDrawChar(uint16 *frameBuffer, int x, int y, uint8 c)
{
    // Draws a character to the screen at (x,y) 
    // (0,0) is at the top left of the screen.
    //
    int wid = fontWidth[c];
    //printf ("d %c (%d)\n", c, bmofs);
    
    for (int x1 = 0; x1 < wid; x1++)
    {
        #define GETFONTBITMAP(c, x, y) fontBitmap[c * 256 + x + (y + 2)*16]

        #define SETPIXELFROMBITMAP(y1) \
            ui3dsSetPixelInline(frameBuffer, cx, cy + y1, \
            GETFONTBITMAP(c,x1,y1) == 0xff ? foreColor : \
            GETFONTBITMAP(c,x1,y1) == 0x00 ? -1 : \
            ((foreColor & ~0x821) >> 1) + ((backColor & ~0x821) >> 1)); \
    

        int cx = (x + translateX + x1);
        int cy = (y + translateY);
        if (cx >= viewportX1 && cx < viewportX2)
        {
            SETPIXELFROMBITMAP(0);
            SETPIXELFROMBITMAP(1);
            SETPIXELFROMBITMAP(2);
            SETPIXELFROMBITMAP(3);
            SETPIXELFROMBITMAP(4);
            SETPIXELFROMBITMAP(5);
            SETPIXELFROMBITMAP(6);
            SETPIXELFROMBITMAP(7);
            SETPIXELFROMBITMAP(8);
            SETPIXELFROMBITMAP(9);
            SETPIXELFROMBITMAP(10);
            SETPIXELFROMBITMAP(11);
        }
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
    
    //midColor = ((foreColor & ~0x821) >> 1) + ((backColor & ~0x821) >> 1);
}


//---------------------------------------------------------------
// Draws a rectangle with the colour (in RGB888 format).
// 
// Note: x0,y0 are inclusive. x1,y1 are exclusive.
//---------------------------------------------------------------
void ui3dsDrawRect(int x0, int y0, int x1, int y1, int color, int alpha)
{
    uint16* fb = (uint16 *) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

    x0 += translateX;
    x1 += translateX;
    y0 += translateY;
    y1 += translateY;

    if (x0 < viewportX1) x0 = viewportX1;
    if (x1 > viewportX2) x1 = viewportX2;
    if (y0 < viewportY1) y0 = viewportY1;
    if (y1 > viewportY2) y1 = viewportY2;
    
    if (alpha == 10)
    {
        for (int x = x0; x < x1; x++)
        {
            int fbofs = (x) * 240 + (239 - y0);
            for (int y = y0; y < y1; y++)
                fb[fbofs--] = color;
        }
    }
    else
    {
        if (alpha < 0) alpha = 0;
        if (alpha > 10) alpha = 10;

        for (int x = x0; x < x1; x++)
        {
            int fbofs = (x) * 240 + (239 - y0);
            for (int y = y0; y < y1; y++)
                fb[fbofs--] = 
                    ui3dsApplyAlphaToColour(color, alpha) +
                    ui3dsApplyAlphaToColour(fb[fbofs], 10 - alpha);
        }
    }
}



//---------------------------------------------------------------
// Draws a rectangle with the back colour 
// 
// Note: x0,y0 are inclusive. x1,y1 are exclusive.
//---------------------------------------------------------------
void ui3dsDrawRect(int x0, int y0, int x1, int y1)
{
    uint16* fb = (uint16 *) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

    x0 += translateX;
    x1 += translateX;
    y0 += translateY;
    y1 += translateY;

    if (x0 < viewportX1) x0 = viewportX1;
    if (x1 > viewportX2) x1 = viewportX2;
    if (y0 < viewportY1) y0 = viewportY1;
    if (y1 > viewportY2) y1 = viewportY2;
    
    for (int x = x0; x < x1; x++)
    {
        int fbofs = (x) * 240 + (239 - y0);
        for (int y = y0; y < y1; y++)
            fb[fbofs--] = backColor;
    }
}


//---------------------------------------------------------------
// Draws a string at the given position without translation.
//---------------------------------------------------------------
void ui3dsDrawString(uint16 *fb, int absoluteX, int absoluteY, char *buffer)
{
    int x = absoluteX;
    int y = absoluteY;
    int len = strlen(buffer);
    int totalWidth = 0;
    for (int i = 0; i < len; i++)
    {
        uint8 c = buffer[i];
        ui3dsDrawChar(fb, x + totalWidth, y, c);
        totalWidth += fontWidth[c];
    }
}


//---------------------------------------------------------------
// Draws a string with the fore + back colour.
//---------------------------------------------------------------
void ui3dsDrawString(int x0, int y0, int x1, int y1, bool centreAligned, char *buffer)
{
    x0 += translateX;
    x1 += translateX;
    y0 += translateY;
    y1 += translateY;
    
    ui3dsDrawRect(x0, y0, x1, y1);  // Draw the background color
   
    if (buffer != NULL)
    {
        int maxWidth = x1 - x0;
        int x = x0;

        if (centreAligned)
        {
            int sWidth = ui3dsGetStringWidth(buffer);
            if (sWidth < maxWidth)
                x = (maxWidth - sWidth) / 2 + x0;
        }

        uint16* fb = (uint16 *) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
        ui3dsDrawString(fb, x, y0, buffer);
    }

}


void ui3dsDrawString(int x0, int y, int x1, bool centreAligned, char *buffer)
{
    ui3dsDrawString(x0, y, x1, y + 12, centreAligned, buffer);
}