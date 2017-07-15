#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <string>
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

inline std::string operator "" s(const char* s, unsigned int length) {
    return std::string(s, length);
}

S9xSettings3DS settings3DS;


#define TICKS_PER_SEC (268123480)
#define TICKS_PER_FRAME_NTSC (4468724)
#define TICKS_PER_FRAME_PAL (5362469)

int frameCount60 = 60;
u64 frameCountTick = 0;
int framesSkippedCount = 0;
char romFileName[_MAX_PATH];
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

namespace {
    template <typename T>
    bool CheckAndUpdate( T& oldValue, const T& newValue, bool& changed ) {
        if ( oldValue != newValue ) {
            oldValue = newValue;
            changed = true;
            return true;
        }
        return false;
    }
}

#define MENU_MAKE_ACTION(ID, text, callback) \
    items.emplace_back( callback, MenuItemType::Action, ID, text, ""s, 0 )

#define MENU_MAKE_DIALOG_ACTION(ID, text, desc) \
    items.emplace_back( nullptr, MenuItemType::Action, ID, text, desc, 0 )

#define MENU_MAKE_DISABLED(text) \
    items.emplace_back( nullptr, MenuItemType::Disabled, -1, text, ""s )

#define MENU_MAKE_HEADER1(text) \
    items.emplace_back( nullptr, MenuItemType::Header1, -1, text, ""s )

#define MENU_MAKE_HEADER2(text) \
    items.emplace_back( nullptr, MenuItemType::Header2, -1, text, ""s )

#define MENU_MAKE_CHECKBOX(ID, text, value, callback) \
    items.emplace_back( callback, MenuItemType::Checkbox, ID, text, ""s, value )

#define MENU_MAKE_GAUGE(ID, text, min, max, value, callback) \
    items.emplace_back( callback, MenuItemType::Gauge, ID, text, ""s, value, min, max )

#define MENU_MAKE_PICKER(ID, text, pickerDescription, pickerOptions, value, backColor, callback) \
    items.emplace_back( callback, MenuItemType::Picker, ID, text, ""s, value, 0, 0, pickerDescription, pickerOptions, backColor )

void exitEmulatorOptionSelected( int val ) {
    if ( val == 1 ) {
        GPU3DS.emulatorState = EMUSTATE_END;
        appExiting = 1;
    }
}

std::vector<SMenuItem> makeOptionsForNoYes() {
    std::vector<SMenuItem> items;
    MENU_MAKE_ACTION(0, "No"s, nullptr);
    MENU_MAKE_ACTION(1, "Yes"s, nullptr);
    return items;
}

std::vector<SMenuItem> makeEmulatorMenu() {
    std::vector<SMenuItem> items;
    MENU_MAKE_HEADER2   ("Resume"s);
    MENU_MAKE_ACTION    (1000, "  Resume Game"s, nullptr); // TODO!
    MENU_MAKE_HEADER2   (""s);

    MENU_MAKE_HEADER2   ("Savestates"s);
    MENU_MAKE_ACTION    (2001, "  Save Slot #1"s, nullptr); // TODO!
    MENU_MAKE_ACTION    (2002, "  Save Slot #2"s, nullptr); // TODO!
    MENU_MAKE_ACTION    (2003, "  Save Slot #3"s, nullptr); // TODO!
    MENU_MAKE_ACTION    (2004, "  Save Slot #4"s, nullptr); // TODO!
    MENU_MAKE_HEADER2   (""s);
    
    MENU_MAKE_ACTION    (3001, "  Load Slot #1"s, nullptr); // TODO!
    MENU_MAKE_ACTION    (3002, "  Load Slot #2"s, nullptr); // TODO!
    MENU_MAKE_ACTION    (3003, "  Load Slot #3"s, nullptr); // TODO!
    MENU_MAKE_ACTION    (3004, "  Load Slot #4"s, nullptr); // TODO!
    MENU_MAKE_HEADER2   (""s);

    MENU_MAKE_HEADER2   ("Others"s);
    MENU_MAKE_ACTION    (4001, "  Take Screenshot"s, nullptr); // TODO!
    MENU_MAKE_ACTION    (5001, "  Reset Console"s, nullptr); // TODO!
    MENU_MAKE_PICKER    (6001, "  Exit"s, "Leaving so soon?", makeOptionsForNoYes(), 0, DIALOGCOLOR_RED, exitEmulatorOptionSelected);
    return items;
}

