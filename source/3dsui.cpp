//=============================================================================
// Basic user interface framework for low-level drawing operations to
// the bottom screen.
//=============================================================================

#include <cstdio>
#include <cstring>

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include "snes9x.h"

#include "3dsui.h"
#include "3dsfont.cpp"

int foreColor = 0xffffff;
int backColor = 0x000000;

int translateX = 0;
int translateY = 0;
int viewportX1 = 0;
int viewportY1 = 0;
int viewportX2 = 320;
int viewportY2 = 240;

int fontHeight = 13;

int viewportStackCount = 0;
int viewportStack[20][4];

#define MAX_ALPHA 8
typedef struct
{
    int red[MAX_ALPHA + 1][32];
    int green[MAX_ALPHA + 1][32];
    int blue[MAX_ALPHA + 1][32];
} SAlpha;

SAlpha alphas;

uint8 *fontWidthArray[] = { fontTempestaWidth, fontRondaWidth, fontArialWidth };
uint8 *fontBitmapArray[] = { fontTempestaBitmap, fontRondaBitmap, fontArialBitmap };

uint8 *fontBitmap;
uint8 *fontWidth;

//---------------------------------------------------------------
// Initialize this library
//---------------------------------------------------------------
void ui3dsInitialize()
{
    // Precompute alphas
    //
    for (int i = 0; i < 32; i++)
    {
        for (int a = 0; a <= MAX_ALPHA; a++)
        {
            int f = i * a / MAX_ALPHA;
            alphas.red[a][i] = f << 11;
            alphas.green[a][i] = f << 6;
            alphas.blue[a][i] = f;
        }
    }

    // Initialize fonts
    //
    for (int f = 0; f < 3; f++)
    {
        fontBitmap = fontBitmapArray[f];
        fontWidth = fontWidthArray[f];
        for (int i = 0; i < 65536; i++)
        {
            uint8 c = fontBitmap[i];
            if (c == ' ')
                fontBitmap[i] = 0;
            else
                fontBitmap[i] = c - '0';
        }

        fontWidth[1] = 10;
        fontWidth[10] = 0;
        fontWidth[13] = 0;
        fontWidth[248] = 10;
        fontWidth[249] = 10;
        fontWidth[250] = 9;
        fontWidth[251] = 1;
        fontWidth[253] = 10;
        fontWidth[254] = 10;
        fontWidth[255] = 7;
    }

    fontBitmap = fontTempestaBitmap;
    fontWidth = fontTempestaWidth;

    viewportX1 = 0;
    viewportY1 = 0;
    viewportX2 = 320;
    viewportY2 = 240;    
}


