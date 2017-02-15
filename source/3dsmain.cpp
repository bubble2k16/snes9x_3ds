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
#include "snes9x.h"
#include "memmap.h"
#include "apu.h"
#include "gfx.h"
#include "snapshot.h"
#include "cheats.h"
#include "movie.h"
#include "display.h"
#include "soundux.h"

#include "3dsexit.h"
#include "3dsgpu.h"
#include "3dsopt.h"
#include "3dssound.h"
#include "3dsmenu.h"
#include "3dsui.h"
#include "3dsfont.h"
#include "3dsconfig.h"

#include "lodepng.h"


typedef struct
{
    int     MaxFrameSkips = 1;              // 0 - disable,
                                            // 1 - enable (max 1 consecutive skipped frame)
                                            // 2 - enable (max 2 consecutive skipped frames)
                                            // 3 - enable (max 3 consecutive skipped frames)
                                            // 4 - enable (max 4 consecutive skipped frames)

    int     HideUnnecessaryBottomScrText = 0;
                                            // Feature: add new option to disable unnecessary bottom screen text.
                                            // 0 - Default show FPS and "Touch screen for menu" text, 1 - Hide those text.

    int     Font = 0;                       // 0 - Tempesta, 1 - Ronda, 2 - Arial
    int     ScreenStretch = 0;              // 0 - no stretch, 1 - stretch full, 2 - aspect fit

    int     ForceFrameRate = 0;             // 0 - Use ROM's Region, 1 - Force 50 fps, 2 - Force 60 fps

    int     StretchWidth, StretchHeight;
    int     CropPixels;

    int     Turbo[6] = {0, 0, 0, 0, 0, 0};  // Turbo buttons: 0 - No turbo, 1 - Release/Press every alt frame.
                                            // Indexes: 0 - A, 1 - B, 2 - X, 3 - Y, 4 - L, 5 - R

    int     Volume = 4;                     // 0: 100% Default volume,
                                            // 1: 125%, 2: 150%, 3: 175%, 4: 200%
                                            // 5: 225%, 6: 250%, 7: 275%, 8: 300%

    long    TicksPerFrame;                  // Ticks per frame. Will change depending on PAL/NTSC

    int     PaletteFix;                     // Palette In-Frame Changes
                                            //   1 - Enabled - Default.
                                            //   2 - Disabled - Style 1.
                                            //   3 - Disabled - Style 2.

    int     SRAMSaveInterval;               // SRAM Save Interval
                                            //   1 - 1 second.
                                            //   2 - 10 seconds
                                            //   3 - 60 seconds
                                            //   4 - Never

} S9xSettings3DS;


S9xSettings3DS settings3DS;


#define TICKS_PER_SEC (268123480)
#define TICKS_PER_FRAME_NTSC (4468724)
#define TICKS_PER_FRAME_PAL (5362469)

int frameCount60 = 60;
u64 frameCountTick = 0;
int framesSkippedCount = 0;
char *romFileName = 0;
char romFileNameFullPath[_MAX_PATH];
char romFileNameLastSelected[_MAX_PATH];
char cwd[_MAX_PATH];


void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext)
{
	*drive = 0;

	const char	*slash = strrchr(path, SLASH_CHAR),
				*dot   = strrchr(path, '.');

	if (dot && slash && dot < slash)
		dot = NULL;

	if (!slash)
	{
		*dir = 0;

		strcpy(fname, path);

		if (dot)
		{
			fname[dot - path] = 0;
			strcpy(ext, dot + 1);
		}
		else
			*ext = 0;
	}
	else
	{
		strcpy(dir, path);
		dir[slash - path] = 0;

		strcpy(fname, slash + 1);

		if (dot)
		{
			fname[dot - slash - 1] = 0;
			strcpy(ext, dot + 1);
		}
		else
			*ext = 0;
	}
}

void _makepath (char *path, const char *, const char *dir, const char *fname, const char *ext)
{
	if (dir && *dir)
	{
		strcpy(path, dir);
		strcat(path, SLASH_STR);
	}
	else
		*path = 0;

	strcat(path, fname);

	if (ext && *ext)
	{
		strcat(path, ".");
		strcat(path, ext);
	}
}

void S9xMessage (int type, int number, const char *message)
{
	//printf("%s\n", message);
}

bool8 S9xInitUpdate (void)
{
	return (TRUE);
}

bool8 S9xDeinitUpdate (int width, int height, bool8 sixteen_bit)
{
	return (TRUE);
}


void S9xAutoSaveSRAM (void)
{
    // Ensure that the timer is reset
    //
    //CPU.AccumulatedAutoSaveTimer = 0;
    CPU.SRAMModified = false;

    ui3dsDrawRect(50, 140, 270, 154, 0x000000);
    ui3dsDrawStringWithNoWrapping(50, 140, 270, 154, 0x3f7fff, HALIGN_CENTER, "Saving SRAM to SD card...");

    // Bug fix: Instead of stopping CSND, we generate silence
    // like we did prior to v0.61
    //
    snd3DS.generateSilence = true;

    //int millisecondsToWait = 5;
    //svcSleepThread ((long)(millisecondsToWait * 1000));

	Memory.SaveSRAM (S9xGetFilename (".srm"));

    ui3dsDrawRect(50, 140, 270, 154, 0x000000);

    // Bug fix: Instead of starting CSND, we continue to mix
    // like we did prior to v0.61
    //
    snd3DS.generateSilence = false;
}

void S9xGenerateSound ()
{
}


void S9xExit (void)
{

}

void S9xSetPalette (void)
{
	return;
}


bool8 S9xOpenSoundDevice(int mode, bool8 stereo, int buffer_size)
{
	return (TRUE);
}

void S9xLoadSDD1Data ()
{
    //Settings.SDD1Pack=FALSE;

    char filename [_MAX_PATH + 1];
    char index [_MAX_PATH + 1];
    char data [_MAX_PATH + 1];

	Settings.SDD1Pack=FALSE;
    Memory.FreeSDD1Data ();

    if (strncmp (Memory.ROMName, ("Star Ocean"), 10) == 0)
	{
		Settings.SDD1Pack=TRUE;
	}
    else if(strncmp(Memory.ROMName, ("STREET FIGHTER ALPHA2"), 21)==0)
	{
		if(Memory.ROMRegion==1)
		{
			Settings.SDD1Pack=TRUE;
		}
		else
		{
			Settings.SDD1Pack=TRUE;
		}
	}
	else
	{
		Settings.SDD1Pack=TRUE;
	}
}

const char * S9xGetFilename (const char *ex)
{
	static char	s[PATH_MAX + 1];
	char		drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

	_splitpath(Memory.ROMFilename, drive, dir, fname, ext);
	snprintf(s, PATH_MAX + 1, "%s/%s%s", dir, fname, ex);

	return (s);
}

const char * S9xGetFilenameInc (const char *ex)
{
	static char	s[PATH_MAX + 1];
	char		drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

	unsigned int	i = 0;
	const char		*d;
	struct stat		buf;

	_splitpath(Memory.ROMFilename, drive, dir, fname, ext);

	do
		snprintf(s, PATH_MAX + 1, "%s/%s.%03d%s", dir, fname, i++, ex);
	while (stat(s, &buf) == 0 && i < 1000);

	return (s);
}

uint32 n3dsKeysHeld = 0;
uint32 lastKeysHeld = 0;
uint32 menuKeyDown = 0;
uint32 prevSnesJoyPad = 0;

uint32 S9xReadJoypad (int which1_0_to_4)
{
    if (which1_0_to_4 != 0)
        return 0;

    uint32 s9xKeysHeld = n3dsKeysHeld;

    if (menuKeyDown)
    {
        // If the key remains pressed after coming back
        // from the menu, we are going to mask it
        // until the user releases it.
        //
        if (s9xKeysHeld & menuKeyDown)
        {
            s9xKeysHeld = s9xKeysHeld & ~menuKeyDown;
        }
        else
            menuKeyDown = 0;
    }

    uint32 snesJoyPad = 0;

    if (s9xKeysHeld & KEY_UP) snesJoyPad |= SNES_UP_MASK;
    if (s9xKeysHeld & KEY_DOWN) snesJoyPad |= SNES_DOWN_MASK;
    if (s9xKeysHeld & KEY_LEFT) snesJoyPad |= SNES_LEFT_MASK;
    if (s9xKeysHeld & KEY_RIGHT) snesJoyPad |= SNES_RIGHT_MASK;
    if (s9xKeysHeld & KEY_L) snesJoyPad |= SNES_TL_MASK;
    if (s9xKeysHeld & KEY_R) snesJoyPad |= SNES_TR_MASK;
    if (s9xKeysHeld & KEY_SELECT) snesJoyPad |= SNES_SELECT_MASK;
    if (s9xKeysHeld & KEY_START) snesJoyPad |= SNES_START_MASK;
    if (s9xKeysHeld & KEY_A) snesJoyPad |= SNES_A_MASK;
    if (s9xKeysHeld & KEY_B) snesJoyPad |= SNES_B_MASK;
    if (s9xKeysHeld & KEY_X) snesJoyPad |= SNES_X_MASK;
    if (s9xKeysHeld & KEY_Y) snesJoyPad |= SNES_Y_MASK;

    // Handle turbo buttons.
    //
    #define HANDLE_TURBO(i, mask) if (settings3DS.Turbo[i] && (prevSnesJoyPad & mask) && (snesJoyPad & mask)) snesJoyPad &= ~mask;
    HANDLE_TURBO(0, SNES_A_MASK);
    HANDLE_TURBO(1, SNES_B_MASK);
    HANDLE_TURBO(2, SNES_X_MASK);
    HANDLE_TURBO(3, SNES_Y_MASK);
    HANDLE_TURBO(4, SNES_TL_MASK);
    HANDLE_TURBO(5, SNES_TR_MASK);

    prevSnesJoyPad = snesJoyPad;

    return snesJoyPad;
}

