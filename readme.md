##Snes9x for 3DS

Snes9x for 3DS is a high-compatibility SNES emulator for your Old 3DS / 2DS. It runs many games at full speed (60 fps). It supports games that use CX4 chip (Megaman X2/X3), and the SDD-1 chip (Star Ocean, Super Street Fighter 2 Alpha). It can also play games that use the SuperFX chip (Yoshi's Island, etc), and the SA-1 chip (Super Mario RPG, Kirby's Dreamland, Kirby Super Star) but it runs slowly. It has generally much high compatibility than existing SNES emulators on the old 3DS because it uses Snes9x 1.43 as a base, and is a good alternative if your existing emulators cannot play all the games.

It also runs on your New 3DS, and it runs games faster! Use your New 3DS if you want to play SuperFX and SA-1 games!

Download the latest from:
   https://github.com/bubble2k16/snes9x_3ds/releases

Give feedback / report bugs here:
   https://gbatemp.net/threads/snes9x-for-old-3ds.435568/

To use:

###Homebrew Launcher:

1. Copy Snes9x_3ds.3dsx, Snes9x_3ds.smdh and snes9x_3ds_top.png into the /3ds/snes9x_3ds on your SD card. 
2. Place your SNES ROMs inside any folder.
3. Go to your Homebrew Launcher (either via Cubic Ninja, Soundhax or other entry points) and launch the snes9x_3ds emulator.

###CIA Version:

1. Use your favorite CIA installer to install snes9x_3ds.cia into your CFW.
2. Place your SNES ROMs inside any folder.
3. Copy snes9x_3ds_top.png to ROOT of your SD card.
4. Exit your CIA installer and go to your CFW's home screen to launch the app.

###When in-game,

1. Tap the bottom screen for the menu.
2. Use Up/Down to choose option, and A to confirm. 
   (Hold down X while pressing Up/Down to Page Up/Page Down)
3. Use Left/Right to change between ROM selection and emulator options.
4. You can quit the emulator to your homebrew launcher / your CFW's home screen.

Feedback and bug reports are welcome. Help with development is also welcome!

-------------------------------------------------------------------------------------------------------

##Screenshots

