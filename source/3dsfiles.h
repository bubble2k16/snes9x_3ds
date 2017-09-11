
#ifndef _3DSFILES_H
#define _3DSFILES_H

#include <string>
#include <vector>

enum class FileEntryType { ParentDirectory, ChildDirectory, File };

struct DirectoryEntry {
    std::string Filename;
    FileEntryType Type;

    DirectoryEntry(const std::string& filename, FileEntryType type)
    : Filename(filename), Type(type) { }
};

//----------------------------------------------------------------------
// Initialize the library
//----------------------------------------------------------------------
void file3dsInitialize(void);


//----------------------------------------------------------------------
// Gets the current directory.
//----------------------------------------------------------------------
char *file3dsGetCurrentDir(void);


//----------------------------------------------------------------------
// Go up or down a level.
//----------------------------------------------------------------------
void file3dsGoUpOrDownDirectory(const DirectoryEntry& entry);


//----------------------------------------------------------------------
// Go up to the parent directory.
//----------------------------------------------------------------------
void file3dsGoToParentDirectory(void);


//----------------------------------------------------------------------
// Go up to the child directory.
//----------------------------------------------------------------------
void file3dsGoToChildDirectory(const char* childDir);


//----------------------------------------------------------------------
// Fetch all file names with any of the given extensions
//----------------------------------------------------------------------
void file3dsGetFiles(std::vector<DirectoryEntry>& files, const std::vector<std::string>& extensions);

#endif