std::vector<SMenuItem> makeOptionsForFont() {
    std::vector<SMenuItem> items;
    MENU_MAKE_DIALOG_ACTION (0, "Tempesta"s,              ""s);
    MENU_MAKE_DIALOG_ACTION (1, "Ronda"s,              ""s);
    MENU_MAKE_DIALOG_ACTION (2, "Arial"s,                 ""s);
    return items;
}

std::vector<SMenuItem> makeOptionsForStretch() {
    std::vector<SMenuItem> items;
    MENU_MAKE_DIALOG_ACTION (0, "No Stretch"s,              "'Pixel Perfect'"s);
    MENU_MAKE_DIALOG_ACTION (7, "Expand to Fit"s,           "'Pixel Perfect' fit"s);
    MENU_MAKE_DIALOG_ACTION (6, "TV-style"s,                "Stretch width only to 292px"s);
    MENU_MAKE_DIALOG_ACTION (5, "4:3"s,                     "Stretch width only"s);
    MENU_MAKE_DIALOG_ACTION (1, "4:3 Fit"s,                 "Stretch to 320x240"s);
    MENU_MAKE_DIALOG_ACTION (2, "Fullscreen"s,              "Stretch to 400x240"s);
    MENU_MAKE_DIALOG_ACTION (3, "Cropped 4:3 Fit"s,         "Crop & Stretch to 320x240"s);
    MENU_MAKE_DIALOG_ACTION (4, "Cropped Fullscreen"s,      "Crop & Stretch to 400x240"s);
    return items;
}

std::vector<SMenuItem> makeOptionsForFrameskip() {
    std::vector<SMenuItem> items;
    MENU_MAKE_DIALOG_ACTION (0, "Disabled"s,                ""s);
    MENU_MAKE_DIALOG_ACTION (1, "Enabled (max 1 frame)"s,   ""s);
    MENU_MAKE_DIALOG_ACTION (2, "Enabled (max 2 frames)"s,   ""s);
    MENU_MAKE_DIALOG_ACTION (3, "Enabled (max 3 frames)"s,   ""s);
    MENU_MAKE_DIALOG_ACTION (4, "Enabled (max 4 frames)"s,   ""s);
    return items;
};

std::vector<SMenuItem> makeOptionsForFrameRate() {
    std::vector<SMenuItem> items;
    MENU_MAKE_DIALOG_ACTION (0, "Default based on ROM region"s,    ""s);
    MENU_MAKE_DIALOG_ACTION (1, "50 FPS"s,                  ""s);
    MENU_MAKE_DIALOG_ACTION (2, "60 FPS"s,                  ""s);
    return items;
};

std::vector<SMenuItem> makeOptionsForAutoSaveSRAMDelay() {
    std::vector<SMenuItem> items;
    MENU_MAKE_DIALOG_ACTION (1, "1 second"s,    ""s);
    MENU_MAKE_DIALOG_ACTION (2, "10 seconds"s,  ""s);
    MENU_MAKE_DIALOG_ACTION (3, "60 seconds"s,  ""s);
    MENU_MAKE_DIALOG_ACTION (4, "Disabled"s,    "Touch bottom screen to save"s);
    return items;
};

std::vector<SMenuItem> makeOptionsForInFramePaletteChanges() {
    std::vector<SMenuItem> items;
    MENU_MAKE_DIALOG_ACTION (1, "Enabled"s,         "Best (not 100% accurate); slower"s);
    MENU_MAKE_DIALOG_ACTION (2, "Disabled Style 1"s,"Faster than \"Enabled\""s);
    MENU_MAKE_DIALOG_ACTION (3, "Disabled Style 2"s,"Faster than \"Enabled\""s);
    return items;
};

