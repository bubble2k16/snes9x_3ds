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
#include "3dsfiles.h"
#include "3dsinput.h"
#include "3dssettings.h"
#include "3dsimpl.h"
#include "3dsimpl_tilecache.h"
#include "3dsimpl_gpu.h"

#include "lodepng.h"

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

                    uint32 c = ((r << 24) | (g << 16) | (b << 8) | 0xff);
                    fb[x * 240 + (239 - y)] = c;
                }
            gfxSwapBuffers();
        }

        free(image);
    }
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
    config3dsReadWriteString("Dir=%s\n", "Dir=%1000[^\n]s\n", file3dsGetCurrentDir());
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

void emulatorLoadRom()
{
    consoleInit(GFX_BOTTOM, NULL);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    consoleClear();
    settingsSave(false);
    snprintf(romFileNameFullPath, _MAX_PATH, "%s%s", file3dsGetCurrentDir(), romFileName);

    impl3dsLoadROM(romFileNameFullPath);

    GPU3DS.emulatorState = EMUSTATE_EMULATE;

    consoleClear();
    settingsLoad();
    settingsUpdateAllSettings();
    menuSetupCheats();

    snd3DS.generateSilence = false;
}


//----------------------------------------------------------------------
// Menus
//----------------------------------------------------------------------
SMenuItem fileMenu[1000];
char romFileNames[1000][_MAX_PATH];

int totalRomFileCount = 0;