//---------------------------------------------------------------
// Sets the font
//---------------------------------------------------------------
void ui3dsSetFont(int fontIndex)
{
    if (fontIndex >= 0 && fontIndex < 3)
    {
        fontBitmap = fontBitmapArray[fontIndex];
        fontWidth = fontWidthArray[fontIndex];
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

    if (viewportX1 < 0) viewportX1 = 0;
    if (viewportX2 > 320) viewportX2 = 320;
    if (viewportY1 < 0) viewportY1 = 0;
    if (viewportY2 > 240) viewportY2 = 240;
}

//---------------------------------------------------------------
// Push the global viewport for all drawing
//---------------------------------------------------------------
void ui3dsPushViewport(int x1, int y1, int x2, int y2)
{
    if (viewportStackCount < 10)
    {
        if (x1 < viewportX1) x1 = viewportX1;
        if (x2 > viewportX2) x2 = viewportX2;
        if (y1 < viewportY1) y1 = viewportY1;
        if (y2 > viewportY2) y2 = viewportY2;

        viewportStack[viewportStackCount][0] = viewportX1;
        viewportStack[viewportStackCount][1] = viewportX2;
        viewportStack[viewportStackCount][2] = viewportY1;
        viewportStack[viewportStackCount][3] = viewportY2;
        viewportStackCount++;

        ui3dsSetViewport(x1, y1, x2, y2);
    }
}

//---------------------------------------------------------------
// Pop the global viewport 
//---------------------------------------------------------------
void ui3dsPopViewport()
{
    if (viewportStackCount > 0)
    {
        viewportStackCount--;
        viewportX1 = viewportStack[viewportStackCount][0];
        viewportX2 = viewportStack[viewportStackCount][1];
        viewportY1 = viewportStack[viewportStackCount][2];
        viewportY2 = viewportStack[viewportStackCount][3];
    }
}


//---------------------------------------------------------------
// Applies alpha to a given colour.
// NOTE: Alpha is a value from 0 to 10. (0 = transparent, 10 = opaque)
//---------------------------------------------------------------
inline uint16 __attribute__((always_inline)) ui3dsApplyAlphaToColour565(int color565, int alpha)
{
    int red = (color565 >> 11) & 0x1f;
    int green = (color565 >> 6) & 0x1f; // drop the LSB of the green colour
    int blue = (color565) & 0x1f;

    int result = alphas.red[alpha][red] | alphas.blue[alpha][blue] | alphas.green[alpha][green];
    //printf ("result %x * %d = (%x | %x | %x) = %x\n", color565, alpha, 
    //   alphas.red[alpha][red], alphas.green[alpha][green], alphas.blue[alpha][blue], result);
    return result;
}


int ui3dsApplyAlphaToColor(int color, float alpha)
{
    if (alpha < 0)      alpha = 0;
    if (alpha > 1.0f)   alpha = 1.0f;

    int a = (int)(alpha * 255);

    return 
        ((((color >> 16) & 0xff) * a / 255) << 16) |
        ((((color >> 8) & 0xff) * a / 255) << 8) |
        ((((color >> 0) & 0xff) * a / 255) << 0);
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
// Gets a pixel colour.
//---------------------------------------------------------------
inline uint16 __attribute__((always_inline)) ui3dsGetPixelInline(uint16 *frameBuffer, int x, int y)
{
    return frameBuffer[ui3dsComputeFrameBufferOffset((x), (y))];  
}


//---------------------------------------------------------------
// Sets a pixel colour.
//---------------------------------------------------------------
inline void __attribute__((always_inline)) ui3dsSetPixelInline(uint16 *frameBuffer, int x, int y, int color)
{
    if (color < 0) return;
    if ((x) >= viewportX1 && (x) < viewportX2 && 
        (y) >= viewportY1 && (y) < viewportY2) 
    { 
        frameBuffer[ui3dsComputeFrameBufferOffset((x), (y))] = color;  
    }
}


//---------------------------------------------------------------
// Draws a single character to the screen
//---------------------------------------------------------------
void ui3dsDrawChar(uint16 *frameBuffer, int x, int y, int color565, uint8 c)
{
    // Draws a character to the screen at (x,y) 
    // (0,0) is at the top left of the screen.
    //
    int wid = fontWidth[c];
    uint8 alpha;
    //printf ("d %c (%d)\n", c, bmofs);

    if ((y) >= viewportY1 && (y) < viewportY2)
    {
        for (int x1 = 0; x1 < wid; x1++)
        {
            #define GETFONTBITMAP(c, x, y) fontBitmap[c * 256 + x + (y)*16]

            #define SETPIXELFROMBITMAP(y1) \
                alpha = GETFONTBITMAP(c,x1,y1); \
                ui3dsSetPixelInline(frameBuffer, cx, cy + y1, \
                alpha == MAX_ALPHA ? color565 : \
                alpha == 0x0 ? -1 : \
                    ui3dsApplyAlphaToColour565(color565, alpha) + \
                    ui3dsApplyAlphaToColour565(ui3dsGetPixelInline(frameBuffer, cx, cy + y1), MAX_ALPHA - alpha));

            int cx = (x + x1);
            int cy = (y);
            if (cx >= viewportX1 && cx < viewportX2)
            {
                for (int h = 0; h < fontHeight; h++)
                {
                    SETPIXELFROMBITMAP(h);
                }
            }
        }
    }
}


//---------------------------------------------------------------
// Computes width of the string
//---------------------------------------------------------------
int ui3dsGetStringWidth(const char *s, int startPos = 0, int endPos = 0xffff)
{
    int totalWidth = 0;
    for (int i = startPos; i <= endPos; i++)
    {
        uint8 c = s[i];
        if (c == 0)
            break;
        totalWidth += fontWidth[c];
    }   
    return totalWidth;
}

#define CONVERT_TO_565(x)    (((x & 0xf8) >> 3) | (((x >> 8) & 0xf8) << 3) | (((x >> 16) & 0xf8) << 8))

//---------------------------------------------------------------
// Colors are in the 888 (RGB) format.
//---------------------------------------------------------------
void ui3dsSetColor(int newForeColor, int newBackColor)
{
    foreColor = newForeColor;
    backColor = newBackColor;
}


//---------------------------------------------------------------
// Draws a rectangle with the colour (in RGB888 format).
// 
// Note: x0,y0 are inclusive. x1,y1 are exclusive.
//---------------------------------------------------------------
void ui3dsDrawRect(int x0, int y0, int x1, int y1, int color, float alpha)
{
    if (color < 0)
        return;

    color = CONVERT_TO_565(color);

    uint16* fb = (uint16 *) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

    x0 += translateX;
    x1 += translateX;
    y0 += translateY;
    y1 += translateY;

    if (x0 < viewportX1) x0 = viewportX1;
    if (x1 > viewportX2) x1 = viewportX2;
    if (y0 < viewportY1) y0 = viewportY1;
    if (y1 > viewportY2) y1 = viewportY2;
    
    if (alpha < 0) alpha = 0;
    if (alpha > 1.0f) alpha = 1.0f;
    
    if (alpha == 1.0f)
    {
        for (int x = x0; x < x1; x++)
        {
            int fbofs = (x) * 240 + (239 - y0);
            for (int y = y0; y < y1; y++)
                fb[fbofs--] = color;
        }
    }
    else if (alpha == 0.0)
    {
        return;
    }
    else
    {
        int iAlpha = alpha * MAX_ALPHA;
        for (int x = x0; x < x1; x++)
        {
            int fbofs = (x) * 240 + (239 - y0);
            for (int y = y0; y < y1; y++)
            {
                fb[fbofs] = 
                    ui3dsApplyAlphaToColour565(color, iAlpha) +
                    ui3dsApplyAlphaToColour565(fb[fbofs], MAX_ALPHA - iAlpha);
                fbofs --;
            }
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
    ui3dsDrawRect(x0, y0, x1, y1, backColor, 1.0f);
}


//---------------------------------------------------------------
// Draws a string at the given position without translation.
//---------------------------------------------------------------
void ui3dsDrawStringOnly(uint16 *fb, int absoluteX, int absoluteY, int color, const char *buffer, int startPos = 0, int endPos = 0xffff)
{
    int x = absoluteX;
    int y = absoluteY;

    if (color < 0)
        return;
    if (y >= viewportY1 - 16 && y <= viewportY2)
    {
        color = CONVERT_TO_565(color);
        for (int i = startPos; i <= endPos; i++)
        {
            uint8 c = buffer[i];
            if (c == 0)
                break;
            if (c != 32)
                ui3dsDrawChar(fb, x, y, color, c);
            x += fontWidth[c];
        }
    }
}


//---------------------------------------------------------------
// Draws a string with the forecolor, with wrapping
//---------------------------------------------------------------
void ui3dsDrawStringWithWrapping(int x0, int y0, int x1, int y1, int color, int horizontalAlignment, const char *buffer)
{
    int strLineCount = 0;
    int strLineStart[30];
    int strLineEnd[30];

    x0 += translateX;
    x1 += translateX;
    y0 += translateY;
    y1 += translateY;
    
    ui3dsPushViewport(x0, y0, x1, y1);
    //ui3dsDrawRect(x0, y0, x1, y1, backColor);  // Draw the background color
   
    if (buffer != NULL)
    {
        int maxWidth = x1 - x0;
        int slen = strlen(buffer);

        int curStartPos = 0;
        int curEndPos = slen - 1;
        int lineWidth = 0;
        for (int i = 0; i < slen; )
        {
            if (i != curStartPos)
            {
                if (buffer[i] == ' ' && i > 0 && buffer[i-1] != ' ')
                    curEndPos = i - 1;
                else if (buffer[i] == '-')  // use space or dash as line breaks
                    curEndPos = i;
                else if (buffer[i] == '\n')  // \n as line breaks.
                {
                    curEndPos = i - 1;
                    lineWidth = 999999;     // force the line break.
                }
            }
            lineWidth += fontWidth[buffer[i]];
            if (lineWidth > maxWidth)
            {
                // Break the line here
                strLineStart[strLineCount] = curStartPos;
                strLineEnd[strLineCount] = curEndPos;
                strLineCount++;

                if (strLineCount >= 30) break; 

                if (lineWidth != 999999)
                {
                    i = curEndPos + 1;
                    while (buffer[i] == ' ')
                        i++;
                }
                else
                {
                    i = curEndPos + 2;
                }
                curStartPos = i;
                curEndPos = slen - 1;
                lineWidth = 0;
            }
            else
                i++;
        }

        // Output the last line.
        curEndPos = slen - 1;
        if (curStartPos <= curEndPos)
        {
            strLineStart[strLineCount] = curStartPos;
            strLineEnd[strLineCount] = curEndPos;
            strLineCount++;
        }

        uint16* fb = (uint16 *) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
        for (int i = 0; i < strLineCount; i++)
        {
            int x = x0;
            if (horizontalAlignment >= 0)
            {
                int sWidth = ui3dsGetStringWidth(buffer, strLineStart[i], strLineEnd[i]);

                if (horizontalAlignment == 0)   // center aligned
                    x = (maxWidth - sWidth) / 2 + x0;
                else                            // right aligned
                    x = maxWidth - sWidth + x0;
            }

            ui3dsDrawStringOnly(fb, x, y0, color, buffer, strLineStart[i], strLineEnd[i]);
            y0 += 12;
        }
    }

    ui3dsPopViewport();
}


//---------------------------------------------------------------
// Draws a string with the forecolor, with no wrapping
//---------------------------------------------------------------
void ui3dsDrawStringWithNoWrapping(int x0, int y0, int x1, int y1, int color, int horizontalAlignment, const char *buffer)
{
    x0 += translateX;
    x1 += translateX;
    y0 += translateY;
    y1 += translateY;
    
    ui3dsPushViewport(x0, y0, x1, y1);
    //ui3dsDrawRect(x0, y0, x1, y1, backColor);  // Draw the background color
   
    if (buffer != NULL)
    {
        uint16* fb = (uint16 *) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
        int maxWidth = x1 - x0;
        int x = x0;
        if (horizontalAlignment >= HALIGN_CENTER)
        {
            int sWidth = ui3dsGetStringWidth(buffer);

            if (horizontalAlignment == HALIGN_CENTER)   // center aligned
                x = (maxWidth - sWidth) / 2 + x0;
            else                                        // right aligned
                x = maxWidth - sWidth + x0;
        }
        ui3dsDrawStringOnly(fb, x, y0, color, buffer);
    }

    ui3dsPopViewport();
}


//---------------------------------------------------------------
// Copies pixel data from the frame buffer to another buffer
//---------------------------------------------------------------
void ui3dsCopyFromFrameBuffer(uint16 *destBuffer)
{
    uint16* fb = (uint16 *) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    memcpy(destBuffer, fb, 320*240*2);
}


//---------------------------------------------------------------
// Copies pixel data from the buffer to the framebuffer
//---------------------------------------------------------------
void ui3dsBlitToFrameBuffer(uint16 *srcBuffer, float alpha)
{
    uint16* fb = (uint16 *) gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    
    int a = (int)(alpha * MAX_ALPHA);
    for (int x = viewportX1; x < viewportX2; x++)
        for (int y = viewportY1; y < viewportY2; y++)
        {
            int ofs = ui3dsComputeFrameBufferOffset(x,y);
            int color = ui3dsApplyAlphaToColour565(srcBuffer[ofs], a);
            fb[ofs] = color;
        }
}

