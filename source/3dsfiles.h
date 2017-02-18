
#ifndef _3DSFILES_H
#define _3DSFILES_H


//----------------------------------------------------------------------
// Initialize the library
//----------------------------------------------------------------------
void file3dsInitialize(void);


//----------------------------------------------------------------------
// Gets the current directory.
//----------------------------------------------------------------------
char *file3dsGetCurrentDir(void);


//----------------------------------------------------------------------
// Go up to the parent directory.
//----------------------------------------------------------------------
void file3dsGoToParentDirectory(void);


//----------------------------------------------------------------------
// Go up to the child directory.
//----------------------------------------------------------------------
void file3dsGoToChildDirectory(char *childDir);


//----------------------------------------------------------------------
// Load all ROM file names (up to 512 ROMs)
//
// Specify a comma separated list of extensions.
//
//----------------------------------------------------------------------
std::vector<std::string> file3dsGetFiles(char *extensions, int maxFiles);

#endif