std::vector<SMenuItem> makeEmulatorNewMenu() {
    std::vector<SMenuItem> items;
    MENU_MAKE_PICKER(6001, "  Exit"s, "Leaving so soon?", makeOptionsForNoYes(), 0, DIALOGCOLOR_RED, exitEmulatorOptionSelected);
    return items;
}

std::vector<SMenuItem> makeOptionMenu() {
    std::vector<SMenuItem> items;
    MENU_MAKE_HEADER1   ("GLOBAL SETTINGS"s);
    MENU_MAKE_PICKER    (11000, "  Screen Stretch"s, "How would you like the final screen to appear?"s, makeOptionsForStretch(), settings3DS.ScreenStretch, DIALOGCOLOR_CYAN,
                         []( int val ) { CheckAndUpdate( settings3DS.ScreenStretch, val, settings3DS.Changed ); });
    MENU_MAKE_PICKER    (18000, "  Font"s, "The font used for the user interface."s, makeOptionsForFont(), settings3DS.Font, DIALOGCOLOR_CYAN,
                         []( int val ) { if ( CheckAndUpdate( settings3DS.Font, val, settings3DS.Changed ) ) { ui3dsSetFont(val); } });
    MENU_MAKE_CHECKBOX  (15001, "  Hide text in bottom screen"s, settings3DS.HideUnnecessaryBottomScrText,
                         []( int val ) { CheckAndUpdate( settings3DS.HideUnnecessaryBottomScrText, val, settings3DS.Changed ); });
    MENU_MAKE_DISABLED  (""s);
    MENU_MAKE_CHECKBOX  (19100, "  Automatically save state on exit and load state on start"s, settings3DS.AutoSavestate,
                         []( int val ) { CheckAndUpdate( settings3DS.AutoSavestate, val, settings3DS.Changed ); });
    MENU_MAKE_DISABLED  (""s);
    MENU_MAKE_HEADER1   ("GAME-SPECIFIC SETTINGS"s);
    MENU_MAKE_HEADER2   ("Graphics"s);
    MENU_MAKE_PICKER    (10000, "  Frameskip"s, "Try changing this if the game runs slow. Skipping frames help it run faster but less smooth."s, makeOptionsForFrameskip(), settings3DS.MaxFrameSkips, DIALOGCOLOR_CYAN,
                         []( int val ) { CheckAndUpdate( settings3DS.MaxFrameSkips, val, settings3DS.Changed ); });
    MENU_MAKE_PICKER    (12000, "  Framerate"s, "Some games run at 50 or 60 FPS by default. Override if required."s, makeOptionsForFrameRate(), settings3DS.ForceFrameRate, DIALOGCOLOR_CYAN,
                         []( int val ) { CheckAndUpdate( settings3DS.ForceFrameRate, val, settings3DS.Changed ); });
    MENU_MAKE_PICKER    (16000, "  In-Frame Palette Changes"s, "Try changing this if some colours in the game look off."s, makeOptionsForInFramePaletteChanges(), settings3DS.PaletteFix, DIALOGCOLOR_CYAN,
                         []( int val ) { CheckAndUpdate( settings3DS.PaletteFix, val, settings3DS.Changed ); });
    MENU_MAKE_DISABLED  (""s);
    MENU_MAKE_HEADER2   ("Audio"s);
    MENU_MAKE_GAUGE     (14000, "  Volume Amplification"s, 0, 8, settings3DS.Volume,
                         []( int val ) { CheckAndUpdate( settings3DS.Volume, val, settings3DS.Changed ); });
    MENU_MAKE_DISABLED  (""s);
    MENU_MAKE_HEADER2   ("Turbo (Auto-Fire) Buttons"s);
    MENU_MAKE_CHECKBOX  (13000, "  Button A"s, settings3DS.Turbo[0], []( int val ) { CheckAndUpdate( settings3DS.Turbo[0], val, settings3DS.Changed ); });
    MENU_MAKE_CHECKBOX  (13001, "  Button B"s, settings3DS.Turbo[1], []( int val ) { CheckAndUpdate( settings3DS.Turbo[1], val, settings3DS.Changed ); });
    MENU_MAKE_CHECKBOX  (13002, "  Button X"s, settings3DS.Turbo[2], []( int val ) { CheckAndUpdate( settings3DS.Turbo[2], val, settings3DS.Changed ); });
    MENU_MAKE_CHECKBOX  (13003, "  Button Y"s, settings3DS.Turbo[3], []( int val ) { CheckAndUpdate( settings3DS.Turbo[3], val, settings3DS.Changed ); });
    MENU_MAKE_CHECKBOX  (13004, "  Button L"s, settings3DS.Turbo[4], []( int val ) { CheckAndUpdate( settings3DS.Turbo[4], val, settings3DS.Changed ); });
    MENU_MAKE_CHECKBOX  (13005, "  Button R"s, settings3DS.Turbo[5], []( int val ) { CheckAndUpdate( settings3DS.Turbo[5], val, settings3DS.Changed ); });
    MENU_MAKE_DISABLED  (""s);
    MENU_MAKE_HEADER2   ("SRAM (Save Data)"s);
    MENU_MAKE_PICKER    (17000, "  SRAM Auto-Save Delay"s, "Try setting to 60 seconds or Disabled this if the game saves SRAM (Save Data) to SD card too frequently."s, makeOptionsForAutoSaveSRAMDelay(), settings3DS.SRAMSaveInterval, DIALOGCOLOR_CYAN,
                         []( int val ) { CheckAndUpdate( settings3DS.SRAMSaveInterval, val, settings3DS.Changed ); });
    MENU_MAKE_CHECKBOX  (19000, "  Force SRAM Write on Pause"s, settings3DS.ForceSRAMWriteOnPause,
                         []( int val ) { CheckAndUpdate( settings3DS.ForceSRAMWriteOnPause, val, settings3DS.Changed ); });
    return items;
};

