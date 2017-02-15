//===================================================================
// 3DS Emulator
//===================================================================

#include <stdio.h>

#ifndef _3DSCONFIG_H_
#define _3DSCONFIG_H_

bool    config3dsOpenFile(const char *filename, bool fileWriteMode);
void    config3dsCloseFile();
void    config3dsReadWriteInt32(char *format, int *value, int minValue = 0, int maxValue = 0xffff);
void    config3dsReadWriteString(char *writeFormat, char *readFormat, char *value);

#endif

