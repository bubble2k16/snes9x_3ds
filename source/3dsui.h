
#ifndef _3DSUI_H_
#define _3DSUI_H_

void ui3dsSetColor(int newForeColor, int newBackColor);
void ui3dsDrawString(int x0, int x1, int y, bool centreAligned, char *buffer);
void ui3dsDrawRect(int x0, int y0, int x1, int y1);


#endif