bool8 S9xReadMousePosition (int which1_0_to_1, int &x, int &y, uint32 &buttons)
{

}

bool8 S9xReadSuperScopePosition (int &x, int &y, uint32 &buttons)
{

}

bool JustifierOffscreen()
{
	return 0;
}

void JustifierButtons(uint32& justifiers)
{

}

char * osd_GetPackDir(void)
{

    return NULL;
}

const char *S9xBasename (const char *f)
{
    const char *p;
    if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL)
	return (p + 1);

    if (p = strrchr (f, SLASH_CHAR))
	return (p + 1);

    return (f);
}

bool8 S9xOpenSnapshotFile (const char *filename, bool8 read_only, STREAM *file)
{

	char	s[PATH_MAX + 1];
	char	drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

    snprintf(s, PATH_MAX + 1, "%s", filename);

	if ((*file = OPEN_STREAM(s, read_only ? "rb" : "wb")))
		return (TRUE);

	return (FALSE);
}

void S9xCloseSnapshotFile (STREAM file)
{
	CLOSE_STREAM(file);
}

void S9xParseArg (char **argv, int &index, int argc)
{

}

void S9xExtraUsage ()
{

}

void S9xGraphicsMode ()
{

}
void S9xTextMode ()
{

}
void S9xSyncSpeed (void)
{
}


//-------------------------------------------------------
// Clear top screen with logo.
//-------------------------------------------------------


void clearTopScreenWithLogo()
{
	unsigned char* image;
	unsigned width, height;

    int error = lodepng_decode32_file(&image, &width, &height, "./snes9x_3ds_top.png");

    if (!error && width == 400 && height == 240)
    {
        // GX_DisplayTransfer needs input buffer in linear RAM

        // lodepng outputs big endian rgba so we need to convert
        for (int i = 0; i < 2; i++)
        {
            u8* src = image;
            uint32* fb = (uint32 *) gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
            for (int y = 0; y < 240; y++)
                for (int x = 0; x < 400; x++)
                {
                    uint32 r = *src++;
                    uint32 g = *src++;
                    uint32 b = *src++;
                    uint32 a = *src++;

                    //r >>= 3;
                    //g >>= 3;
                    //b >>= 3;
                    //uint16 c = (uint16)((r << 11) | (g << 6) | (b << 1) | 1);
                    uint32 c = ((r << 24) | (g << 16) | (b << 8) | 0xff);
                    fb[x * 240 + (239 - y)] = c;
                }
            gfxSwapBuffers();
        }

        free(image);
    }
}



//-------------------------------------------
// Reads and processes Joy Pad buttons.
//-------------------------------------------
int debugFrameCounter = 0;
extern int csndTicksPerSecond;
int adjustableValue = 0x70;

uint32 readJoypadButtons()
{
    hidScanInput();
    n3dsKeysHeld = hidKeysHeld();

    u32 keysDown = (~lastKeysHeld) & n3dsKeysHeld;

#ifndef RELEASE
    // -----------------------------------------------
    // For debug only
    // -----------------------------------------------
    if (GPU3DS.enableDebug)
    {
        keysDown = keysDown & (~lastKeysHeld);
        if (keysDown || (n3dsKeysHeld & KEY_L))
        {
            //printf ("  kd:%x lkh:%x nkh:%x\n", keysDown, lastKeysHeld, n3dsKeysHeld);
            //Settings.Paused = false;
        }
        else
        {
            //printf ("  kd:%x lkh:%x nkh:%x\n", keysDown, lastKeysHeld, n3dsKeysHeld);
            //Settings.Paused = true;
        }
    }

    if (keysDown & (KEY_SELECT))
    {
        GPU3DS.enableDebug = !GPU3DS.enableDebug;
        printf ("Debug mode = %d\n", GPU3DS.enableDebug);
    }

    /*if (keysDown & (KEY_L))
    {
        adjustableValue -= 1;
        printf ("Adjust: %d\n", adjustableValue);
    }
    if (keysDown & (KEY_R))
    {
        adjustableValue += 1;
        printf ("Adjust: %d\n", adjustableValue);
    }*/
    // -----------------------------------------------
#endif

    if (keysDown & KEY_TOUCH)
    {
        // Save the SRAM if it has been modified before we going
        // into the menu.
        //
        if (CPU.SRAMModified || CPU.AutoSaveTimer)
        {
            S9xAutoSaveSRAM();
        }

        if (GPU3DS.emulatorState == EMUSTATE_EMULATE)
            GPU3DS.emulatorState = EMUSTATE_PAUSEMENU;
    }
    lastKeysHeld = n3dsKeysHeld;
    return keysDown;

}


//----------------------------------------------------------------------
// Menu options
//----------------------------------------------------------------------

#define MENU_MAKE_ACTION(ID, text) \
    { MENUITEM_ACTION, ID, text, NULL, 0 }

#define MENU_MAKE_DIALOG_ACTION(ID, text, desc) \
    { MENUITEM_ACTION, ID, text, desc, 0 }

#define MENU_MAKE_DISABLED(text) \
    { MENUITEM_DISABLED, -1, text, NULL }

#define MENU_MAKE_HEADER1(text) \
    { MENUITEM_HEADER1, -1, text, NULL }

#define MENU_MAKE_HEADER2(text) \
    { MENUITEM_HEADER2, -1, text, NULL }

#define MENU_MAKE_CHECKBOX(ID, text, value) \
    { MENUITEM_CHECKBOX, ID, text, NULL, value }

#define MENU_MAKE_GAUGE(ID, text, min, max, value) \
    { MENUITEM_GAUGE, ID, text, NULL, value, min, max }

#define MENU_MAKE_PICKER(ID, text, pickerDescription, pickerOptions, backColor) \
    { MENUITEM_PICKER, ID, text, NULL, 0, 0, 0, pickerDescription, sizeof(pickerOptions)/sizeof(SMenuItem), pickerOptions, backColor }


SMenuItem emulatorMenu[] = {
    MENU_MAKE_HEADER2   ("Resume"),
    MENU_MAKE_ACTION    (1000, "  Resume Game"),
    MENU_MAKE_HEADER2   (""),

    MENU_MAKE_HEADER2   ("Savestates"),
    MENU_MAKE_ACTION    (2001, "  Save Slot #1"),
    MENU_MAKE_ACTION    (2002, "  Save Slot #2"),
    MENU_MAKE_ACTION    (2003, "  Save Slot #3"),
    MENU_MAKE_ACTION    (2004, "  Save Slot #4"),
    MENU_MAKE_HEADER2   (""),
    
    MENU_MAKE_ACTION    (3001, "  Load Slot #1"),
    MENU_MAKE_ACTION    (3002, "  Load Slot #2"),
    MENU_MAKE_ACTION    (3003, "  Load Slot #3"),
    MENU_MAKE_ACTION    (3004, "  Load Slot #4"),
    MENU_MAKE_HEADER2   (""),

    MENU_MAKE_HEADER2   ("Others"),
    MENU_MAKE_ACTION    (4001, "  Take Screenshot"),
    MENU_MAKE_ACTION    (5001, "  Reset Console"),
    MENU_MAKE_ACTION    (6001, "  Exit"),
    /*
    { -1         ,   "Resume           ", -1, 0, 0, 0 },
    { 1000,          "  Resume Game    ", -1, 0, 0, 0 },
    { -1         ,   NULL,                -1, 0, 0, 0 },
    { -1         ,   "Savestates       ", -1, 0, 0, 0 },
    { 2001,          "  Save Slot #1   ", -1, 0, 0, 0 },
    { 2002,          "  Save Slot #2   ", -1, 0, 0, 0 },
    { 2003,          "  Save Slot #3   ", -1, 0, 0, 0 },
    { 2004,          "  Save Slot #4   ", -1, 0, 0, 0 },
    { -1         ,   NULL,                -1, 0, 0, 0 },
    { 3001,          "  Load Slot #1   ", -1, 0, 0, 0 },
    { 3002,          "  Load Slot #2   ", -1, 0, 0, 0 },
    { 3003,          "  Load Slot #3   ", -1, 0, 0, 0 },
    { 3004,          "  Load Slot #4   ", -1, 0, 0, 0 },
    { -1         ,   NULL,                -1, 0, 0, 0 },
    { -1         ,   "Emulation        ", -1, 0, 0, 0 },
    { 4001,          "  Take Screenshot", -1, 0, 0, 0 },
    { 5001,          "  Reset Console  ", -1, 0, 0, 0 },
    { 6001,          "  Exit           ", -1, 0, 0, 0 }*/
    };

