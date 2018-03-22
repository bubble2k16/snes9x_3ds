#include <array>

enum class EmulatedFramerate {
    UseRomRegion = 0,
    ForceFps50 = 1,
    ForceFps60 = 2,
    Match3DS = 3,
    Count = 4
};

template <int Count>
struct ButtonMapping {
    std::array<uint32, Count> MappingBitmasks;

    bool IsHeld(uint32 held3dsButtons) const {
        for (uint32 mapping : MappingBitmasks) {
            if (mapping != 0 && (mapping & held3dsButtons) == mapping) {
                return true;
            }
        }

        return false;
    }

    void SetSingleMapping(uint32 mapping) {
        SetDoubleMapping(mapping, 0);
    }

    void SetDoubleMapping(uint32 mapping0, uint32 mapping1) {
        if (Count > 0) {
            MappingBitmasks[0] = mapping0;
        }
        if (Count > 1) {
            MappingBitmasks[1] = mapping1;
        }
        if (Count > 2) {
            for (size_t i = 2; i < MappingBitmasks.size(); ++i) {
                MappingBitmasks[i] = 0;
            }
        }
    }
};

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

    EmulatedFramerate ForceFrameRate = EmulatedFramerate::UseRomRegion;

    int     StretchWidth, StretchHeight;
    int     CropPixels;

    int     Turbo[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // Turbo buttons: 0 - No turbo, 1 - Release/Press every alt frame.
                                            // Indexes: 0 - A, 1 - B, 2 - X, 3 - Y, 4 - L, 5 - R

    int     Volume = 4;                     // 0: 100% Default volume,
                                            // 1: 125%, 2: 150%, 3: 175%, 4: 200%
                                            // 5: 225%, 6: 250%, 7: 275%, 8: 300%

    long    TicksPerFrame;                  // Ticks per frame. Will change depending on PAL/NTSC

    int     PaletteFix;                     // Palette In-Frame Changes
                                            //   1 - Enabled - Default.
                                            //   2 - Disabled - Style 1.
                                            //   3 - Disabled - Style 2.

    int     AutoSavestate = 0;              // Automatically save the the current state when the emulator is closed
                                            // or the game is changed, and load it again when the game is loaded.
                                            //   0 - Disabled
                                            //   1 - Enabled

    int     SRAMSaveInterval;               // SRAM Save Interval
                                            //   1 - 1 second.
                                            //   2 - 10 seconds
                                            //   3 - 60 seconds
                                            //   4 - Never

    int     ForceSRAMWriteOnPause;          // If the SRAM should be written to SD even when no change was detected.
                                            // Some games (eg. Yoshi's Island) don't detect SRAM writes correctly.
                                            //   0 - Disabled
                                            //   1 - Enabled

    // Using the original button mapping to map the 3DS button
    // to the console buttons. This is for consistency with the
    // other EMUS for 3DS.
    //
    int     GlobalButtonMapping[10][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};  
    int     ButtonMapping[10][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}};  

    ::ButtonMapping<1> ButtonHotkeyOpenMenu; // Stores button that can be held to open the menu.

    ::ButtonMapping<1> ButtonHotkeyDisableFramelimit; // Stores button that can be held to disable the frame limit.

    ::ButtonMapping<1> GlobalButtonHotkeyOpenMenu; // Stores button that can be held to open the menu.

    ::ButtonMapping<1> GlobalButtonHotkeyDisableFramelimit; // Stores button that can be held to disable the frame limit.

    bool    Changed = false;                // Stores whether the configuration has been changed and should be written.

    int     UseGlobalButtonMappings = 0;    // Use global button mappings for all games
                                            // 0 - no, 1 - yes

    int     UseGlobalTurbo = 0;             // Use global button mappings for all games
                                            // 0 - no, 1 - yes

    int     UseGlobalVolume = 0;            // Use global button mappings for all games
                                            // 0 - no, 1 - yes

    int     UseGlobalEmuControlKeys = 0;    // Use global emulator control keys for all games

    int     GlobalTurbo[8] = {0, 0, 0, 0, 0, 0, 0, 0};  
                                            // Turbo buttons: 0 - No turbo, 1 - Release/Press every alt frame.
                                            // Indexes for 3DS buttons: 0 - A, 1 - B, 2 - X, 3 - Y, 4 - L, 5 - R, 6 - ZL, 7 - ZR

    int     GlobalVolume = 4;               // 0: 100%, 4: 200%, 8: 400%

    bool    RomFsLoaded = false;            // Stores whether we successfully opened the RomFS.

    int     UseGlobalDSPCore;               // Whether the DSP Core should apply to all games
    int     GlobalDSPCore;                  // Globally selected DSP Core
    int     DSPCore;                        // Game-specific selected DSP Core
} S9xSettings3DS;
