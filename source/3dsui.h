
#ifndef _3DSUI_H_
#define _3DSUI_H_

void ui3dsInitialize();
void ui3dsSetFont(int fontIndex);

void ui3dsSetViewport(int x1, int y1, int x2, int y2);
void ui3dsPushViewport(int x1, int y1, int x2, int y2);
void ui3dsPopViewport();
void ui3dsSetTranslate(int tx, int ty);

void ui3dsSetColor(int newForeColor, int newBackColor);
int ui3dsApplyAlphaToColor(int color, float alpha);

void ui3dsDrawRect(int x0, int y0, int x1, int y1);
void ui3dsDrawRect(int x0, int y0, int x1, int y1, int color, float alpha = 1.0f);

void ui3dsDrawStringWithWrapping(int x0, int y0, int x1, int y1, int color, int horizontalAlignment, const char *buffer);
void ui3dsDrawStringWithNoWrapping(int x0, int y0, int x1, int y1, int color, int horizontalAlignment, const char *buffer);

void ui3dsCopyFromFrameBuffer(uint16 *destBuffer);
void ui3dsBlitToFrameBuffer(uint16 *srcBuffer, float alpha = 1.0f);


#define HALIGN_LEFT     -1
#define HALIGN_CENTER   0
#define HALIGN_RIGHT    1


#endif