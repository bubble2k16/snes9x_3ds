//===================================================================
// 3DS Emulator
//===================================================================

#include <stdio.h>

#ifndef _3DSCONFIG_H_
#define _3DSCONFIG_H_


//----------------------------------------------------------------------
// Opens a config .cfg file.
//----------------------------------------------------------------------
bool    config3dsOpenFile(const char *filename, bool fileWriteMode);


//----------------------------------------------------------------------
// Closes the config file.
//----------------------------------------------------------------------
void    config3dsCloseFile();


//----------------------------------------------------------------------
// Load / Save an int32 value specific to game.
//----------------------------------------------------------------------
void    config3dsReadWriteInt32(const char *format, int *value, int minValue = 0, int maxValue = 0xffff);


//----------------------------------------------------------------------
// Load / Save a string specific to game.
//----------------------------------------------------------------------
void    config3dsReadWriteString(const char *writeFormat, char *readFormat, char *value);

#endif