![Seiken Densetsu 3 (English Patch)](https://github.com/bubble2k16/snes9x_3ds/blob/master/images/snes9x_1.jpg?raw=1) 

Seiken Densetsu 3 (English Patch)

![Secret of Mana](https://github.com/bubble2k16/snes9x_3ds/blob/master/images/snes9x_2.jpg?raw=1) 

Secret of Mana

![Super Mario Kart](https://github.com/bubble2k16/snes9x_3ds/blob/master/images/snes9x_3.jpg?raw=1) 

Super Mario Kart

![Disney's Magical Quest 3 Starring Mickey & Donald](https://github.com/bubble2k16/snes9x_3ds/blob/master/images/snes9x_4.jpg?raw=1) 

Disney's Magical Quest 3 Starring Mickey & Donald

![Mighty Morphin Power Rangers - The Fighting Edition](https://github.com/bubble2k16/snes9x_3ds/blob/master/images/snes9x_5.jpg?raw=1) 

Mighty Morphin Power Rangers - The Fighting Edition

![Megaman X](https://github.com/bubble2k16/snes9x_3ds/blob/master/images/snes9x_6.jpg?raw=1) 

Megaman X

![Megaman X3](https://github.com/bubble2k16/snes9x_3ds/blob/master/images/snes9x_8.jpg?raw=1)

Megaman X3
 
![Castlevania - Dracula X](https://github.com/bubble2k16/snes9x_3ds/blob/master/images/snes9x_7.jpg?raw=1) 

Castlevania - Dracula X

-------------------------------------------------------------------------------------------------------

##Frequently Asked Questions 

1. Why do some games have strange color issues (Wild Guns, Kirby Super Star, Batman Forever)?

Try going to the Settings and change the In-Frame Palette Changes to either one of the 3 options: Enabled, Disabled Style 1, Disabled Style 2. Color emulation is never perfect because we are using the 3DS GPU for rendering, which lacks certain capabilities

2. Why some games keep writing to SRAM every second or so (Treasure Hunter G, Kirby's Dreamland)?

Try going to the Settings and change the SRAM Auto-Save Delay to 10 seconds, 60 seconds, or disable it entirely! Don't worry, the SRAM will be saved again when you tap on the bottom screen.

3. Why can't I load some games? They are supposed to be supported according to the compatibility list!

Ensure that your ROM file is not a zipped file. Currently, the emulator only supports .SFC, .SMC, .FIG formats.

4. Can I use my save states generated from Windows versions of Snes9x?

You can try using save states from Snes9x v1.43, but sometimes this emulator doesn't recognize them.

5. How can I make Snes9x utilize the full speed of the New 3DS?

The .3DSX version currently uses the full clock speed of the New 3DS, enabling all your SuperFX, SA-1 games to run faster. But the .CIA version currently has some problems, so you may have to use another app like HANS to configure the New 3DS to switch to its full clock speed before starting up this Snes9x emulator.

6. After closing the lid and re-opening it, the sound synchronization during game emulation goes off!

Yes, this is a known issue. You can try to tap the bottom to bring up the menu, and then resume your game. The sound should become synchronized again. 

7. After pressing the Home button, the emulator music still plays!

Try to avoid pressing the Home button or putting the 3DS to sleep. Quit the emulator entirely from the emulator's menu before going to the Home screen.

-------------------------------------------------------------------------------------------------------

##What's supported

1. Graphic modes 0 - 5, 7. 
2. Save states of up to 4 slots
3. Cheats - place your .CHT with the same filename in the same folder as your ROM. For example, if your ROM name is MyGame.smc, then your cheat file should be named MyGame.CHT.
4. Currently uses CSND for audio. So your entry point to home-brew must be able to use CSND. If you can play BlargSNES with sound, you should be able to play Snes9X_3DS with sound.
5. Frame skipping.
6. Stretch to full screen / 4:3 ratio 
7. PAL (50fps) / NTSC (60 fps) frame rates.
8. Navigation of ROMs in different folders.
9. SDD1 chip (Street Fighter 2 Alpha, Star Ocean)
10. SFX1/2 chip (Yoshi's Island, but slow on old 3DS)
11. CX4 chip (Megaman X-2, Megaman X-3)
12. DSP chips (Super Mario Kart, Ace o Nerae)
13. SA-1 chip (Super Mario RPG, Kirby Superstar)
14. Use of full clock speed in the New 3DS.
15. Sound emulation (at 32KHz, with echo and interpolation)

##What's missing / needs to be improved

1. Some sound emulation errors.
2. Mosaics.
3. In-frame palette changes - This is because this emulator uses the 3DS GPU for all graphic rendering. Without in-frame palette changes implemented, a small number of games experience colour issues.

-------------------------------------------------------------------------------------------------------

##Change History

v0.80
- Fixed NMI interrupt for specific games such as Cu-On-Pa. This allows Cu-On-Pa to get past the selection screen.
- Fixed ROM size calculation for Wonder Project J (English Patch). The game no longer hangs at the menu.
- Implemented optimized rendering for offset-per-tile modes. This gives many games a major performance boost. Games such as Strike Gunner STG, Tetris Attack, Kirby's Avalanche, Contra's Stage 1 Boss, Super Mario All-Stars' Super Mario 2 all benefit from this improvement.
- Implemented optimized rendering for sprites for modest performance boost.
- Used SPC700 core from Snes9x v1.51.
- Re-implemented an moderately optimized SA-1 core. Now games like Kirby Super Star, Super Mario RPG can boot. It will NOT run well on the old 3DS currently, but it will run smooth on a new 3DS with full clock speed.

v0.71
- Fixed nasty bug that breaks loading of the second ROM, if the first ROM was loaded from a directory path that has spaces in it.
- Added feature to allow users to page up/down the options in the menu while holding down X.
- Minor optimizations

v0.70
- Fixed sprites priority issues. This fixes sprites problems of Shadowrun, Chrono Trigger, Super Mario All Stars' (and possibly other games too) 
- The newly implemented rendering engine also provides some modest speed improvements in some games.
- Added feature to save the directory of the previously loaded ROM. 

v0.63
- Fixed bug in DMA when the source address comes from Save RAM. This fixes a number of Super Mario World Hacks so that text is readable on BG layer 3.
- Fixed menu key lock issue to improve usability when exiting from menu back into the game.
- Fixed DKC tile corruption issue. This probably also fixes most (if not all) tile corruption issues in other games, if you play a game for too long in a single sitting.
- Fixed sound problems when auto-saving SRAM. Somehow the hardware or the library doesn't like us stopping and starting the CSND too often. Not too sure what the problem is.
- Added an auto-save SRAM option to tell the emulator when to save SRAM (1 second, 10 seconds, 60 seconds, or disable auto-save).
Note that regardless of the setting, the emulator will always save any modified SRAM every time you touch the bottom screen to activate the menu. 

v0.62
- Improved Mode 7 zoomed-in textures. Games like Seiken Densetsu, ActRaiser 1/2 look better when viewing the Mode 7 textures near to the ground. Thanks for Discostew for the motivation!
- Improved sound handling to ensure that the left speaker plays at the same volume as the right speaker.
- Added option to change in-frame palette handling. There are three options to choose from: 
   > Enabled. (Slow, accurate, but never as accurate as software) 
   >  Disabled Style 1 (faster, less accurate)
   > Disabled Style 2 (faster, less accurate) works a little differently from Style 1 
  Most games are by default Disabled Style 2. 
  Bahamut Lagoon and Front Mission Gun Hazard are by default Disabled Style 1. 
  Wild Guns, Judge Dredd, Batman Forever are by default Enabled.
- Fixed Clock Tower to boot properly again.
- Fixed Star Fox flickering problem when there are frameskips.
- Fixed outside-of-track texture problem with Super Mario Kart’s Bowser Castle stages.
- Fixed Final Fantasy V Intro logo cutout problem.
- Fixed Donkey Kong Country's tile colour corruption bug

v0.61
- Fixed DSP bug that causes some games like Dragon Ball - Super Budoten 3, Umihara Kawase to mute. 
- Fixed bug that doesn't stop sound from playing when saving to SRAM.

v0.6
- Default sound amplification to 2x  …
- Reduced GPU freezing problem after loading new ROMs / resetting too many times
- Resynchronise sound when you go into the menu (you can reset the sync this way after waking your 
  3DS from sleep mode)
- Improved sound sample rate to 32 KHz, and added echo and interpolation. The result is overall 
  better sound quality
- Improved sync between SPC700 emulation and the DSP.
- Fixed Contra III Mode 7 EXTBG colors again.
- Enabled the SPC700 Envelope Height reading.
- Fixed Chrono Trigger music not resuming after any battle music or “sealed by mysterious force” music.

v0.51
- Implemented pseudo hi-res mode 5 using alternate frame flickering to achieve the pseudo hi-res mode. 
  Games that use this mode include: Secret of Mana, Seiken Densetsu 3, Air Strike Patrol, and a number of others.
  
v0.5
- Implemented major graphic rendering optimizations for color handling, transparency and window effects.
  Although codes from BlargSNES was not used directly, ideas came from BlargSNES.
  Some games that use windowing effects should see some / significant performance increase.
  F-Zero (US / EUR versions) run full speed.
  Super Mario Kart (EUR) runs full speed.
  Axelay (US) runs at a playable speed.
  Breath of Fire 2 battle transition effects are smooth.
  Super Mario World circular window fade-in/out effects are smooth.
  etc.
- Fixed sprite flickering problems in Yoshi's Island and DKC2.
- Fixed 256-color background problems. DKC1/2 and Earthworm Jim 2 intro shows up correctly.
- Fixed Mode 7 BG2 problems. Now Super Star Wars - Empire Strikes Back (snowspeeder stage) works correctly.

v0.42
- Minor optimisation to GPU drawing
- Increased maximum number of cheats to 200.
- Fix the crashing problem if there are too many cheats in the .CHT file.
- Fixed the too many sprites on screen problem (fixes Shadowrun freezing)

v0.41
- Fixed some transparency issues with Saturday Night Slam Master and Ghost Chaser Densei.
- Updated SRAM saving timing.
- Minor optimisation for SuperFX emulation.
- First implementation of speed hacks (experimental for F-Zero, Axelay,
Super Mario Kart, Yoshi’s Island)
- Modified some debugging stuff.
- Fixed some mode 7 glitches when a mode 7 game (like Super Mario Kart has been running for more than a few minutes)
- Implemented palette flickering hack for Front Mission Gun Hazard, Bahamut Lagoon.
- Implemented hack for Power Rangers Fighting Edition to prevent graphical glitches
- Implemented hack for delayed SRAM (per 1 minute) saving for Star Ocean
- Fixed the problem of loading up too many different ROMs once after another, causing the loading to finally fail.
- When loading a game without settings, reset the frame rate setting to be based on the game's region.

v0.4
- Experimental cheat feature. Requires you to have the .CHT file
  (May corrupt your save game, use at your own risk)
- Added settings to enable turbo buttons (per game)
- Added settings to allow user to amplify volume (per game)
  (beware of sound clipping that results in terrible sounds)
- Palette hack for Secret of Mana to ensure that the dialog borders are blue 
  (the bottom status window still behaves oddly due to palette changes)
- Transparency fixes:
  o Legend of Zelda's prologue's dark room
  o Reported dark tints on several games like Doremi Fantasy, Bomberman, Secret of Mana, Chrono Trigger
  o Gradient shading on dialog boxes for Chrono Trigger and Final Fantasy III (battles) appear correct.
- Fixed window clip problems.
- Fixed mode 7 glitches causing mode 7 bugs in Secret of Mana, NHL 94, Magical Quest 3 
- Fixed a number of sprite clipping problems 
  o Megaman 7's Dr Wily's spaceship in the intro now pans in from the left correctly
  o Tales of Phantasia's trainee's sprites (after the intro) now clips correct 
- Fixed color inconsistency causing some white sprites to be visible on a white background 
  (eg. in NHL's EA Sports intro screen)
- Added speed hack for games that use the WAI instruction 
  (esp Donkey Kong Country 1, 2, 3 should see a good speed increase)
- Added hack to avoid hiding sprites even if they exceed the maximum number the real hardware can display.
  Apparently, this fixes the missing cursor in Final Fantasy III.
- And other bugs.

v0.35
- Fixed Super Mario Kart grass area texture outside of the track. 
  This fix should work for all cases.
- Some minor performance and UI adjustments

v0.34
- Mode 7 minor performance improvements and update to draw Super Mario Kart's grass outside of the racing track. (some it works only under some conditions)
- Taking of screenshot now only captures the upper screen.
- Option of forcing frame rate to 50 FPS / 60 FPS / or based on the selected ROM's region.
- Additional screen scaling option to 320x240.
- Fixed the crashing problem when selecting options without any ROM running
- Fixed DKC1 piracy problem. The wrong mask logic was used when writing to SRAM.
- Improved the logic to save SRAM to SD card about 1-2 seconds after your game was saved in the emulator.

-------------------------------------------------------------------------------------------------------

##Credits

1. Snes9x team for the fantastic SNES emulator
2. StapleButter / DiscostewSM for their work on BlargSNES. Many ideas for optimisations came from them.
3. Author of SF2D library. Some codes for hardware acceleration evolved from here.
4. Authors of the Citra 3DS Emulator team. Without them, this project would have been extremely difficult.

