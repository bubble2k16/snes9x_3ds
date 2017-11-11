
#include "3dsconfig.h"

static FILE    *fp = NULL;
static bool    WriteMode = false;    

//----------------------------------------------------------------------
// Opens a config .cfg file.
//----------------------------------------------------------------------
bool config3dsOpenFile(const char *filename, bool fileWriteMode)
{
    if (!fp)
    {
        WriteMode = fileWriteMode;
        if (fileWriteMode)
            fp = fopen(filename, "w+");
        else
            fp = fopen(filename, "r");
        if (fp)
            return true;
        else
            return false;
    }
}


//----------------------------------------------------------------------
// Closes the config file.
//----------------------------------------------------------------------
void config3dsCloseFile()
{
    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }
}


//----------------------------------------------------------------------
// Load / Save an int32 value specific to game.
//----------------------------------------------------------------------
void config3dsReadWriteInt32(const char *format, int *value, int minValue, int maxValue)
{
    if (!fp)
        return;
    //if (strlen(format) == 0)
    //    return;

    if (WriteMode)
    {
        if (value != NULL)
        {
            //printf ("Writing %s %d\n", format, *value);
        	fprintf(fp, format, *value);
        }
        else
        {
            //printf ("Writing %s\n", format);
        	fprintf(fp, format);

        }
    }
    else
    {
        if (value != NULL)
        {
            fscanf(fp, format, value);
            if (*value < minValue)
                *value = minValue;
            if (*value > maxValue)
                *value = maxValue;
        }
        else
        {
            fscanf(fp, format);
            //printf ("skipped line\n");
        }
    }
}


//----------------------------------------------------------------------
// Load / Save a string specific to game.
//----------------------------------------------------------------------
void config3dsReadWriteString(const char *writeFormat, char *readFormat, char *value)
{
    if (!fp)
        return;
    
    if (WriteMode)
    {
        if (value != NULL)
        {
            //printf ("Writing %s %d\n", format, *value);
        	fprintf(fp, writeFormat, value);
        }
        else
        {
            //printf ("Writing %s\n", format);
        	fprintf(fp, writeFormat);
        }
    }
    else
    {
        if (value != NULL)
        {
            fscanf(fp, readFormat, value);
            char c;
            fscanf(fp, "%c", &c);
            //printf ("Scanned %s\n", value);
        }
        else
        {
            fscanf(fp, readFormat);
            char c;
            fscanf(fp, "%c", &c);
            //fscanf(fp, "%s", dummyString);
            //printf ("skipped line\n");
        }
    }
}