SMenuItem optionsForFont[] = {
    MENU_MAKE_DIALOG_ACTION (0, "Tempesta",               ""),
    MENU_MAKE_DIALOG_ACTION (1, "Ronda",               ""),
    MENU_MAKE_DIALOG_ACTION (2, "Arial",                  "")
};

SMenuItem optionsForStretch[] = {
    MENU_MAKE_DIALOG_ACTION (0, "No Stretch",               "'Pixel Perfect'"),
    MENU_MAKE_DIALOG_ACTION (7, "Expand to Fit",            "'Pixel Perfect' fit"),
    MENU_MAKE_DIALOG_ACTION (6, "TV-style",                 "Stretch width only to 292px"),
    MENU_MAKE_DIALOG_ACTION (5, "4:3",                      "Stretch width only"),
    MENU_MAKE_DIALOG_ACTION (1, "4:3 Fit",                  "Stretch to 320x240"),
    MENU_MAKE_DIALOG_ACTION (2, "Fullscreen",               "Stretch to 400x240"),
    MENU_MAKE_DIALOG_ACTION (3, "Cropped 4:3 Fit",          "Crop & Stretch to 320x240"),
    MENU_MAKE_DIALOG_ACTION (4, "Cropped Fullscreen",       "Crop & Stretch to 400x240")
};

SMenuItem optionsForFrameskip[] = {
    MENU_MAKE_DIALOG_ACTION (0, "Disabled",                 ""),
    MENU_MAKE_DIALOG_ACTION (1, "Enabled (max 1 frame)",    ""),
    MENU_MAKE_DIALOG_ACTION (2, "Enabled (max 2 frames)",    ""),
    MENU_MAKE_DIALOG_ACTION (3, "Enabled (max 3 frames)",    ""),
    MENU_MAKE_DIALOG_ACTION (4, "Enabled (max 4 frames)",    "")
};

SMenuItem optionsForFrameRate[] = {
    MENU_MAKE_DIALOG_ACTION (0, "Default based on ROM",     ""),
    MENU_MAKE_DIALOG_ACTION (1, "50 FPS",                   ""),
    MENU_MAKE_DIALOG_ACTION (2, "60 FPS",                   "")
};

SMenuItem optionsForAutoSaveSRAMDelay[] = {
    MENU_MAKE_DIALOG_ACTION (1, "1 second",     ""),
    MENU_MAKE_DIALOG_ACTION (2, "10 seconds",   ""),
    MENU_MAKE_DIALOG_ACTION (3, "60 seconds",   ""),
    MENU_MAKE_DIALOG_ACTION (4, "Disabled",     "Touch bottom screen to save")
};

SMenuItem optionsForInFramePaletteChanges[] = {
    MENU_MAKE_DIALOG_ACTION (1, "Enabled",          "Best (not 100% accurate); slower"),
    MENU_MAKE_DIALOG_ACTION (2, "Disabled Style 1", "Faster than \"Enabled\""),
    MENU_MAKE_DIALOG_ACTION (3, "Disabled Style 2", "Faster than \"Enabled\"")
};

SMenuItem emulatorNewMenu[] = {
    MENU_MAKE_ACTION(6001, "  Exit")
    };

SMenuItem optionMenu[] = {
    MENU_MAKE_HEADER1   ("GLOBAL SETTINGS"),
    MENU_MAKE_PICKER    (11000, "  Screen Stretch", "How would you like the final screen to appear?", optionsForStretch, DIALOGCOLOR_CYAN),
    MENU_MAKE_PICKER    (18000, "  Font", "The font used for the user interface.", optionsForFont, DIALOGCOLOR_CYAN),
    MENU_MAKE_CHECKBOX  (15001, "  Hide text in bottom screen", 0),
    MENU_MAKE_DISABLED  (""),
    MENU_MAKE_HEADER1   ("GAME-SPECIFIC SETTINGS"),
    MENU_MAKE_HEADER2   ("Graphics"),
    MENU_MAKE_PICKER    (10000, "  Frameskip", "Try changing this if the game runs slow. Skipping frames help it run faster but less smooth.", optionsForFrameskip, DIALOGCOLOR_CYAN),
    MENU_MAKE_PICKER    (12000, "  Framerate", "Some games run at 50 or 60 FPS by default. Override if required.", optionsForFrameRate, DIALOGCOLOR_CYAN),
    MENU_MAKE_PICKER    (16000, "  In-Frame Palette Changes", "Try changing this if some colours in the game look off.", optionsForInFramePaletteChanges, DIALOGCOLOR_CYAN),
    MENU_MAKE_DISABLED  (""),
    MENU_MAKE_HEADER2   ("Audio"),
    MENU_MAKE_GAUGE     (14000, "  Volume Amplification", 0, 8, 4),
    MENU_MAKE_DISABLED  (""),
    MENU_MAKE_HEADER2   ("Turbo (Auto-Fire) Buttons"),
    MENU_MAKE_CHECKBOX  (13000, "  Button A", 0),
    MENU_MAKE_CHECKBOX  (13001, "  Button B", 0),
    MENU_MAKE_CHECKBOX  (13002, "  Button X", 0),
    MENU_MAKE_CHECKBOX  (13003, "  Button Y", 0),
    MENU_MAKE_CHECKBOX  (13004, "  Button L", 0),
    MENU_MAKE_CHECKBOX  (13005, "  Button R", 0),
    MENU_MAKE_DISABLED  (""),
    MENU_MAKE_HEADER2   ("SRAM (Save Data)"),
    MENU_MAKE_PICKER    (17000, "  SRAM Auto-Save Delay", "Try setting to 60 seconds or Disabled this if the game saves SRAM (Save Data) to SD card too frequently.", optionsForAutoSaveSRAMDelay, DIALOGCOLOR_CYAN)

    };

SMenuItem cheatMenu[MAX_CHEATS+1] =
{
    MENU_MAKE_HEADER2   ("Cheats")
    /*{ -1         , "Cheats", -1, 0, 0, 0 }*/
};

char *amplificationText[9] =
    {
        "  Volume Amplification (1.00x)",
        "  Volume Amplification (1.25x)",
        "  Volume Amplification (1.50x)",
        "  Volume Amplification (1.75x)",
        "  Volume Amplification (2.00x)",
        "  Volume Amplification (2.25x)",
        "  Volume Amplification (2.50x)",
        "  Volume Amplification (2.75x)",
        "  Volume Amplification (3.00x)"
    };
int emulatorMenuCount = 0;
int optionMenuCount = 0;
int cheatMenuCount = 1;

SMenuItem optionsForNoYes[] = {
    MENU_MAKE_ACTION(0, "No"),
    MENU_MAKE_ACTION(1, "Yes")
};

SMenuItem optionsForOk[] = {
    MENU_MAKE_ACTION(0, "OK")
};


//----------------------------------------------------------------------
// Update settings.
//----------------------------------------------------------------------

