#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <tuple>
#include <vector>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <dirent.h>

#include "port.h"
#include "3dsfiles.h"

inline std::string operator "" s(const char* s, unsigned int length) {
    return std::string(s, length);
}

static char currentDir[_MAX_PATH] = "";

//----------------------------------------------------------------------
// Initialize the library
//----------------------------------------------------------------------
void file3dsInitialize(void)
{
    getcwd(currentDir, _MAX_PATH);
#ifdef RELEASE
    if (currentDir[0] == '/')
    {
        char tempDir[_MAX_PATH];

        sprintf(tempDir, "sdmc:%s", currentDir);
        strcpy(currentDir, tempDir);
    }
#endif
}


//----------------------------------------------------------------------
// Gets the current directory.
//----------------------------------------------------------------------
char *file3dsGetCurrentDir(void)
{
    return currentDir;
}


//----------------------------------------------------------------------
// Go up or down a level.
//----------------------------------------------------------------------
void file3dsGoUpOrDownDirectory(const DirectoryEntry& entry) {
    if (entry.Type == FileEntryType::ParentDirectory) {
        file3dsGoToParentDirectory();
    } else if (entry.Type == FileEntryType::ChildDirectory) {
        file3dsGoToChildDirectory(entry.Filename.c_str());
    }
}

//----------------------------------------------------------------------
// Count the directory depth. 1 = root folder
//----------------------------------------------------------------------
int file3dsCountDirectoryDepth(char *dir)
{
    int depth = 0;
    for (int i = 0; i < strlen(dir); i++)
        if (dir[i] == '/')
            depth++;
    return depth;
}

//----------------------------------------------------------------------
// Go up to the parent directory.
//----------------------------------------------------------------------
void file3dsGoToParentDirectory(void)
{
    int len = strlen(currentDir);

    if (len > 1)
    {
        for (int i = len - 2; i>=0; i--)
        {
            if (currentDir[i] == '/')
            {
                currentDir[i + 1] = 0;
                break;
            }
        }
    }
}


//----------------------------------------------------------------------
// Go up to the child directory.
//----------------------------------------------------------------------
void file3dsGoToChildDirectory(const char* childDir)
{
    strncat(currentDir, &childDir[2], _MAX_PATH);
    strncat(currentDir, "/", _MAX_PATH);
}


//----------------------------------------------------------------------
// Gets the extension of a given file.
//----------------------------------------------------------------------
char *file3dsGetExtension(char *filePath)
{
    int len = strlen(filePath);
    char *extension = &filePath[len];

    for (int i = len - 1; i >= 0; i--)
    {
        if (filePath[i] == '.')
        {
            extension = &filePath[i + 1];
            break;
        }
    }
    return extension;
}


//----------------------------------------------------------------------
// Case-insensitive string equality check.
//----------------------------------------------------------------------
bool CaseInsensitiveEquals(const std::string& s0, const std::string& s1) {
    return s0.size() == s1.size() ? std::equal(s0.begin(), s0.end(), s1.begin(), [](unsigned char c0, unsigned char c1) { return std::tolower(c0) == std::tolower(c1); }) : false;
}

//----------------------------------------------------------------------
// Fetch all file names with any of the given extensions
//----------------------------------------------------------------------
void file3dsGetFiles(std::vector<DirectoryEntry>& files, const std::vector<std::string>& extensions)
{
    files.clear();

    if (currentDir[0] == '/')
    {
        char tempDir[_MAX_PATH];
        sprintf(tempDir, "sdmc:%s", currentDir);
        strcpy(currentDir, tempDir);
    }

    struct dirent* dir;
    DIR* d = opendir(currentDir);

    if (file3dsCountDirectoryDepth(currentDir) > 1)
    {
        // Insert the parent directory.
        files.emplace_back(".. (Up to Parent Directory)"s, FileEntryType::ParentDirectory);
    }

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_name[0] == '.')
                continue;
            if (dir->d_type == DT_DIR)
            {
                files.emplace_back(std::string("\x01 ") + std::string(dir->d_name), FileEntryType::ChildDirectory);
            }
            if (dir->d_type == DT_REG)
            {
                std::string ext = file3dsGetExtension(dir->d_name);
                if (std::any_of(extensions.begin(), extensions.end(), [&ext](const std::string& e) { return CaseInsensitiveEquals(e, ext); }))
                {
                    files.emplace_back(std::string(dir->d_name), FileEntryType::File);
                }
            }
        }
        closedir(d);
    }

    std::sort( files.begin(), files.end(), [](const DirectoryEntry& a, const DirectoryEntry& b) {
        return std::tie(a.Type, a.Filename) < std::tie(b.Type, b.Filename);
    } );
}