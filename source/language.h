/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/

/* This is where all the GUI text strings will eventually end up */

#define WINDOW_TITLE "Snes9X v%s for Windows"

#define MY_REG_KEY   "Software\\Emulators\\Snes9X"

#define REG_KEY_VER  "1.31"

#define DISCLAIMER_TEXT        "Snes9X v%s for Windows.\r\n" \
        "(c) Copyright 1996 - 2002 Gary Henderson and Jerremy Koot.\r\n" \
        "(c) Copyright 2001- 2004 John Weidman.\r\n" \
	"(c) Copyright 2002 - 2004 blip, Brad Jorsch, funkyass, Joel Yliluoma, Kris Bleakley, Matthew Kendora, Nach, Peter Bortas, zones.\r\n\r\n" \
	"Snes9X is a Super Nintendo Entertainment System\r\n" \
		"emulator that allows you to play most games designed\r\n" \
		"for the SNES on your PC.\r\n\r\n" \
		"Please visit http://www.snes9x.com for\r\n" \
		"up-to-the-minute information and help on Snes9X.\r\n\r\n" \
		"Nintendo is a trade mark."


#define APP_NAME "Snes9x"
/* possible global strings */
#define SNES9X_INFO "Snes9x: Information"
#define SNES9X_WARN "Snes9x: WARNING!"
#define SNES9X_DXS "Snes9X: DirectSound"
#define SNES9X_SNDQ "Snes9X: Sound CPU Question"
#define SNES9X_NP_ERROR "Snes9X: NetPlay Error"
#define BUTTON_OK "&OK"
#define BUTTON_CANCEL "&Cancel"

/* Gamepad Dialog Strings */
#define INPUTCONFIG_TITLE "Input Configuration"
#define INPUTCONFIG_JPTOGGLE "Enable"
#define INPUTCONFIG_DIAGTOGGLE "Toggle Diagonals"
/* #define INPUTCONFIG_OK "&OK" */
/* #define INPUTCONFIG_CANCEL "&Cancel" */
#define INPUTCONFIG_JPCOMBO "Joypad #%d"
#define INPUTCONFIG_LABEL_UP "Up"
#define INPUTCONFIG_LABEL_DOWN "Down"
#define INPUTCONFIG_LABEL_LEFT "Left"
#define INPUTCONFIG_LABEL_RIGHT "Right"
#define INPUTCONFIG_LABEL_A "A"
#define INPUTCONFIG_LABEL_B "B"
#define INPUTCONFIG_LABEL_X "X"
#define INPUTCONFIG_LABEL_Y "Y"
#define INPUTCONFIG_LABEL_L "L"
#define INPUTCONFIG_LABEL_R "R"
#define INPUTCONFIG_LABEL_START "Start"
#define INPUTCONFIG_LABEL_SELECT "Select"
#define INPUTCONFIG_LABEL_UPLEFT "Up Left"
#define INPUTCONFIG_LABEL_UPRIGHT "Up Right"
#define INPUTCONFIG_LABEL_DOWNRIGHT "Down Right"
#define INPUTCONFIG_LABEL_DOWNLEFT "Down Left"
#define INPUTCONFIG_LABEL_BLUE "Blue means the current key/button is already mapped; Red means it's a Snes9x/Windows reserved key."

/* gaming buttons and axises */
#define GAMEDEVICE_JOYNUMPREFIX "(J%d)"
#define GAMEDEVICE_JOYBUTPREFIX "#[%d]"
#define GAMEDEVICE_XNEG "Left"
#define GAMEDEVICE_XPOS "Right"
#define GAMEDEVICE_YPOS "Up"
#define GAMEDEVICE_YNEG "Down"
#define GAMEDEVICE_POVLEFT "POV Left"
#define GAMEDEVICE_POVRIGHT "POV Right"
#define GAMEDEVICE_POVUP "POV Up"
#define GAMEDEVICE_POVDOWN "POV Down" 
#define GAMEDEVICE_POVDNLEFT "POV Dn Left" 
#define GAMEDEVICE_POVDNRIGHT "POV Dn Right" 
#define GAMEDEVICE_POVUPLEFT  "POV Up Left" 
#define GAMEDEVICE_POVUPRIGHT "POV Up Right" 
#define GAMEDEVICE_ZPOS "Z Up"
#define GAMEDEVICE_ZNEG "Z Down"
#define GAMEDEVICE_RPOS "R Up"
#define GAMEDEVICE_RNEG "R Down"
#define GAMEDEVICE_UPOS "U Up"
#define GAMEDEVICE_UNEG "U Down"
#define GAMEDEVICE_VPOS "V Up"
#define GAMEDEVICE_VNEG "V Down"
#define GAMEDEVICE_BUTTON "Button %d"

/* gaming general */
#define GAMEDEVICE_DISABLED "Disabled"

