#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <dirent.h>

#include "port.h"
#include "3dsfiles.h"


static char currentDir[_MAX_PATH] = "";

//----------------------------------------------------------------------
// Initialize the library
//----------------------------------------------------------------------
void file3dsInitialize(void)
{
    getcwd(currentDir, _MAX_PATH);
}


//----------------------------------------------------------------------
// Gets the current directory.
//----------------------------------------------------------------------
char *file3dsGetCurrentDir(void)
{
    return currentDir;
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
void file3dsGoToChildDirectory(char *childDir)
{
    strncat(currentDir, childDir, _MAX_PATH);
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
// Case-insensitive check for substring.
//----------------------------------------------------------------------
char* stristr( char* str1, const char* str2 )
{
    char* p1 = str1 ;
    const char* p2 = str2 ;
    char* r = *p2 == 0 ? str1 : 0 ;

    while( *p1 != 0 && *p2 != 0 )
    {
        if( tolower( *p1 ) == tolower( *p2 ) )
        {
            if( r == 0 )
            {
                r = p1 ;
            }

            p2++ ;
        }
        else
        {
            p2 = str2 ;
            if( tolower( *p1 ) == tolower( *p2 ) )
            {
                r = p1 ;
                p2++ ;
            }
            else
            {
                r = 0 ;
            }
        }

        p1++ ;
    }

    return *p2 == 0 ? r : 0 ;
}

//----------------------------------------------------------------------
// Load all ROM file names (up to 512 ROMs)
//
// Specify a comma separated list of extensions.
//
//----------------------------------------------------------------------
std::vector<std::string> file3dsGetFiles(char *extensions, int maxFiles)
{
    std::vector<std::string> files;
    char buffer[_MAX_PATH];

    struct dirent* dir;
    DIR* d = opendir(currentDir);

    if (strlen(currentDir) > 1)
    {
        // Insert the parent directory.
        snprintf(buffer, _MAX_PATH, "\x01 ..");   
        files.push_back(buffer);
    }

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            char *dot = strrchr(dir->d_name, '.');

            if (dir->d_name[0] == '.')
                continue;
            if (dir->d_type == DT_DIR)
            {
                snprintf(buffer, _MAX_PATH, "\x01 %s", dir->d_name);
                files.push_back(buffer);
            }
            if (dir->d_type == DT_REG)
            {
                char *ext = file3dsGetExtension(dir->d_name);

                if (!stristr(extensions, ext))
                    continue;

                files.push_back(dir->d_name);
            }
        }
        closedir(d);
    }

    std::sort(files.begin(), files.end());

    return files;
}