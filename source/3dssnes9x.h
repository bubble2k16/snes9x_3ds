

// Uncomment this to convert before releasing this to remove
// all the debugging stuff.
//
#define RELEASE 

// Uncomment this to allow user to break into debug mode (for the 65816 CPU)
// 
//#define DEBUG_CPU

// Uncomment this to allow user to break into debug mode (for the SPC700 APU)
//
//#define DEBUG_APU



#define DEBUG_WAIT_L_KEY 	\
    { \
        uint32 prevkey = 0; \
        while (aptMainLoop()) \ 
        {  \
            hidScanInput(); \ 
            uint32 key = hidKeysHeld(); \
            if (key == KEY_L) break; \
            if (key == KEY_TOUCH) break; \
            if (key == KEY_SELECT) { GPU3DS.enableDebug ^= 1; break; } \
            if (prevkey != 0 && key == 0) \
                break;  \
            prevkey = key; \
        } \ 
    }

#define CLEAR_BOTTOM_SCREEN \
    gfxSetDoubleBuffering(GFX_BOTTOM,false); \
    gfxSwapBuffers(); \
    consoleInit(GFX_BOTTOM, NULL); \
    consoleClear(); \