//----------------------------------------------------------------------
// Load all ROM file names (up to 1000 ROMs)
//----------------------------------------------------------------------
void fileGetAllFiles(void)
{
    std::vector<std::string> files = file3dsGetFiles("smc,sfc,fig", 1000);

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
    menu3dsSetTabSubTitle(1, file3dsGetCurrentDir());
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
                    file3dsGoToParentDirectory();
                else
                    file3dsGoToChildDirectory(&romFileName[2]);

                fileGetAllFiles();
                menu3dsClearMenuTabs();
                menu3dsAddTab("Emulator", emulatorNewMenu, emulatorMenuCount);
                menu3dsAddTab("Select ROM", fileMenu, totalRomFileCount);
                menu3dsSetCurrentMenuTab(1);
                menu3dsSetTabSubTitle(1, file3dsGetCurrentDir());
                selection = -1;
            }
            else
            {
                emulatorLoadRom();
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

    emulatorLoadRom();
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
    menu3dsSetTabSubTitle(3, file3dsGetCurrentDir());
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
                    file3dsGoToParentDirectory();
                else
                    file3dsGoToChildDirectory(&romFileName[2]);

                fileGetAllFiles();
                menu3dsClearMenuTabs();
                menu3dsAddTab("Emulator", emulatorMenu, emulatorMenuCount);
                menu3dsAddTab("Options", optionMenu, optionMenuCount);
                menu3dsAddTab("Cheats", cheatMenu, cheatMenuCount);
                menu3dsAddTab("Select ROM", fileMenu, totalRomFileCount);
                menu3dsSetCurrentMenuTab(3);
                menu3dsSetTabSubTitle(3, file3dsGetCurrentDir());
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
            char text[200];
           
            sprintf(text, "Saving into slot %d...\nThis may take a while", slot);
            menu3dsShowDialog("Savestates", text, DIALOGCOLOR_CYAN, NULL, 0);
            bool result = impl3dsSaveState(slot);
            menu3dsHideDialog();

            if (result)
            {
                sprintf(text, "Slot %d save completed.", slot);
                result = menu3dsShowDialog("Savestates", text, DIALOGCOLOR_GREEN, optionsForOk, sizeof(optionsForOk) / sizeof(SMenuItem));
                menu3dsHideDialog();
            }
            else
            {
                sprintf(text, "Oops. Unable to save slot %d!", slot);
                result = menu3dsShowDialog("Savestates", text, DIALOGCOLOR_RED, optionsForOk, sizeof(optionsForOk) / sizeof(SMenuItem));
                menu3dsHideDialog();
            }

            menu3dsSetSelectedItemIndexByID(0, 1000);
        }
        else if (selection >= 3001 && selection <= 3010)
        {
            int slot = selection - 3000;
            char text[200];

            bool result = impl3dsLoadState(slot);
            if (result)
            {
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                consoleClear();
                break;
            }
            else
            {
                sprintf(text, "Oops. Unable to load slot %d!", slot);
                menu3dsShowDialog("Savestates", text, DIALOGCOLOR_RED, optionsForOk, sizeof(optionsForOk) / sizeof(SMenuItem));
                menu3dsHideDialog();
            }
        }
        else if (selection == 4001)
        {
            menu3dsShowDialog("Screenshot", "Now taking a screenshot...\nThis may take a while.", DIALOGCOLOR_CYAN, NULL, 0);

            char ext[256];
            const char *path = NULL;

            // Loop through and look for an non-existing
            // file name.
            //
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
                impl3dsResetConsole();
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                consoleClear();

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
        emulatorLoadRom();

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


//--------------------------------------------------------
// Initialize the emulator engine and everything else.
// This calls the impl3dsInitializeCore, which executes
// initialization code specific to the emulation core.
//--------------------------------------------------------
void emulatorInitialize()
{
    file3dsInitialize();

    romFileNameLastSelected[0] = 0;

    if (!gpu3dsInitialize())
    {
        printf ("Unable to initialized GPU\n");
        exit(0);
    }

    printf ("Initializing...\n");

    if (!impl3dsInitializeCore())
    {
        printf ("Unable to initialize emulator core\n");
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

    // Do this one more time.
    if (file3dsGetCurrentDir()[0] == 0)
        file3dsInitialize();

    srvInit();
}


//--------------------------------------------------------
// Finalize the emulator.
//--------------------------------------------------------
void emulatorFinalize()
{
    consoleClear();

    impl3dsFinalize();

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


//---------------------------------------------------------
// Counts the number of frames per second, and prints
// it to the bottom screen every 60 frames.
//---------------------------------------------------------
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
// This is the main emulation loop. It calls the 
//    impl3dsRunOneFrame
//   (which must be implemented for any new core)
// for the execution of the frame.
//----------------------------------------------------------
void emulatorLoop()
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

    bool skipDrawingFrame = false;

    // Reinitialize the console.
    consoleInit(GFX_BOTTOM, NULL);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    menu3dsDrawBlackScreen();
    if (settings3DS.HideUnnecessaryBottomScrText == 0)
    {
        ui3dsDrawStringWithNoWrapping(0, 100, 320, 115, 0x7f7f7f, HALIGN_CENTER, "Touch screen for menu");
    }

    snd3dsStartPlaying();

	while (true)
	{
        t3dsStartTiming(1, "aptMainLoop");

        startFrameTick = svcGetSystemTick();
        aptMainLoop();

        if (appExiting)
            break;

        gpu3dsStartNewFrame();
        gpu3dsCheckSlider();
        updateFrameCount();

    	input3dsScanInputForEmulation();
        impl3dsRunOneFrame(firstFrame, skipDrawingFrame);

        if (GPU3DS.emulatorState != EMUSTATE_EMULATE)
            break;

        firstFrame = false; 

        // This either waits for the next frame, or decides to skip
        // the rendering for the next frame if we are too slow.
        //
#ifndef RELEASE
        if (GPU3DS.isReal3DS)
#endif
        {

            long currentTick = svcGetSystemTick();
            long actualTicksThisFrame = currentTick - startFrameTick;

            snesFrameTotalActualTicks += actualTicksThisFrame;  // actual time spent rendering past x frames.
            snesFrameTotalAccurateTicks += settings3DS.TicksPerFrame;  // time supposed to be spent rendering past x frames.

            int isSlow = 0;


            long skew = snesFrameTotalAccurateTicks - snesFrameTotalActualTicks;

            if (skew < 0)
            {
                // We've skewed out of the actual frame rate.
                // Once we skew beyond 0.1 (10%) frames slower, skip the frame.
                //
                if (skew < -settings3DS.TicksPerFrame/10 && snesFramesSkipped < settings3DS.MaxFrameSkips)
                {
                    skipDrawingFrame = true;
                    snesFramesSkipped++;

                    framesSkippedCount++;   // this is used for the stats display every 60 frames.
                }
                else
                {
                    skipDrawingFrame = false;

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

                // Reset the counters.
                //
                snesFrameTotalActualTicks = 0;
                snesFrameTotalAccurateTicks = 0;
                snesFramesSkipped = 0;

                svcSleepThread ((long)(timeDiffInMilliseconds * 1000));

                skipDrawingFrame = false;
            }

        }

	}

    snd3dsStopPlaying();
}


//---------------------------------------------------------
// Main entrypoint.
//---------------------------------------------------------
int main()
{
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
                emulatorLoop();
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