bool settingsUpdateAllSettings(bool updateGameSettings = true)
{
    bool settingsChanged = false;

    // update screen stretch
    //
    if (settings3DS.ScreenStretch == 0)
    {
        settings3DS.StretchWidth = 256;
        settings3DS.StretchHeight = -1;    // Actual height
        settings3DS.CropPixels = 0;
    }
    else if (settings3DS.ScreenStretch == 1)
    {
        // Added support for 320x240 (4:3) screen ratio
        settings3DS.StretchWidth = 320;
        settings3DS.StretchHeight = 240;
        settings3DS.CropPixels = 0;
    }
    else if (settings3DS.ScreenStretch == 2)
    {
        settings3DS.StretchWidth = 400;
        settings3DS.StretchHeight = 240;
        settings3DS.CropPixels = 0;
    }
    else if (settings3DS.ScreenStretch == 3)
    {
        settings3DS.StretchWidth = 320;
        settings3DS.StretchHeight = 240;
        settings3DS.CropPixels = 8;
    }
    else if (settings3DS.ScreenStretch == 4)
    {
        settings3DS.StretchWidth = 400;
        settings3DS.StretchHeight = 240;
        settings3DS.CropPixels = 8;
    }
    else if (settings3DS.ScreenStretch == 5)
    {
        settings3DS.StretchWidth = 04030000;       // Stretch width only to 4/3
        settings3DS.StretchHeight = -1;
        settings3DS.CropPixels = 0;
    }
    else if (settings3DS.ScreenStretch == 6)    // TV
    {
        settings3DS.StretchWidth = 292;       
        settings3DS.StretchHeight = -1;
        settings3DS.CropPixels = 0;
    }
    else if (settings3DS.ScreenStretch == 7)    // Stretch h/w but keep 1:1 ratio
    {
        settings3DS.StretchWidth = 01010000;       
        settings3DS.StretchHeight = 240;
        settings3DS.CropPixels = 0;
    }

    // Update the screen font
    //
    ui3dsSetFont(settings3DS.Font);

    if (updateGameSettings)
    {
        // Update frame rate
        //
        if (Settings.PAL)
            settings3DS.TicksPerFrame = TICKS_PER_FRAME_PAL;
        else
            settings3DS.TicksPerFrame = TICKS_PER_FRAME_NTSC;

        if (settings3DS.ForceFrameRate == 1)
            settings3DS.TicksPerFrame = TICKS_PER_FRAME_PAL;

        else if (settings3DS.ForceFrameRate == 2)
            settings3DS.TicksPerFrame = TICKS_PER_FRAME_NTSC;

        // update global volume
        //
        if (settings3DS.Volume < 0)
            settings3DS.Volume = 0;
        if (settings3DS.Volume > 8)
            settings3DS.Volume = 8;
        Settings.VolumeMultiplyMul4 = (settings3DS.Volume + 4);
        //printf ("vol: %d\n", Settings.VolumeMultiplyMul4);

        // update in-frame palette fix
        //
        if (settings3DS.PaletteFix == 1)
            SNESGameFixes.PaletteCommitLine = -2;
        else if (settings3DS.PaletteFix == 2)
            SNESGameFixes.PaletteCommitLine = 1;
        else if (settings3DS.PaletteFix == 3)
            SNESGameFixes.PaletteCommitLine = -1;
        else
        {
            if (SNESGameFixes.PaletteCommitLine == -2)
                settings3DS.PaletteFix = 1;
            else if (SNESGameFixes.PaletteCommitLine == 1)
                settings3DS.PaletteFix = 2;
            else if (SNESGameFixes.PaletteCommitLine == -1)
                settings3DS.PaletteFix = 3;
            settingsChanged = true;
        }

        if (settings3DS.SRAMSaveInterval == 1)
            Settings.AutoSaveDelay = 60;
        else if (settings3DS.SRAMSaveInterval == 2)
            Settings.AutoSaveDelay = 600;
        else if (settings3DS.SRAMSaveInterval == 3)
            Settings.AutoSaveDelay = 3600;
        else if (settings3DS.SRAMSaveInterval == 4)
            Settings.AutoSaveDelay = -1;
        else
        {
            if (Settings.AutoSaveDelay == 60)
                settings3DS.SRAMSaveInterval = 1;
            else if (Settings.AutoSaveDelay == 600)
                settings3DS.SRAMSaveInterval = 2;
            else if (Settings.AutoSaveDelay == 3600)
                settings3DS.SRAMSaveInterval = 3;
            settingsChanged = true;
        }
        
        // Fixes the Auto-Save timer bug that causes
        // the SRAM to be saved once when the settings were
        // changed to Disabled.
        //
        if (Settings.AutoSaveDelay == -1)
            CPU.AutoSaveTimer = -1;
        else
            CPU.AutoSaveTimer = 0;
    }

    return settingsChanged;
}


//----------------------------------------------------------------------
// Read/write all possible game specific settings.
//----------------------------------------------------------------------
bool settingsReadWriteFullListByGame(bool writeMode)
{
    bool success = config3dsOpenFile(S9xGetFilename(".cfg"), writeMode);
    if (!success)
        return false;

    config3dsReadWriteInt32("#v1\n", NULL, 0, 0);
    config3dsReadWriteInt32("# Do not modify this file or risk losing your settings.\n", NULL, 0, 0);

    // set default values first.
    if (!writeMode)
    {
        settings3DS.PaletteFix = 0;
        settings3DS.SRAMSaveInterval = 0;
    }

    config3dsReadWriteInt32("Frameskips=%d\n", &settings3DS.MaxFrameSkips, 0, 4);
    config3dsReadWriteInt32("Framerate=%d\n", &settings3DS.ForceFrameRate, 0, 2);
    config3dsReadWriteInt32("TurboA=%d\n", &settings3DS.Turbo[0], 0, 1);
    config3dsReadWriteInt32("TurboB=%d\n", &settings3DS.Turbo[1], 0, 1);
    config3dsReadWriteInt32("TurboX=%d\n", &settings3DS.Turbo[2], 0, 1);
    config3dsReadWriteInt32("TurboY=%d\n", &settings3DS.Turbo[3], 0, 1);
    config3dsReadWriteInt32("TurboL=%d\n", &settings3DS.Turbo[4], 0, 1);
    config3dsReadWriteInt32("TurboR=%d\n", &settings3DS.Turbo[5], 0, 1);
    config3dsReadWriteInt32("Vol=%d\n", &settings3DS.Volume, 0, 8);
    config3dsReadWriteInt32("PalFix=%d\n", &settings3DS.PaletteFix, 0, 3);
    config3dsReadWriteInt32("SRAMInterval=%d\n", &settings3DS.SRAMSaveInterval, 0, 4);

    // All new options should come here!

    config3dsCloseFile();
    return true;
}


//----------------------------------------------------------------------
// Read/write all possible game specific settings.
//----------------------------------------------------------------------
bool settingsReadWriteFullListGlobal(bool writeMode)
{
    bool success = config3dsOpenFile("./snes9x_3ds.cfg", writeMode);
    if (!success)
        return false;
    
    config3dsReadWriteInt32("#v1\n", NULL, 0, 0);
    config3dsReadWriteInt32("# Do not modify this file or risk losing your settings.\n", NULL, 0, 0);

    config3dsReadWriteInt32("ScreenStretch=%d\n", &settings3DS.ScreenStretch, 0, 7);
    config3dsReadWriteInt32("HideUnnecessaryBottomScrText=%d\n", &settings3DS.HideUnnecessaryBottomScrText, 0, 1);
    config3dsReadWriteInt32("Font=%d\n", &settings3DS.Font, 0, 2);

    // Fixes the bug where we have spaces in the directory name
    config3dsReadWriteString("Dir=%s\n", "Dir=%1000[^\n]s\n", cwd);
    config3dsReadWriteString("ROM=%s\n", "ROM=%1000[^\n]s\n", romFileNameLastSelected);

    // All new options should come here!

    config3dsCloseFile();
    return true;
}

//----------------------------------------------------------------------
// Save settings by game.
//----------------------------------------------------------------------
bool settingsSave(bool includeGameSettings = true)
{
    consoleClear();
    ui3dsDrawRect(50, 140, 270, 154, 0x000000);
    ui3dsDrawStringWithNoWrapping(50, 140, 270, 154, 0x3f7fff, HALIGN_CENTER, "Saving settings to SD card...");

    if (includeGameSettings)
        settingsReadWriteFullListByGame(true);

    settingsReadWriteFullListGlobal(true);
    ui3dsDrawRect(50, 140, 270, 154, 0x000000);

    return true;
}

//----------------------------------------------------------------------
// Load settings by game.
//----------------------------------------------------------------------
bool settingsLoad(bool includeGameSettings = true)
{
    bool success = settingsReadWriteFullListGlobal(false);
    if (!success)
        return false;
    settingsUpdateAllSettings(false);

    if (includeGameSettings)
    {
        success = settingsReadWriteFullListByGame(false);
        if (success)
        {
            if (settingsUpdateAllSettings())
                settingsSave();
            return true;
        }
        else
        {
            // If we can't find the saved settings, always
            // set the frame rate to be based on the ROM's region.
            // For the rest of the settings, we use whatever has been
            // set in the previous game.
            //
            settings3DS.MaxFrameSkips = 1;
            settings3DS.ForceFrameRate = 0;
            settings3DS.Volume = 4;

            for (int i = 0; i < 6; i++)     // and clear all turbo buttons.
                settings3DS.Turbo[i] = 0;

            if (SNESGameFixes.PaletteCommitLine == -2)
                settings3DS.PaletteFix = 1;
            else if (SNESGameFixes.PaletteCommitLine == 1)
                settings3DS.PaletteFix = 2;
            else if (SNESGameFixes.PaletteCommitLine == -1)
                settings3DS.PaletteFix = 3;

            if (Settings.AutoSaveDelay == 60)
                settings3DS.SRAMSaveInterval = 1;
            else if (Settings.AutoSaveDelay == 600)
                settings3DS.SRAMSaveInterval = 2;
            else if (Settings.AutoSaveDelay == 3600)
                settings3DS.SRAMSaveInterval = 3;

            settingsUpdateAllSettings();

            return settingsSave();
        }
    }
}