void menuSetupCheats(std::vector<SMenuItem>& cheatMenu);

std::vector<SMenuItem> makeCheatMenu() {
    std::vector<SMenuItem> items;
    MENU_MAKE_HEADER2   ("Cheats"s);
    menuSetupCheats(items);
    return items;
};

std::vector<SMenuItem> makeOptionsForOk() {
    std::vector<SMenuItem> items;
    MENU_MAKE_ACTION(0, "OK"s, nullptr);
    return items;
}


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
        settings3DS.ForceSRAMWriteOnPause = 0;
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
    config3dsReadWriteInt32("ForceSRAMWrite=%d\n", &settings3DS.ForceSRAMWriteOnPause, 0, 1);

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

    config3dsReadWriteInt32("AutoSavestate=%d\n", &settings3DS.AutoSavestate, 0, 1);

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

    settings3DS.Changed = false;
    return true;
}

//----------------------------------------------------------------------
// Load settings by game.
//----------------------------------------------------------------------
bool settingsLoad(bool includeGameSettings = true)
{
    settings3DS.Changed = false;
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

void emulatorLoadRom()
{
    consoleInit(GFX_BOTTOM, NULL);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    consoleClear();
    settingsSave(false);

    char romFileNameFullPath[_MAX_PATH];
    snprintf(romFileNameFullPath, _MAX_PATH, "%s%s", file3dsGetCurrentDir(), romFileName);
    impl3dsLoadROM(romFileNameFullPath);

    GPU3DS.emulatorState = EMUSTATE_EMULATE;

    consoleClear();
    settingsLoad();
    settingsUpdateAllSettings();

    if (settings3DS.AutoSavestate)
        impl3dsLoadStateAuto();

    snd3DS.generateSilence = false;
}


//----------------------------------------------------------------------
// Load all ROM file names (up to 1000 ROMs)
//----------------------------------------------------------------------
void fileGetAllFiles(std::vector<SMenuItem>& fileMenu, std::vector<std::string>& romFileNames)
{
    fileMenu.clear();
    romFileNames.clear();

    // TODO: Lift the 1k file limitation once we no longer have hardcoded IDs for menu items.
    std::vector<std::string> files = file3dsGetFiles("smc,sfc,fig", 1000);

    // Increase the total number of files we can display.
    for (int i = 0; i < files.size() && i < 1000; i++)
    {
        romFileNames.emplace_back(files[i]);
        fileMenu.emplace_back(nullptr, MenuItemType::Action, i, std::string(romFileNames[i]), ""s);
    }
}


//----------------------------------------------------------------------
// Find the ID of the last selected file in the file list.
//----------------------------------------------------------------------
int fileFindLastSelectedFile(std::vector<SMenuItem>& fileMenu)
{
    for (int i = 0; i < fileMenu.size() && i < 1000; i++)
    {
        if (strncmp(fileMenu[i].Text.c_str(), romFileNameLastSelected, _MAX_PATH) == 0)
            return i;
    }
    return -1;
}


//----------------------------------------------------------------------
// Handle menu cheats.
//----------------------------------------------------------------------
bool menuCopyCheats(std::vector<SMenuItem>& cheatMenu, bool copyMenuToSettings)
{
    bool cheatsUpdated = false;
    for (int i = 0; (i+1) < cheatMenu.size() && i < MAX_CHEATS && i < Cheat.num_cheats; i++)
    {
        cheatMenu[i+1].Type = MenuItemType::Checkbox;
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
            cheatMenu[i+1].SetValue(Cheat.c[i].enabled);
    }
    
    return cheatsUpdated;
}


//----------------------------------------------------------------------
// Start up menu.
//----------------------------------------------------------------------
void menuSelectFile(void)
{
    std::vector<SMenuTab> menuTab;
    int currentMenuTab = 0;
    bool isDialog = false;
    SMenuTab dialogTab;
    std::vector<SMenuItem> fileMenu;
    std::vector<std::string> romFileNames;

    gfxSetDoubleBuffering(GFX_BOTTOM, true);

    fileGetAllFiles(fileMenu, romFileNames);
    menuTab.clear();
    menu3dsAddTab(menuTab, "Emulator", makeEmulatorNewMenu());
    menu3dsAddTab(menuTab, "Select ROM", fileMenu);
    menuTab[0].SubTitle.clear();
    menuTab[1].SubTitle.assign(file3dsGetCurrentDir());
    currentMenuTab = 1;

    int previousFileID = fileFindLastSelectedFile(fileMenu);
    menu3dsSetSelectedItemByIndex(menuTab[1], previousFileID);

    menu3dsSetTransferGameScreen(false);

    bool animateMenu = true;
    int selection = 0;
    do
    {
        if (appExiting)
            return;

        selection = menu3dsShowMenu(dialogTab, isDialog, currentMenuTab, menuTab, animateMenu);
        animateMenu = false;

        if (selection >= 0 && selection < 1000)
        {
            // Load ROM
            //
            strncpy(romFileName, romFileNames[selection].c_str(), _MAX_PATH);
            strncpy(romFileNameLastSelected, romFileName, _MAX_PATH);
            if (romFileName[0] == 1)
            {
                if (strcmp(romFileName, "\x01 ..") == 0)
                    file3dsGoToParentDirectory();
                else
                    file3dsGoToChildDirectory(&romFileName[2]);

                fileGetAllFiles(fileMenu, romFileNames);
                menuTab.clear();
                currentMenuTab = 0;
                menu3dsAddTab(menuTab, "Emulator", makeEmulatorNewMenu());
                menu3dsAddTab(menuTab, "Select ROM", fileMenu);
                currentMenuTab = 1;
                menuTab[1].SubTitle.assign(file3dsGetCurrentDir());
                selection = -1;
            }
            else
            {
                emulatorLoadRom();
                return;
            }
        }

        selection = -1;     // Bug fix: Fixes crashing when setting options before any ROMs are loaded.
    }
    while (selection == -1);

    menu3dsHideMenu(dialogTab, isDialog, currentMenuTab, menuTab);

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
}


//----------------------------------------------------------------------
// Menu when the emulator is paused in-game.
//----------------------------------------------------------------------
void menuPause()
{
    std::vector<SMenuTab> menuTab;
    int currentMenuTab = 0;
    bool isDialog = false;
    SMenuTab dialogTab;
    std::vector<SMenuItem> fileMenu;
    std::vector<std::string> romFileNames;

    gfxSetDoubleBuffering(GFX_BOTTOM, true);
    
    bool settingsUpdated = false;
    bool cheatsUpdated = false;
    bool loadRomBeforeExit = false;
    bool returnToEmulation = false;


    fileGetAllFiles(fileMenu, romFileNames);
    menuTab.clear();
    currentMenuTab = 0;
    menu3dsAddTab(menuTab, "Emulator", makeEmulatorMenu());
    menu3dsAddTab(menuTab, "Options", makeOptionMenu());
    menu3dsAddTab(menuTab, "Cheats", makeCheatMenu());
    menu3dsAddTab(menuTab, "Select ROM", fileMenu);
    std::vector<SMenuItem>& cheatMenu = menuTab[2].MenuItems;

    menuCopyCheats(cheatMenu, false);

    menuTab[0].SubTitle.clear();
    menuTab[1].SubTitle.clear();
    menuTab[2].SubTitle.clear();
    menuTab[3].SubTitle.assign(file3dsGetCurrentDir());

    int previousFileID = fileFindLastSelectedFile(fileMenu);
    menu3dsSetSelectedItemByIndex(menuTab[3], previousFileID);

    menu3dsSetTransferGameScreen(true);

    bool animateMenu = true;

    while (true)
    {
        if (appExiting)
        {
            break;
        }

        int selection = menu3dsShowMenu(dialogTab, isDialog, currentMenuTab, menuTab, animateMenu);
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
            strncpy(romFileName, romFileNames[selection].c_str(), _MAX_PATH);
            if (romFileName[0] == 1)
            {
                if (strcmp(romFileName, "\x01 ..") == 0)
                    file3dsGoToParentDirectory();
                else
                    file3dsGoToChildDirectory(&romFileName[2]);

                fileGetAllFiles(fileMenu, romFileNames);
                menuTab.clear();
                currentMenuTab = 0;
                menu3dsAddTab(menuTab, "Emulator", makeEmulatorMenu());
                menu3dsAddTab(menuTab, "Options", makeOptionMenu());
                menu3dsAddTab(menuTab, "Cheats", makeCheatMenu());
                menu3dsAddTab(menuTab, "Select ROM", fileMenu);
                currentMenuTab = 3;
                menuTab[3].SubTitle.assign(file3dsGetCurrentDir());
            }
            else
            {
                bool loadRom = true;

                // in case someone changed the AutoSavestate option while the menu was open
                if (settings3DS.Changed)
                    settingsSave();

                if (settings3DS.AutoSavestate)
                {
                    menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, "Save State", "Autosaving...", DIALOGCOLOR_CYAN, std::vector<SMenuItem>());
                    bool result = impl3dsSaveStateAuto();
                    menu3dsHideDialog(dialogTab, isDialog, currentMenuTab, menuTab);

                    if (!result)
                    {
                        int choice = menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, "Autosave failure", "Automatic savestate writing failed.\nLoad chosen game anyway?", DIALOGCOLOR_RED, makeOptionsForNoYes());
                        if (choice != 1)
                            loadRom = false;
                    }
                }

                if (loadRom)
                {
                    strncpy(romFileNameLastSelected, romFileName, _MAX_PATH);
                    loadRomBeforeExit = true;
                    break;
                }
            }
        }
        else if (selection >= 2001 && selection <= 2010)
        {
            int slot = selection - 2000;
            char text[200];
           
            sprintf(text, "Saving into slot %d...\nThis may take a while", slot);
            menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, "Savestates", text, DIALOGCOLOR_CYAN, std::vector<SMenuItem>());
            bool result = impl3dsSaveStateSlot(slot);
            menu3dsHideDialog(dialogTab, isDialog, currentMenuTab, menuTab);

            if (result)
            {
                sprintf(text, "Slot %d save completed.", slot);
                result = menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, "Savestates", text, DIALOGCOLOR_GREEN, makeOptionsForOk());
                menu3dsHideDialog(dialogTab, isDialog, currentMenuTab, menuTab);
            }
            else
            {
                sprintf(text, "Oops. Unable to save slot %d!", slot);
                result = menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, "Savestates", text, DIALOGCOLOR_RED, makeOptionsForOk());
                menu3dsHideDialog(dialogTab, isDialog, currentMenuTab, menuTab);
            }

            // TODO: Select 'resume' option again?
        }
        else if (selection >= 3001 && selection <= 3010)
        {
            int slot = selection - 3000;
            char text[200];

            bool result = impl3dsLoadStateSlot(slot);
            if (result)
            {
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                consoleClear();
                break;
            }
            else
            {
                sprintf(text, "Oops. Unable to load slot %d!", slot);
                menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, "Savestates", text, DIALOGCOLOR_RED, makeOptionsForOk());
                menu3dsHideDialog(dialogTab, isDialog, currentMenuTab, menuTab);
            }
        }
        else if (selection == 4001)
        {
            menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, "Screenshot", "Now taking a screenshot...\nThis may take a while.", DIALOGCOLOR_CYAN, std::vector<SMenuItem>());

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
            menu3dsHideDialog(dialogTab, isDialog, currentMenuTab, menuTab);

            if (success)
            {
                char text[600];
                snprintf(text, 600, "Done! File saved to %s", path);
                menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, "Screenshot", text, DIALOGCOLOR_GREEN, makeOptionsForOk());
                menu3dsHideDialog(dialogTab, isDialog, currentMenuTab, menuTab);
            }
            else 
            {
                menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, "Screenshot", "Oops. Unable to take screenshot!", DIALOGCOLOR_RED, makeOptionsForOk());
                menu3dsHideDialog(dialogTab, isDialog, currentMenuTab, menuTab);
            }
        }
        else if (selection == 5001)
        {
            int result = menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, "Reset Console", "Are you sure?", DIALOGCOLOR_RED, makeOptionsForNoYes());
            menu3dsHideDialog(dialogTab, isDialog, currentMenuTab, menuTab);

            if (result == 1)
            {
                impl3dsResetConsole();
                GPU3DS.emulatorState = EMUSTATE_EMULATE;
                consoleClear();

                break;
            }
            
        }
    }

    menu3dsHideMenu(dialogTab, isDialog, currentMenuTab, menuTab);

    // Save settings and cheats
    //
    if (settings3DS.Changed)
        settingsSave();
    settingsUpdateAllSettings();

    if (menuCopyCheats(cheatMenu, true))
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

void menuSetupCheats(std::vector<SMenuItem>& cheatMenu)
{
    if (Cheat.num_cheats > 0)
    {
        for (int i = 0; i < MAX_CHEATS && i < Cheat.num_cheats; i++)
        {
            cheatMenu.emplace_back(nullptr, MenuItemType::Checkbox, 20000+i, std::string(Cheat.c[i].name), ""s, Cheat.c[i].enabled ? 1 : 0);
        }
    }
    else
    {
        for (int i = 0; i < 14; i++)
        {
            cheatMenu.emplace_back(nullptr, MenuItemType::Disabled, -2, std::string(noCheatsText[i]), ""s);
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

    enableAptHooks();

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
    appSuspended = 0;

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

        if (appExiting || appSuspended)
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
    if (GPU3DS.emulatorState > 0 && settings3DS.AutoSavestate)
        impl3dsSaveStateAuto();

    printf("emulatorFinalize:\n");
    emulatorFinalize();
    printf ("Exiting...\n");
	exit(0);
}