/* gaming keys */
#define GAMEDEVICE_KEY "#%d"
#define GAMEDEVICE_NUMPADPREFIX "Numpad-%c"
#define GAMEDEVICE_VK_TAB "Tab"
#define GAMEDEVICE_VK_BACK "Backspace"
#define GAMEDEVICE_VK_CLEAR "Delete"
#define GAMEDEVICE_VK_RETURN "Enter"
#define GAMEDEVICE_VK_LSHIFT "LShift"
#define GAMEDEVICE_VK_RSHIFT "RShift"
#define GAMEDEVICE_VK_LCONTROL "LCTRL"
#define GAMEDEVICE_VK_RCONTROL "RCTRL"
#define GAMEDEVICE_VK_LMENU "LAlt"
#define GAMEDEVICE_VK_RMENU "RAlt"
#define GAMEDEVICE_VK_PAUSE "Pause"
#define GAMEDEVICE_VK_CAPITAL "Capslock"
#define GAMEDEVICE_VK_ESCAPE "Disabled"
#define GAMEDEVICE_VK_SPACE "Space"
#define GAMEDEVICE_VK_PRIOR "PgUp"
#define GAMEDEVICE_VK_NEXT "PgDn"
#define GAMEDEVICE_VK_HOME "Home"
#define GAMEDEVICE_VK_END "End"
#define GAMEDEVICE_VK_LEFT "Left"
#define GAMEDEVICE_VK_RIGHT "Right"
#define GAMEDEVICE_VK_UP "Up"
#define GAMEDEVICE_VK_DOWN "Down"
#define GAMEDEVICE_VK_SELECT "Select"
#define GAMEDEVICE_VK_PRINT "Print"
#define GAMEDEVICE_VK_EXECUTE "Execute"
#define GAMEDEVICE_VK_SNAPSHOT "SnapShot"
#define GAMEDEVICE_VK_INSERT "Insert"
#define GAMEDEVICE_VK_DELETE "Delete"
#define GAMEDEVICE_VK_HELP "Help"
#define GAMEDEVICE_VK_LWIN "LWinKey"
#define GAMEDEVICE_VK_RWIN "RWinKey"
#define GAMEDEVICE_VK_APPS "AppKey"
#define GAMEDEVICE_VK_MULTIPLY "Numpad *"
#define GAMEDEVICE_VK_ADD "Numpad +"
#define GAMEDEVICE_VK_SEPARATOR "\\"
#define GAMEDEVICE_VK_OEM_1 "Semi-Colon"
#define GAMEDEVICE_VK_OEM_7 "Apostrophe"
#define GAMEDEVICE_VK_OEM_COMMA "Comma" 
#define GAMEDEVICE_VK_OEM_PERIOD "Period" 
#define GAMEDEVICE_VK_SUBTRACT "Numpad -"
#define GAMEDEVICE_VK_DECIMAL "Numpad ."
#define GAMEDEVICE_VK_DIVIDE "Numpad /"
#define GAMEDEVICE_VK_NUMLOCK "Num-lock"
#define GAMEDEVICE_VK_SCROLL "Scroll-lock"

/* evil things I found in WinProc */

#define WINPROC_TURBOMODE_ON "Turbo Mode Activated"
#define WINPROC_TURBOMODE_OFF "Turbo Mode Deactivated"
#define WINPROC_TURBOMODE_TEXT "Turbo Mode"
#define WINPROC_HDMA_TEXT "HDMA emulation"
#define WINPROC_BG1 "BG#1" /* Background Layers */
#define WINPROC_BG2 "BG#2"
#define WINPROC_BG3 "BG#3"
#define WINPROC_BG4 "BG#4"
#define WINPROC_SPRITES "Sprites"
#define WINPROC_PADSWAP "Joypad swapping" 
#define WINPROC_CONTROLERS0 "Multiplayer 5 on #0"
#define WINPROC_CONTROLERS1 "Joypad on #0"
#define WINPROC_CONTROLERS2 "Mouse on #1"
#define WINPROC_CONTROLERS3 "Mouse on #0"
#define WINPROC_CONTROLERS4 "Superscope on #1"
#define WINPROC_CONTROLERS5 "Justifier 1 on #1"
#define WINPROC_CONTROLERS6 "Justifier 2 on #1"
#define WINPROC_BGHACK "Background layering hack"
#define WINPROC_MODE7INTER "Mode 7 Interpolation"
#define WINPROC_TRANSPARENCY "Transparency effects"
#define WINPROC_CLIPWIN "Graphic clip windows"
#define WINPROC_PAUSE "Pause"
#define WINPROC_EMUFRAMETIME "Emulated frame time: %dms"
#define WINPROC_AUTOSKIP "Auto Frame Skip"
#define WINPROC_FRAMESKIP "Frame skip: %d"
#define WINPROC_TURBO_R_ON "Turbo R Activated"
#define WINPROC_TURBO_R_OFF "Turbo R Deactivated"
#define WINPROC_TURBO_L_ON "Turbo L Activated"
#define WINPROC_TURBO_L_OFF "Turbo L Deactivated"
#define WINPROC_TURBO_X_ON "Turbo X Activated"
#define WINPROC_TURBO_X_OFF "Turbo X Deactivated"
#define WINPROC_TURBO_Y_ON "Turbo Y Activated"
#define WINPROC_TURBO_Y_OFF "Turbo Y Deactivated"
#define WINPROC_TURBO_A_ON "Turbo A Activated"
#define WINPROC_TURBO_A_OFF "Turbo A Deactivated"
#define WINPROC_TURBO_B_ON "Turbo B Activated"
#define WINPROC_TURBO_B_OFF "Turbo B Deactivated"
#define WINPROC_TURBO_SEL_ON "Turbo Select Activated"
#define WINPROC_TURBO_SEL_OFF "Turbo Select Deactivated"
#define WINPROC_TURBO_START_ON "Turbo Start Activated"
#define WINPROC_TURBO_START_OFF "Turbo Start Deactivated"
#define WINPROC_FILTER_RESTART "You will need to restart Snes9x before the output image\nprocessing option change will take effect."
#define WINPROC_DISCONNECT "Disconnect from the NetPlay server first."
#define WINPROC_NET_RESTART "Your game will be reset after the ROM has been sent due to\nyour 'Sync Using Reset Game' setting.\n\n"
#define WINPROC_INTERPOLATED_SND "Interpolated sound"
#define WINPROC_SYNC_SND "Sync sound"
#define WINPROC_SND_OFF "Disabling the sound CPU emulation will help to improve\nemulation speed but you will not hear any sound effects\nor music. If you later want to re-enable the sound CPU\nemulation you will need to reset your game before it will\ntake effect.\n\nAre you sure this is what you want?"
#define WINPROC_SND_RESTART "You will need to reset your game or load another one\nbefore enabling the sound CPU will take effect."