//-------------------------------------------------------
// Load the ROM and reset the CPU.
//-------------------------------------------------------

extern SCheatData Cheat;
void menuSetupCheats();  // forward declaration

void snesLoadRom()
{
    consoleInit(GFX_BOTTOM, NULL);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    consoleClear();
    settingsSave(false);
    snprintf(romFileNameFullPath, _MAX_PATH, "%s%s", cwd, romFileName);

    bool loaded = Memory.LoadROM(romFileNameFullPath);
    Memory.LoadSRAM (S9xGetFilename (".srm"));

    gpu3dsInitializeMode7Vertexes();
    gpu3dsCopyVRAMTilesIntoMode7TileVertexes(Memory.VRAM);
    cacheInit();
    //printf ("a\n");
    // Bug fix: For some reason doing this has a probability of locking up the GPU
    // so we will comment this out.
    //gpu3dsClearAllRenderTargets();
    //printf ("b\n");

    GPU3DS.emulatorState = EMUSTATE_EMULATE;

    consoleClear();
    settingsLoad();
    settingsUpdateAllSettings();
    menuSetupCheats();

    //Settings.HWOBJRenderingMode = 1;
    Settings.HWOBJRenderingMode = 0;

    debugFrameCounter = 0;
    prevSnesJoyPad = 0;
    snd3DS.generateSilence = false;
}


//----------------------------------------------------------------------
// Menus
//----------------------------------------------------------------------
SMenuItem fileMenu[1000];
char romFileNames[1000][_MAX_PATH];


int totalRomFileCount = 0;



//----------------------------------------------------------------------
// Go up to the parent directory.
//----------------------------------------------------------------------
void fileGoToParentDirectory(void)
{
    int len = strlen(cwd);

    if (len > 1)
    {
        for (int i = len - 2; i>=0; i--)
        {
            if (cwd[i] == '/')
            {

                cwd[i + 1] = 0;
                break;
            }
        }
    }
}


//----------------------------------------------------------------------
// Go up to the child directory.
//----------------------------------------------------------------------
void fileGoToChildDirectory(char *childDir)
{
    strncat(cwd, childDir, _MAX_PATH);
    strncat(cwd, "/", _MAX_PATH);
}


//----------------------------------------------------------------------
// Load all ROM file names (up to 512 ROMs)
//----------------------------------------------------------------------
void fileGetAllFiles(void)
{
    std::vector<std::string> files;
    char buffer[_MAX_PATH];

    struct dirent* dir;
    DIR* d = opendir(cwd);

    if (strlen(cwd) > 1)
    {
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
                if (!strstr(dir->d_name, ".smc") &&
                    !strstr(dir->d_name, ".fig") &&
                    !strstr(dir->d_name, ".sfc"))
                    continue;

                files.push_back(dir->d_name);
            }
        }
        closedir(d);
    }

    std::sort(files.begin(), files.end());

    totalRomFileCount = 0;

    // Increase the total number of files we can display.
    for (int i = 0; i < files.size() && i < 1000; i++)
    {
        strncpy(romFileNames[i], files[i].c_str(), _MAX_PATH);
        totalRomFileCount++;
        fileMenu[i].Type = MENUITEM_ACTION;
        fileMenu[i].ID = i;
        fileMenu[i].Text = romFileNames[i];
    }
}


//----------------------------------------------------------------------
// Find the ID of the last selected file in the file list.
//----------------------------------------------------------------------
int fileFindLastSelectedFile()
{
    for (int i = 0; i < totalRomFileCount && i < 1000; i++)
    {
        if (strncmp(fileMenu[i].Text, romFileNameLastSelected, _MAX_PATH) == 0)
            return i;
    }
    return -1;
}




//----------------------------------------------------------------------
// Copy values from menu to settings.
//----------------------------------------------------------------------
bool menuCopySettings(bool copyMenuToSettings)
{
#define UPDATE_SETTINGS(var, tabIndex, ID)  \
    if (copyMenuToSettings && (var) != menu3dsGetValueByID(tabIndex, ID)) \
    { \
        var = menu3dsGetValueByID(tabIndex, ID); \
        settingsUpdated = true; \
    } \
    if (!copyMenuToSettings) \
    { \
        menu3dsSetValueByID(tabIndex, ID, (var)); \
    }

    bool settingsUpdated = false;
    UPDATE_SETTINGS(settings3DS.Font, 1, 18000);
    UPDATE_SETTINGS(settings3DS.ScreenStretch, 1, 11000);
    UPDATE_SETTINGS(settings3DS.HideUnnecessaryBottomScrText, 1, 15001);
    UPDATE_SETTINGS(settings3DS.MaxFrameSkips, 1, 10000);
    UPDATE_SETTINGS(settings3DS.ForceFrameRate, 1, 12000);
    UPDATE_SETTINGS(settings3DS.Turbo[0], 1, 13000);
    UPDATE_SETTINGS(settings3DS.Turbo[1], 1, 13001);
    UPDATE_SETTINGS(settings3DS.Turbo[2], 1, 13002);
    UPDATE_SETTINGS(settings3DS.Turbo[3], 1, 13003);
    UPDATE_SETTINGS(settings3DS.Turbo[4], 1, 13004);
    UPDATE_SETTINGS(settings3DS.Turbo[5], 1, 13005);
    UPDATE_SETTINGS(settings3DS.Volume, 1, 14000);
    UPDATE_SETTINGS(settings3DS.PaletteFix, 1, 16000);
    UPDATE_SETTINGS(settings3DS.SRAMSaveInterval, 1, 17000);

    return settingsUpdated;
}


//----------------------------------------------------------------------
// Handle menu cheats.
//----------------------------------------------------------------------
bool menuCopyCheats(bool copyMenuToSettings)
{
    bool cheatsUpdated = false;
    for (int i = 0; i < MAX_CHEATS && i < Cheat.num_cheats; i++)
    {
        cheatMenu[i+1].Type = MENUITEM_CHECKBOX;
        cheatMenu[i+1].ID = 20000 + i;
        cheatMenu[i+1].Text = Cheat.c[i].name;

        if (copyMenuToSettings)
        {
            if (Cheat.c[i].enabled != cheatMenu[i+1].Value)
            {
                Cheat.c[i].enabled = cheatMenu[i+1].Value;
                if (Cheat.c[i].enabled)
                    S9xEnableCheat(i);
                else
                    S9xDisableCheat(i);
                cheatsUpdated = true;
            }
        }
        else
            cheatMenu[i+1].Value = Cheat.c[i].enabled;
    }
    
    return cheatsUpdated;
}


//----------------------------------------------------------------------
// Start up menu.
//----------------------------------------------------------------------
void menuSelectFile(void)
{
    gfxSetDoubleBuffering(GFX_BOTTOM, true);
    
    emulatorMenuCount = sizeof(emulatorNewMenu) / sizeof(SMenuItem);
    optionMenuCount = sizeof(optionMenu) / sizeof(SMenuItem);

    fileGetAllFiles();
    int previousFileID = fileFindLastSelectedFile();
    menu3dsClearMenuTabs();
    menu3dsAddTab("Emulator", emulatorNewMenu, emulatorMenuCount);
    menu3dsAddTab("Select ROM", fileMenu, totalRomFileCount);
    menu3dsSetTabSubTitle(0, NULL);
    menu3dsSetTabSubTitle(1, cwd);
    menu3dsSetCurrentMenuTab(1);
    if (previousFileID >= 0)
        menu3dsSetSelectedItemIndexByID(1, previousFileID);
    menu3dsSetTransferGameScreen(false);

    bool animateMenu = true;
    int selection = 0;
    do
    {
        if (appExiting)
            return;

        selection = menu3dsShowMenu(NULL, animateMenu);
        animateMenu = false;

        if (selection >= 0 && selection < 1000)
        {
            // Load ROM
            //
            romFileName = romFileNames[selection];
            strncpy(romFileNameLastSelected, romFileName, _MAX_PATH);
            if (romFileName[0] == 1)
            {
                if (strcmp(romFileName, "\x01 ..") == 0)
                    fileGoToParentDirectory();
                else
                    fileGoToChildDirectory(&romFileName[2]);

                fileGetAllFiles();
                menu3dsClearMenuTabs();
                menu3dsAddTab("Emulator", emulatorNewMenu, emulatorMenuCount);
                menu3dsAddTab("Select ROM", fileMenu, totalRomFileCount);
                menu3dsSetCurrentMenuTab(1);
                menu3dsSetTabSubTitle(1, cwd);
                selection = -1;
            }
            else
            {
                snesLoadRom();
                return;
            }
        }
        else if (selection == 6001)
        {
            int result = menu3dsShowDialog("Exit",  "Leaving so soon?", DIALOGCOLOR_RED, optionsForNoYes, sizeof(optionsForNoYes) / sizeof(SMenuItem));
            menu3dsHideDialog();

            if (result == 1)
            {
                GPU3DS.emulatorState = EMUSTATE_END;
                return;
            }
        }

        selection = -1;     // Bug fix: Fixes crashing when setting options before any ROMs are loaded.
    }
    while (selection == -1);

    menu3dsHideMenu();

    snesLoadRom();
}


//----------------------------------------------------------------------
// Checks if file exists.
//----------------------------------------------------------------------
bool IsFileExists(const char * filename) {
    if (FILE * file = fopen(filename, "r")) {
        fclose(file);
        return true;
    }
    return false;
}


//----------------------------------------------------------------------
// Callback when a menu item changes
//----------------------------------------------------------------------
void menuItemChangedCallback(int ID, int value)
{
    if (ID == 18000)
    {
        ui3dsSetFont(value);
    }
}


//----------------------------------------------------------------------
// Menu when the emulator is paused in-game.
//----------------------------------------------------------------------

void menuPause()
{
    gfxSetDoubleBuffering(GFX_BOTTOM, true);
    
    emulatorMenuCount = sizeof(emulatorMenu) / sizeof(SMenuItem);
    optionMenuCount = sizeof(optionMenu) / sizeof(SMenuItem);
    bool settingsUpdated = false;
    bool cheatsUpdated = false;
    bool loadRomBeforeExit = false;
    bool returnToEmulation = false;


    menu3dsClearMenuTabs();
    menu3dsAddTab("Emulator", emulatorMenu, emulatorMenuCount);
    menu3dsAddTab("Options", optionMenu, optionMenuCount);
    menu3dsAddTab("Cheats", cheatMenu, cheatMenuCount);
    menu3dsAddTab("Select ROM", fileMenu, totalRomFileCount);

    menuCopySettings(false);
    menuCopyCheats(false);

    int previousFileID = fileFindLastSelectedFile();
    menu3dsSetTabSubTitle(0, NULL);
    menu3dsSetTabSubTitle(1, NULL);
    menu3dsSetTabSubTitle(2, NULL);
    menu3dsSetTabSubTitle(3, cwd);
    if (previousFileID >= 0)
        menu3dsSetSelectedItemIndexByID(3, previousFileID);
    menu3dsSetCurrentMenuTab(0);
    menu3dsSetTransferGameScreen(true);

    bool animateMenu = true;

    while (true)
    {
        if (appExiting)
        {
            break;
        }

        int selection = menu3dsShowMenu(menuItemChangedCallback, animateMenu);
        animateMenu = false;

        if (selection == -1 || selection == 1000)
        {
            // Cancels the menu and resumes game
            //
            returnToEmulation = true;

            break;
        }
        else if (selection < 1000)
        {
            // Load ROM
            //
            romFileName = romFileNames[selection];
            if (romFileName[0] == 1)
            {
                if (strcmp(romFileName, "\x01 ..") == 0)
                    fileGoToParentDirectory();
                else
                    fileGoToChildDirectory(&romFileName[2]);

                fileGetAllFiles();
                menu3dsClearMenuTabs();
                menu3dsAddTab("Emulator", emulatorMenu, emulatorMenuCount);
                menu3dsAddTab("Options", optionMenu, optionMenuCount);
                menu3dsAddTab("Cheats", cheatMenu, cheatMenuCount);
                menu3dsAddTab("Select ROM", fileMenu, totalRomFileCount);
                menu3dsSetCurrentMenuTab(3);
                menu3dsSetTabSubTitle(3, cwd);
            }
            else
            {
                strncpy(romFileNameLastSelected, romFileName, _MAX_PATH);
                loadRomBeforeExit = true;
                break;
            }
        }
        else if (selection >= 2001 && selection <= 2010)
        {
            int slot = selection - 2000;
            char s[_MAX_PATH];
            char text[200];
           
            sprintf(text, "Saving into slot %d...\nThis may take a while", slot);
            int result = menu3dsShowDialog("Savestates", text, DIALOGCOLOR_CYAN, NULL, 0);
            sprintf(s, ".%d.frz", slot);
            Snapshot(S9xGetFilename (s));
            menu3dsHideDialog();

            sprintf(text, "Slot %d save completed.", slot);
            result = menu3dsShowDialog("Savestates", text, DIALOGCOLOR_GREEN, optionsForOk, sizeof(optionsForOk) / sizeof(SMenuItem));
            menu3dsHideDialog();

            menu3dsSetSelectedItemIndexByID(0, 1000);
        }
        else if (selection >= 3001 && selection <= 3010)
        {
            int slot = selection - 3000;
            char s[_MAX_PATH];

            sprintf(s, ".%d.frz", slot);
            if (S9xLoadSnapshot(S9xGetFilename (s)))
            {
                gpu3dsInitializeMode7Vertexes();
                gpu3dsCopyVRAMTilesIntoMode7TileVertexes(Memory.VRAM);
                debugFrameCounter = 0;
                // Bug fix: For some reason doing this has a probability of locking up the GPU
                // so we will comment this out.
                //gpu3dsClearAllRenderTargets();
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                consoleClear();

                break;
            }
            else
            {
                sprintf(s, "Oops. Unable to load slot %d!", slot);
                menu3dsShowDialog("Savestates", s, DIALOGCOLOR_RED, optionsForOk, sizeof(optionsForOk) / sizeof(SMenuItem));
                menu3dsHideDialog();
            }
        }
        else if (selection == 4001)
        {
            menu3dsShowDialog("Screenshot", "Now taking a screenshot...\nThis may take a while.", DIALOGCOLOR_CYAN, NULL, 0);

            char ext[256];
            const char *path = NULL;

            int i = 1;
            while (i <= 999)
            {
                snprintf(ext, 255, ".b%03d.bmp", i);
                path = S9xGetFilename(ext);
                if (!IsFileExists(path))
                    break;
                path = NULL;
                i++;
            }


            bool success = false;
            if (path)
            {
                success = menu3dsTakeScreenshot(path);
            }
            menu3dsHideDialog();

            if (success)
            {
                char text[600];
                snprintf(text, 600, "Done! File saved to %s", path);
                menu3dsShowDialog("Screenshot", text, DIALOGCOLOR_GREEN, optionsForOk, sizeof(optionsForOk)/sizeof(SMenuItem));
                menu3dsHideDialog();
            }
            else 
            {
                menu3dsShowDialog("Screenshot", "Oops. Unable to take screenshot!", DIALOGCOLOR_RED, optionsForOk, sizeof(optionsForOk)/sizeof(SMenuItem));
                menu3dsHideDialog();
            }
        }
        else if (selection == 5001)
        {
            int result = menu3dsShowDialog("Reset Console", "Are you sure?", DIALOGCOLOR_RED, optionsForNoYes, sizeof(optionsForNoYes) / sizeof(SMenuItem));
            menu3dsHideDialog();

            if (result == 1)
            {
                S9xReset();
                cacheInit();
                gpu3dsInitializeMode7Vertexes();
                gpu3dsCopyVRAMTilesIntoMode7TileVertexes(Memory.VRAM);
                // Bug fix: For some reason doing this has a probability of locking up the GPU
                // so we will comment this out.
                //gpu3dsClearAllRenderTargets();
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                consoleClear();

                prevSnesJoyPad = 0;

                break;
            }
            
        }
        else if (selection == 6001)
        {
            int result = menu3dsShowDialog("Exit",  "Leaving so soon?", DIALOGCOLOR_RED, optionsForNoYes, sizeof(optionsForNoYes) / sizeof(SMenuItem));
            if (result == 1)
            {
                GPU3DS.emulatorState = EMUSTATE_END;

                break;
            }
            else
                menu3dsHideDialog();
            
        }

    }

    menu3dsHideMenu();

    // Gets the last key pressed and saves it into menuKeyDown
    //
    while (aptMainLoop())
    {
        hidScanInput();
        menuKeyDown = hidKeysHeld() & (KEY_A | KEY_B | KEY_START);
        break;
    }

    // Save settings and cheats
    //
    if (menuCopySettings(true))
        settingsSave();
    settingsUpdateAllSettings();

    if (menuCopyCheats(true))
    {
        // Only one of these will succeeed.
        S9xSaveCheatFile (S9xGetFilename(".cht"));
        S9xSaveCheatTextFile (S9xGetFilename(".chx"));
    }

    if (returnToEmulation)
    {
        GPU3DS.emulatorState = EMUSTATE_EMULATE;
        consoleClear();
    }

    // Loads the new ROM if a ROM was selected.
    //
    if (loadRomBeforeExit)
        snesLoadRom();

}