/* Emulator Settings */

#define EMUSET_TITLE "Emulation Settings"
#define EMUSET_LABEL_FREEZE "Freeze Folder Directory"
#define EMUSET_BROWSE "&Browse..."
#define EMUSET_LABEL_ASRAM "Auto-Save S-RAM"
#define EMUSET_LABEL_ASRAM_TEXT "seconds after last change (0 disables auto-save)"
#define EMUSET_LABEL_SMAX "Skip at most"
#define EMUSET_LABEL_SMAX_TEXT "frames in auto-frame rate mode"
#define EMUSET_LABEL_STURBO "Skip Rendering"
#define EMUSET_LABEL_STURBO_TEXT "frames in Turbo mode"
#define EMUSET_TOGGLE_TURBO "Tab Toggles Turbo"

/* Netplay Options */

#define NPOPT_TITLE "Netplay Options"
#define NPOPT_LABEL_PORTNUM "Socket Port Number"
#define NPOPT_LABEL_PAUSEINTERVAL "Ask Server to Pause when"
#define NPOPT_LABEL_PAUSEINTERVAL_TEXT "frames behind"
#define NPOPT_LABEL_MAXSKIP "Maximum Frame Rate Skip"
#define NPOPT_SYNCBYRESET "Sync By Reset"
#define NPOPT_SENDROM "Send ROM Image to Client on Connect"
#define NPOPT_ACTASSERVER "Act As Server"
#define NPOPT_PORTNUMBLOCK "Port Settings"
#define NPOPT_CLIENTSETTINGSBLOCK "Client Settings"
#define NPOPT_SERVERSETTINGSBLOCK "Server Settings"

/* Netplay Connect */


#define NPCON_TITLE "Connect to Server"
#define NPCON_LABEL_SERVERADDY "Server Address"
#define NPCON_LABEL_PORTNUM "Port Number"
#define NPCON_CLEARHISTORY "Clear History"


/* Movie Messages */

#define MOVIE_INFO_REPLAY "Movie replay"
#define MOVIE_INFO_RECORD "Movie record"
#define MOVIE_INFO_RERECORD "Movie re-record"
#define MOVIE_INFO_REWIND "Movie rewind"
#define MOVIE_INFO_STOP "Movie stop"
#define MOVIE_INFO_END "Movie end"
#define MOVIE_INFO_RECORDING_ENABLED "Recording enabled"
#define MOVIE_INFO_RECORDING_DISABLED "Recording disabled"
#define MOVIE_ERR_SNAPSHOT_WRONG_MOVIE "Snapshot not from this movie"
#define MOVIE_ERR_SNAPSHOT_NOT_MOVIE "Not a movie snapshot"
#define MOVIE_ERR_COULD_NOT_OPEN "Could not open movie file."
#define MOVIE_ERR_NOT_FOUND "File not found."
#define MOVIE_ERR_WRONG_FORMAT "File is wrong format."
#define MOVIE_ERR_WRONG_VERSION "File is wrong version."


/*  AVI Messages */

#define AVI_CONFIGURATION_CHANGED "AVI recording stopped (configuration settings changed)."