//-------------------------------------------------------
// Sets up all the cheats to be displayed in the menu.
//-------------------------------------------------------
char *noCheatsText[] {
    "",
    "    No cheats available for this game ",
    "",
    "    To enable cheats:  ",
    "      Copy your .CHT/.CHX file into the same folder as  ",
    "      ROM file and make sure it has the same name. ",
    "",
    "      If your ROM filename is: ",
    "          MyGame.smc ",
    "      Then your cheat filename must be: ",
    "          MyGame.cht or MyGame.chx ",
    "",
    "    Refer to readme.md for the .CHX file format. ",
    ""
     };

void menuSetupCheats()
{
    if (Cheat.num_cheats > 0)
    {
        cheatMenuCount = Cheat.num_cheats + 1;

        // Bug fix: If the number of cheats exceeds what we can store,
        // make sure we limit it.
        //
        if (cheatMenuCount > MAX_CHEATS)
            cheatMenuCount = MAX_CHEATS;
        for (int i = 0; i < MAX_CHEATS && i < Cheat.num_cheats; i++)
        {
            cheatMenu[i+1].Type = MENUITEM_CHECKBOX;
            cheatMenu[i+1].ID = 20000 + i;
            cheatMenu[i+1].Text = Cheat.c[i].name;
            cheatMenu[i+1].Value = Cheat.c[i].enabled ? 1 : 0;
        }
    }
    else
    {
        cheatMenuCount = 14;
        for (int i = 0; i < cheatMenuCount; i++)
        {
            cheatMenu[i+1].Type = MENUITEM_DISABLED;
            cheatMenu[i+1].ID = -2;
            cheatMenu[i+1].Text = noCheatsText[i];
        }
    }
}


#define P1_ButtonA 1
#define P1_ButtonB 2
#define P1_ButtonX 3
#define P1_ButtonY 4
#define P1_ButtonL 5
#define P1_ButtonR 6
#define P1_Up 7
#define P1_Down 8
#define P1_Left 9
#define P1_Right 10
#define P1_Select 11
#define P1_Start 1


//-------------------------------------------------------
// Initialize the SNES 9x engine.
//-------------------------------------------------------
bool snesInitialize()
{

    memset(&Settings, 0, sizeof(Settings));
    Settings.Paused = false;
    Settings.BGLayering = TRUE;
    Settings.SoundBufferSize = 0;
    Settings.CyclesPercentage = 100;
    Settings.APUEnabled = Settings.NextAPUEnabled = TRUE;
    Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
    Settings.SkipFrames = 0;
    Settings.ShutdownMaster = TRUE;
    Settings.FrameTimePAL = 20000;
    Settings.FrameTimeNTSC = 16667;
    Settings.FrameTime = Settings.FrameTimeNTSC;
    Settings.DisableSampleCaching = FALSE;
    Settings.DisableMasterVolume = FALSE;
    Settings.Mouse = FALSE;
    Settings.SuperScope = FALSE;
    Settings.MultiPlayer5 = FALSE;
    Settings.ControllerOption = SNES_JOYPAD;
    Settings.SupportHiRes = FALSE;
    Settings.NetPlay = FALSE;
    Settings.ServerName [0] = 0;
    Settings.ThreadSound = FALSE;
    Settings.AutoSaveDelay = 60;         // Bug fix to save SRAM within 60 frames (1 second instead of 30 seconds)
#ifdef _NETPLAY_SUPPORT
    Settings.Port = NP_DEFAULT_PORT;
#endif
    Settings.ApplyCheats = TRUE;
    Settings.TurboMode = FALSE;
    Settings.TurboSkipFrames = 15;

    Settings.Transparency = FALSE;
    Settings.SixteenBit = TRUE;
    Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;

    // Sound related settings.
    Settings.DisableSoundEcho = FALSE;
    Settings.SixteenBitSound = TRUE;
    Settings.SoundPlaybackRate = SAMPLE_RATE;
    Settings.Stereo = TRUE;
    Settings.SoundBufferSize = 0;
    Settings.APUEnabled = Settings.NextAPUEnabled = TRUE;
    Settings.InterpolatedSound = TRUE;
    Settings.AltSampleDecode = 0;
    Settings.SoundEnvelopeHeightReading = 1;

    if(!Memory.Init())
    {
        printf ("Unable to initialize memory.\n");
        return false;
    }

    if(!S9xInitAPU())
    {
        printf ("Unable to initialize APU.\n");
        return false;
    }

    if(!S9xGraphicsInit())
    {
        printf ("Unable to initialize graphics.\n");
        return false;
    }


    if(!S9xInitSound (
        7, Settings.Stereo,
        Settings.SoundBufferSize))
    {
        printf ("Unable to initialize sound.\n");
        return false;
    }
    so.playback_rate = Settings.SoundPlaybackRate;
    so.stereo = Settings.Stereo;
    so.sixteen_bit = Settings.SixteenBitSound;
    so.buffer_size = 32768;
    so.encoded = FALSE;


    return true;
}


//--------------------------------------------------------
// Initialize the Snes9x engine and everything else.
//--------------------------------------------------------
void emulatorInitialize()
{
    getcwd(cwd, 1023);
    romFileNameLastSelected[0] = 0;

    if (!gpu3dsInitialize())
    {
        printf ("Unable to initialized GPU\n");
        exit(0);
    }

    printf ("Initializing...\n");

    cacheInit();
    if (!snesInitialize())
    {
        printf ("Unable to initialize SNES9x\n");
        exit(0);
    }

    if (!snd3dsInitialize())
    {
        printf ("Unable to initialize CSND\n");
        exit (0);
    }

    ui3dsInitialize();

    /*if (romfsInit()!=0)
    {
        printf ("Unable to initialize romfs\n");
        exit (0);
    }
    */
    printf ("Initialization complete\n");

    osSetSpeedupEnable(1);    // Performance: use the higher clock speed for new 3DS.

    enableExitHook();

    settingsLoad(false);
    if (cwd[0] == 0)
        getcwd(cwd, 1023);
}


//--------------------------------------------------------
// Finalize the emulator.
//--------------------------------------------------------
void emulatorFinalize()
{
    consoleClear();

#ifndef RELEASE
    printf("gspWaitForP3D:\n");
#endif
    gspWaitForVBlank();
    gpu3dsWaitForPreviousFlush();
    gspWaitForVBlank();

#ifndef RELEASE
    printf("snd3dsFinalize:\n");
#endif
    snd3dsFinalize();

#ifndef RELEASE
    printf("gpu3dsFinalize:\n");
#endif
    gpu3dsFinalize();

#ifndef RELEASE
    printf("S9xGraphicsDeinit:\n");
#endif
    S9xGraphicsDeinit();

#ifndef RELEASE
    printf("S9xDeinitAPU:\n");
#endif
    S9xDeinitAPU();
    
#ifndef RELEASE
    printf("Memory.Deinit:\n");
#endif
    Memory.Deinit();

#ifndef RELEASE
    printf("ptmSysmExit:\n");
#endif
    ptmSysmExit ();

    //printf("romfsExit:\n");
    //romfsExit();
    
#ifndef RELEASE
    printf("hidExit:\n");
#endif
	hidExit();
    
#ifndef RELEASE
    printf("aptExit:\n");
#endif
	aptExit();
    
#ifndef RELEASE
    printf("srvExit:\n");
#endif
	srvExit();
}



bool firstFrame = true;



char frameCountBuffer[70];
void updateFrameCount()
{
    if (frameCountTick == 0)
        frameCountTick = svcGetSystemTick();

    if (frameCount60 == 0)
    {
        u64 newTick = svcGetSystemTick();
        float timeDelta = ((float)(newTick - frameCountTick))/TICKS_PER_SEC;
        int fpsmul10 = (int)((float)600 / timeDelta);

#if !defined(RELEASE) && !defined(DEBUG_CPU) && !defined(DEBUG_APU)
        consoleClear();
#endif

        if (settings3DS.HideUnnecessaryBottomScrText == 0)
        {
            if (framesSkippedCount)
                snprintf (frameCountBuffer, 69, "FPS: %2d.%1d (%d skipped)\n", fpsmul10 / 10, fpsmul10 % 10, framesSkippedCount);
            else
                snprintf (frameCountBuffer, 69, "FPS: %2d.%1d \n", fpsmul10 / 10, fpsmul10 % 10);

            ui3dsDrawRect(2, 2, 200, 16, 0x000000);
            ui3dsDrawStringWithNoWrapping(2, 2, 200, 16, 0x7f7f7f, HALIGN_LEFT, frameCountBuffer);
        }

        frameCount60 = 60;
        framesSkippedCount = 0;


#if !defined(RELEASE) && !defined(DEBUG_CPU) && !defined(DEBUG_APU)
        printf ("\n\n");
        for (int i=0; i<100; i++)
        {
            t3dsShowTotalTiming(i);
        }
        t3dsResetTimings();
#endif
        frameCountTick = newTick;

    }

    frameCount60--;
}





//----------------------------------------------------------
// Main SNES emulation loop.
//----------------------------------------------------------
void snesEmulatorLoop()
{
	// Main loop
    //GPU3DS.enableDebug = true;

    int snesFramesSkipped = 0;
    long snesFrameTotalActualTicks = 0;
    long snesFrameTotalAccurateTicks = 0;

    bool firstFrame = true;

    snd3DS.generateSilence = false;

    gpu3dsResetState();

    frameCount60 = 60;
    frameCountTick = 0;
    framesSkippedCount = 0;

    long startFrameTick = svcGetSystemTick();

    IPPU.RenderThisFrame = true;

    // Reinitialize the console.
    consoleInit(GFX_BOTTOM, NULL);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    menu3dsDrawBlackScreen();
    if (settings3DS.HideUnnecessaryBottomScrText == 0)
    {
        ui3dsDrawStringWithNoWrapping(0, 100, 320, 115, 0x7f7f7f, HALIGN_CENTER, "Touch screen for menu");
    }

    snd3dsStartPlaying();

	while (aptMainLoop())
	{
        t3dsStartTiming(1, "aptMainLoop");

        Memory.ApplySpeedHackPatches();
        startFrameTick = svcGetSystemTick();

        if (appExiting)
            break;

        updateFrameCount();

        gpu3dsStartNewFrame();
        gpu3dsEnableAlphaBlending();

		readJoypadButtons();
        if (GPU3DS.emulatorState != EMUSTATE_EMULATE)
            break;

		gpu3dsSetRenderTargetToMainScreenTexture();
	    gpu3dsUseShader(2);             // for drawing tiles

#ifdef RELEASE
        if (!Settings.SA1)
            S9xMainLoop();
        else
            S9xMainLoopWithSA1();
#else
        if (!Settings.Paused)
        {
            if (!Settings.SA1)
                S9xMainLoop();
            else
                S9xMainLoopWithSA1();
        }
#endif
/*
        if (IPPU.RenderThisFrame)
        {
            for (int j = 0; j < 10; j++)
                for (int i = 0; i < 65536; i++)
                    forSlowSimulation[i] = Memory.VRAM[i] + (i*j);
        }
  */

        // ----------------------------------------------
        // Copy the SNES main/sub screen to the 3DS frame
        // buffer
        // (Can this be done in the V_BLANK?)
        t3dsStartTiming(3, "CopyFB");
        gpu3dsSetRenderTargetToTopFrameBuffer();

        if (firstFrame)
        {
            // Clear the entire frame buffer to black, including the borders
            //
            gpu3dsDisableAlphaBlending();
            gpu3dsSetTextureEnvironmentReplaceColor();
            gpu3dsDrawRectangle(0, 0, 400, 240, 0, 0x000000ff);
            gpu3dsEnableAlphaBlending();
        }

        gpu3dsUseShader(1);             // for copying to screen.
        gpu3dsDisableAlphaBlending();
        gpu3dsDisableDepthTest();
        gpu3dsDisableAlphaTest();

        gpu3dsBindTextureMainScreen(GPU_TEXUNIT0);
        gpu3dsSetTextureEnvironmentReplaceTexture0();
        gpu3dsDisableStencilTest();

        int sWidth = settings3DS.StretchWidth;
        int sHeight = (settings3DS.StretchHeight == -1 ? PPU.ScreenHeight - 1 : settings3DS.StretchHeight);
        if (sWidth == 04030000)
            sWidth = sHeight * 4 / 3;
        else if (sWidth == 01010000)
        {
            if (PPU.ScreenHeight < SNES_HEIGHT_EXTENDED)
            {
                sWidth = SNES_HEIGHT_EXTENDED * SNES_WIDTH / SNES_HEIGHT;
                sHeight = SNES_HEIGHT_EXTENDED;
            }
            else
            {
                sWidth = SNES_WIDTH;
                sHeight = SNES_HEIGHT_EXTENDED;
            }
        }

        int sx0 = (400 - sWidth) / 2;
        int sx1 = sx0 + sWidth;
        int sy0 = (240 - sHeight) / 2;
        int sy1 = sy0 + sHeight;

        gpu3dsAddQuadVertexes(
            sx0, sy0, sx1, sy1,
            settings3DS.CropPixels, settings3DS.CropPixels ? settings3DS.CropPixels : 1, 
            256 - settings3DS.CropPixels, PPU.ScreenHeight - settings3DS.CropPixels, 
            0.1f);
        gpu3dsDrawVertexes();

        t3dsEndTiming(3);

        if (!firstFrame)
        {
            // ----------------------------------------------
            // Wait for the rendering to the SNES
            // main/sub screen for the previous frame
            // to complete
            //
            t3dsStartTiming(5, "Transfer");
            gpu3dsTransferToScreenBuffer();
            gpu3dsSwapScreenBuffers();
            t3dsEndTiming(5);

        }
        else
        {
            firstFrame = false;
        }

        // ----------------------------------------------
        // Flush all draw commands of the current frame
        // to the GPU.
        t3dsStartTiming(4, "Flush");
        gpu3dsFlush();
        t3dsEndTiming(4);

        t3dsEndTiming(1);


        // For debugging only.
        /*if (!GPU3DS.isReal3DS)
        {
            snd3dsMixSamples();
            //snd3dsMixSamples();
            //printf ("---\n");
        }*/

        /*
        // Debugging only
        snd3dsMixSamples();
        printf ("\n");

        S9xPrintAPUState ();
        printf ("----\n");*/


#ifndef RELEASE
        if (GPU3DS.isReal3DS)
#endif
        {

            long currentTick = svcGetSystemTick();
            long actualTicksThisFrame = currentTick - startFrameTick;

            snesFrameTotalActualTicks += actualTicksThisFrame;  // actual time spent rendering past x frames.
            snesFrameTotalAccurateTicks += settings3DS.TicksPerFrame;  // time supposed to be spent rendering past x frames.

            //printf ("%7.5f - %7.5f = %7.5f ",
            //    snesFrameTotalActualTime, snesFrameTotalCorrectTime,
            //    snesFrameTotalActualTime - snesFrameTotalCorrectTime);

            int isSlow = 0;


            long skew = snesFrameTotalAccurateTicks - snesFrameTotalActualTicks;

            //printf ("%ld %ld sk : %ld", snesFrameTotalAccurateTicks, snesFrameTotalActualTicks, skew);
            if (skew < 0)
            {
                // We've skewed out of the actual frame rate.
                // Once we skew beyond 0.1 (10%) frames slower, skip the frame.
                //
                if (skew < -settings3DS.TicksPerFrame/10 && snesFramesSkipped < settings3DS.MaxFrameSkips)
                {
                    //printf (" s\n");
                    // Skewed beyond threshold. So now we skip.
                    //
                    IPPU.RenderThisFrame = false;
                    snesFramesSkipped++;

                    framesSkippedCount++;   // this is used for the stats display every 60 frames.
                }
                else
                {
                    //printf (" -\n");
                    IPPU.RenderThisFrame = true;

                    if (snesFramesSkipped >= settings3DS.MaxFrameSkips)
                    {
                        snesFramesSkipped = 0;
                        snesFrameTotalActualTicks = actualTicksThisFrame;
                        snesFrameTotalAccurateTicks = settings3DS.TicksPerFrame;
                    }
                }
            }
            else
            {

                float timeDiffInMilliseconds = (float)skew * 1000000 / TICKS_PER_SEC;

                //printf (" +\n");
                svcSleepThread ((long)(timeDiffInMilliseconds * 1000));

                IPPU.RenderThisFrame = true;

                // Reset the counters.
                //
                snesFrameTotalActualTicks = 0;
                snesFrameTotalAccurateTicks = 0;
                snesFramesSkipped = 0;
            }

        }

	}

    snd3dsStopPlaying();
}


int main()
{
    //testAPU();
    //testGPU();
    //testTileCache();
    emulatorInitialize();
    clearTopScreenWithLogo();


    menuSelectFile();

    while (true)
    {
        if (appExiting)
            goto quit;

        switch (GPU3DS.emulatorState)
        {
            case EMUSTATE_PAUSEMENU:
                menuPause();
                break;

            case EMUSTATE_EMULATE:
                snesEmulatorLoop();
                break;

            case EMUSTATE_END:
                goto quit;

        }

    }

quit:
    printf("emulatorFinalize:\n");
    emulatorFinalize();
    printf ("Exiting...\n");
	exit(0);
}
