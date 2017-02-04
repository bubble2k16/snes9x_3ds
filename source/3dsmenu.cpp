#include <cstring>
#include <string.h>
#include <stdio.h>
#include <3ds.h>

#include "snes9x.h"
#include "port.h"

#include "3dsexit.h"
#include "3dsmenu.h"
#include "3dsgpu.h"
#include "3dsui.h"

#define CONSOLE_WIDTH           40
#define MENU_HEIGHT             (14)
#define DIALOG_HEIGHT           (5)

#define SNES9X_VERSION "v1.00b"



typedef struct
{
    SMenuItem   *MenuItems;
    char        SubTitle[256];
    char        *Title;
    char        *DialogText;
    int         ItemCount;
    int         FirstItemIndex;
    int         SelectedItemIndex;
} SMenuTab;


SMenuTab            menuTab[4];
SMenuTab            dialogTab;

int                 menuTabCount;
int                 currentMenuTab = 0;
bool                isDialog = false;

bool                transferGameScreen = false;


//-------------------------------------------------------
// Sets a flag to tell the menu selector
// to transfer the emulator's rendered frame buffer
// to the actual screen's frame buffer.
//
// Usually you will set this to true during emulation,
// and set this to false when this program first runs.
//-------------------------------------------------------
void S9xSetTransferGameScreen(bool transfer)
{
    transferGameScreen = transfer;
}


void S9xSwapBuffers()
{
    gfxFlushBuffers();
    if (transferGameScreen)
        gpu3dsTransferToScreenBuffer();
    gfxSwapBuffers();
}

char *S9xMenuTruncateString(char *outBuffer, char *inBuffer)
{
    memset(outBuffer, 0, CONSOLE_WIDTH);
    if (strlen(inBuffer) < CONSOLE_WIDTH - 3)
        return inBuffer;

    for (int i = 0; i < CONSOLE_WIDTH - 3; i++)
    {
        outBuffer[i] = inBuffer[i];
        if (inBuffer[i] == 0)
            break;
    }

    return outBuffer;
}


void S9xShowTitleAndMessage(
    int titleForeColor, int titleBackColor,
    int mainForeColor, int mainBackColor,
    char *title, char *messageLine1, char *messageLine2, char *messageLine3, char *messageLine4)
{
    ui3dsSetColor(titleForeColor, titleBackColor);
    ui3dsDrawRect(0, 0, 320, 16);
    ui3dsDrawRect(0, 224, 320, 240);
    ui3dsDrawString(2, 2, 318, true, title);

    ui3dsSetColor(mainForeColor, mainBackColor);
    ui3dsDrawRect(0, 16, 320, 224);

    int line = 70;
    ui3dsDrawString(2, line, 318, true, messageLine1);
    ui3dsDrawString(2, line+12, 318, true, messageLine2);
    ui3dsDrawString(2, line+24, 318, true, messageLine3);
    ui3dsDrawString(2, line+36, 318, true, messageLine4);
}


// Draw a black screen.
//
void S9xDrawBlackScreen(float opacity)
{
    ui3dsDrawRect(0, 0, 320, 240, 0x000000, opacity);    
}


char menuTextBuffer[512];

void S9xDrawItems(
    SMenuTab *currentTab, int horizontalPadding, int menuStartY, int maxItems,
    int selectedItemBackColor,
    int selectedItemTextColor, 
    int checkedItemTextColor, 
    int normalItemTextColor,
    int disabledItemTextColor, 
    int headerItemTextColor, 
    int subtitleTextColor)
{
    int fontHeight = 13;
    char gauge[52];
    
    // Display the subtitle
    if (currentTab->SubTitle[0])
    {
        maxItems--;
        snprintf (menuTextBuffer, 511, "%s", currentTab->SubTitle);
        ui3dsDrawStringWithNoWrapping(20, menuStartY, 300, menuStartY + fontHeight, 
            subtitleTextColor, HALIGN_LEFT, menuTextBuffer);
        menuStartY += fontHeight;
    }

    int line = 0;
    int color = 0xffffff;
    for (int i = currentTab->FirstItemIndex;
        i < currentTab->ItemCount && i < currentTab->FirstItemIndex + maxItems; i++)
    {
        int y = line * fontHeight + menuStartY;

        if (currentTab->SelectedItemIndex == i)
        {
            color = selectedItemTextColor;
            ui3dsDrawRect(0, y, 320, y + 14, selectedItemBackColor);
        }
        else if (currentTab->MenuItems[i].ID == -1)
            color = headerItemTextColor;
        else if (currentTab->MenuItems[i].ID < -1)
            color = disabledItemTextColor;
        else if (currentTab->MenuItems[i].Checked == 1)
            color = checkedItemTextColor;
        else if (currentTab->MenuItems[i].Checked == 0)
            color = disabledItemTextColor;
        else
            color = normalItemTextColor;

        if (currentTab->MenuItems[i].Text == NULL)
            menuTextBuffer[0] = 0;
        else
            snprintf(menuTextBuffer, 512, "%s", currentTab->MenuItems[i].Text);
        ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + 14, color, HALIGN_LEFT, menuTextBuffer);

        if (currentTab->MenuItems[i].Checked == 0)
            ui3dsDrawStringWithNoWrapping(280, y, 320 - horizontalPadding, y + 14, color, HALIGN_LEFT, "\xfe");
        else if (currentTab->MenuItems[i].Checked == 1)
            ui3dsDrawStringWithNoWrapping(280, y, 320 - horizontalPadding, y + 14, color, HALIGN_LEFT, "\xfd");
        else
        {
            if (currentTab->MenuItems[i].GaugeMinValue < currentTab->MenuItems[i].GaugeMaxValue)
            {
                int max = 40;
                int diff = currentTab->MenuItems[i].GaugeMaxValue - currentTab->MenuItems[i].GaugeMinValue;
                int pos = (currentTab->MenuItems[i].GaugeValue - currentTab->MenuItems[i].GaugeMinValue) * (max - 1) / diff;

                for (int j = 0; j < max; j++)
                    gauge[j] = (j == pos) ? '\xfa' : '\xfb';
                gauge[max] = 0;
                ui3dsDrawStringWithNoWrapping(245, y, 320 - horizontalPadding, y + 14, color, HALIGN_LEFT, gauge);
            }
            else
                ui3dsDrawStringWithNoWrapping(245, y, 320 - horizontalPadding, y + 14, color, HALIGN_LEFT, "");
        }
        line ++;
    }
}

// Display the list of choices for selection
//
void S9xDrawMenu()
{
    SMenuTab *currentTab = &menuTab[currentMenuTab];

    char tempBuffer[CONSOLE_WIDTH];

    // Draw the flat background
    //
    ui3dsDrawRect(0, 0, 320, 24, 0x1976D2);
    ui3dsDrawRect(0, 24, 320, 220, 0xFFFFFF);
    ui3dsDrawRect(0, 220, 320, 240, 0x1976D2);

    // Draw the tabs at the top
    //
    for (int i = 0; i < menuTabCount; i++)
    {
        int color = i == currentMenuTab ? 0xFFFFFF : 0x90CAF9 ;
        ui3dsDrawStringWithNoWrapping(i * 75 + 10, 6, (i+1)*75 + 10, 21, color, HALIGN_CENTER, 
            menuTab[i].Title);

        if (i == currentMenuTab)
            ui3dsDrawRect(i * 75 + 10, 21, (i+1)*75 + 10, 24, 0xFFFFFF);
    }

    // Shadows
    //ui3dsDrawRect(0, 23, 320, 24, 0xaaaaaa);
    //ui3dsDrawRect(0, 24, 320, 25, 0xcccccc);
    //ui3dsDrawRect(0, 25, 320, 27, 0xeeeeee);

    ui3dsDrawStringWithNoWrapping(10, 224, 310, 240, 0x90CAF9, HALIGN_LEFT,
        "A:Select  B:Cancel");
    ui3dsDrawStringWithNoWrapping(10, 224, 310, 240, 0x90CAF9, HALIGN_RIGHT,
        SNES9X_VERSION);

    int line = 0;
    int maxItems = MENU_HEIGHT;
    int menuStartY = 29;

    S9xDrawItems(
        currentTab, 20, menuStartY, maxItems,
        0x333333,       // selectedItemBackColor
        0xffffff,       // selectedItemTextColor
        0x000000,       // checkedItemTextColor
        0x333333,       // normalItemTextColor      
        0x888888,       // disabledItemTextColor
        0x1E88E5,       // headerItemTextColor
        0x1E88E5);      // subtitleTextColor
      
/*
    ui3dsDrawStringWithWrapping(10, 10, 100, 70, 0xff0000, HALIGN_LEFT, "This is a long text that should wrap to a few lines!");
    ui3dsDrawStringWithWrapping(10, 90, 100, 150, 0xff0000, HALIGN_RIGHT, "This is a long text that should wrap and right justify itself!");
    ui3dsDrawStringWithWrapping(10, 170, 100, 230, 0xff0000, HALIGN_CENTER, "This is a long text that should wrap and center justify itself!");
    ui3dsDrawStringWithNoWrapping(110, 10, 200, 70, 0xff0000, HALIGN_LEFT, "This is a long text will be truncated");
    ui3dsDrawStringWithNoWrapping(110, 90, 200, 150, 0xff0000, HALIGN_CENTER, "This is a long text will be truncated");
    ui3dsDrawStringWithNoWrapping(110, 170, 200, 230, 0xff0000, HALIGN_RIGHT, "This is a long text will be truncated");
*/
}




int dialogBackColor = 0xEC407A;
int dialogTextColor = 0xffffff;
int dialogItemTextColor = 0xffffff;
int dialogSelectedItemTextColor = 0xffffff;
int dialogSelectedItemBackColor = 0x000000;

void S9xDrawDialog()
{
    // Dialog's Background
    int dialogBackColor2 = ui3dsApplyAlphaToColor(dialogBackColor, 0.9f);
    ui3dsDrawRect(0, 80, 320, 155, dialogBackColor2);
    ui3dsDrawRect(0, 155, 320, 240, dialogBackColor);

    int dialogTitleTextColor = 
        ui3dsApplyAlphaToColor(dialogBackColor, 0.3f);

    ui3dsDrawStringWithNoWrapping(30, 90, 290, 105, dialogTitleTextColor, HALIGN_LEFT, dialogTab.Title);

    ui3dsDrawStringWithWrapping(30, 105, 290, 150, dialogTextColor, HALIGN_LEFT, dialogTab.DialogText);

    S9xDrawItems(
        &dialogTab, 30, 160, DIALOG_HEIGHT,
        dialogSelectedItemBackColor,
        dialogSelectedItemTextColor, 
        dialogItemTextColor, 
        dialogItemTextColor,
        dialogItemTextColor, 
        dialogItemTextColor, 
        dialogItemTextColor);
}


void S9xDrawEverythingAndSwapBuffers(int dialogFrame = 0)
{
    int y = 240;

    if (isDialog)
    {
        y = 240 - (8 - dialogFrame) * 20;
    }

    ui3dsSetTranslate(0, 0);

    ui3dsSetViewport(0, 0, 320, y);
    S9xDrawMenu();

    if (isDialog)
    {
        ui3dsSetViewport(0, 0, 320, 240);
        ui3dsDrawRect(0, 0, 320, y, 0x000000, 
            (float)(8 - dialogFrame) / 10);    
        
        ui3dsSetTranslate(0, dialogFrame * 20);
        S9xDrawDialog();
        ui3dsSetTranslate(0, 0);
    }
    S9xSwapBuffers();
}


// Displays the menu and allows the user to select from
// a list of choices.
//
int S9xMenuSelectItem()
{
    int framesDKeyHeld = 0;
    int returnResult = -1;

    SMenuTab *currentTab = &menuTab[currentMenuTab];

    if (isDialog)
        currentTab = &dialogTab;

    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;

    for (int i = 0; i < 2; i ++)
    {
        aptMainLoop();
        S9xDrawEverythingAndSwapBuffers();
        gspWaitForVBlank();

        hidScanInput();
        lastKeysHeld = hidKeysHeld();
    }

    while (aptMainLoop())
    {   
        if (appExiting)
        {
            returnResult = -1;
            break;
        }

        hidScanInput();
        thisKeysHeld = hidKeysHeld();

        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        int maxItems = MENU_HEIGHT;
        if (isDialog)
            maxItems = DIALOG_HEIGHT;

        if (currentTab->SubTitle[0])
        {
            maxItems--;
        }

        if (thisKeysHeld & KEY_UP || thisKeysHeld & KEY_DOWN)
            framesDKeyHeld ++;
        else
            framesDKeyHeld = 0;
        if (keysDown & KEY_B)
        {
            returnResult = -1;
            break;
        }
        if ((keysDown & KEY_RIGHT) || (keysDown & KEY_R))
        {
            if (!isDialog)
            {
                if (keysDown & KEY_RIGHT)
                {
                    // Gauge adjustment
                    if (currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMinValue <
                        currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMaxValue)
                    {
                        if (currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeValue <
                            currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMaxValue)
                        {
                            currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeValue ++ ;
                        }
                        returnResult = currentTab->MenuItems[currentTab->SelectedItemIndex].ID;
                        break;    
                    }
                }

                currentMenuTab++;
                if (currentMenuTab >= menuTabCount)
                    currentMenuTab = 0;
                currentTab = &menuTab[currentMenuTab];

                S9xDrawEverythingAndSwapBuffers();
            }
        }
        if ((keysDown & KEY_LEFT) || (keysDown & KEY_L))
        {
            if (!isDialog)
            {
                if (keysDown & KEY_LEFT)
                {
                    // Gauge adjustment
                    if (currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMinValue <
                        currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMaxValue)
                    {
                        if (currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeValue >
                            currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMinValue)
                        {
                            currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeValue -- ;
                        }
                        returnResult = currentTab->MenuItems[currentTab->SelectedItemIndex].ID;
                        break;
                    }
                }

                currentMenuTab--;
                if (currentMenuTab < 0)
                    currentMenuTab = menuTabCount - 1;
                currentTab = &menuTab[currentMenuTab];

                S9xDrawEverythingAndSwapBuffers();
            }
        }
        if (keysDown & KEY_Y)
        {
            // Gauge adjustment
            if (currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMinValue <
                currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMaxValue)
            {
                if (currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeValue >
                    currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMinValue)
                {
                    currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeValue -- ;
                }
                returnResult = currentTab->MenuItems[currentTab->SelectedItemIndex].ID;
                break;
            }

            // Bug fix: Do not return if this is not a gauge
            //return currentTab->MenuItems[currentTab->SelectedItemIndex].ID;
            S9xDrawEverythingAndSwapBuffers();
        }

        if (keysDown & KEY_START || keysDown & KEY_A)
        {
            // Gauge adjustment
            if ((keysDown & KEY_A) &&
                currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMinValue <
                currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMaxValue)
            {
                if (currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeValue <
                    currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMaxValue)
                {
                    currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeValue ++ ;
                }
                returnResult = currentTab->MenuItems[currentTab->SelectedItemIndex].ID;
                break;
            }

            returnResult = currentTab->MenuItems[currentTab->SelectedItemIndex].ID;
            break;
        }
        if (keysDown & KEY_UP || ((thisKeysHeld & KEY_UP) && (framesDKeyHeld > 30) && (framesDKeyHeld % 2 == 0)))
        {
            int moveCursorTimes = 0;

            do
            {
                if (thisKeysHeld & KEY_X)
                {
                    currentTab->SelectedItemIndex -= 15;
                    if (currentTab->SelectedItemIndex < 0)
                        currentTab->SelectedItemIndex = 0;
                }
                else
                {
                    currentTab->SelectedItemIndex--;
                    if (currentTab->SelectedItemIndex < 0)
                    {
                        currentTab->SelectedItemIndex = currentTab->ItemCount - 1;
                    }
                }
                moveCursorTimes++;
            }
            while (currentTab->MenuItems[currentTab->SelectedItemIndex].ID <= -1 &&
                moveCursorTimes < currentTab->ItemCount);

            if (currentTab->SelectedItemIndex < currentTab->FirstItemIndex)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + maxItems)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - maxItems + 1;

            S9xDrawEverythingAndSwapBuffers();

        }
        if (keysDown & KEY_DOWN || ((thisKeysHeld & KEY_DOWN) && (framesDKeyHeld > 30) && (framesDKeyHeld % 2 == 0)))
        {
            int moveCursorTimes = 0;
            do
            {
                if (thisKeysHeld & KEY_X)
                {
                    currentTab->SelectedItemIndex += 15;
                    if (currentTab->SelectedItemIndex >= currentTab->ItemCount)
                        currentTab->SelectedItemIndex = currentTab->ItemCount - 1;
                }
                else
                {
                    currentTab->SelectedItemIndex++;
                    if (currentTab->SelectedItemIndex >= currentTab->ItemCount)
                    {
                        currentTab->SelectedItemIndex = 0;
                        currentTab->FirstItemIndex = 0;
                    }
                }
                moveCursorTimes++;
            }
            while (currentTab->MenuItems[currentTab->SelectedItemIndex].ID <= -1 &&
                moveCursorTimes < currentTab->ItemCount);

            if (currentTab->SelectedItemIndex < currentTab->FirstItemIndex)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + maxItems)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - maxItems + 1;

            S9xDrawEverythingAndSwapBuffers();
        }

        gspWaitForVBlank();
    }

    return returnResult;
    
}


void S9xAddTab(char *title, SMenuItem *menuItems, int itemCount)
{
    SMenuTab *currentTab = &menuTab[menuTabCount];

    currentTab->Title = title;
    currentTab->MenuItems = menuItems;
    currentTab->ItemCount = itemCount;

    currentTab->FirstItemIndex = 0;
    currentTab->SelectedItemIndex = 0;
    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID > -1)
        {
            currentTab->SelectedItemIndex = i;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + MENU_HEIGHT)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - MENU_HEIGHT + 1;
            break;
        }
    }

    menuTabCount++;
}



void S9xSetTabSubTitle(int tabIndex, char *subtitle)
{
    SMenuTab *currentTab = &menuTab[tabIndex];

    currentTab->SubTitle[0] = 0;
    if (subtitle != NULL)
        strncpy(currentTab->SubTitle, subtitle, 255);
}

void S9xSetCurrentMenuTab(int tabIndex)
{
    currentMenuTab = tabIndex;
}


void S9xSetSelectedItemIndexByID(int tabIndex, int ID)
{
    currentMenuTab = tabIndex;

    SMenuTab *currentTab = &menuTab[tabIndex];

    int maxItems = MENU_HEIGHT;
    if (currentTab->SubTitle[0])
        maxItems--;

    for (int i = 0; i < currentTab->ItemCount; i++)
    {
        if (currentTab->MenuItems[i].ID == ID)
        {
            currentTab->SelectedItemIndex = i;

            if (currentTab->SelectedItemIndex < currentTab->FirstItemIndex)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + maxItems)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - maxItems + 1;

            break;
        }
    }
}


void S9xClearMenuTabs()
{
    menuTabCount = 0;
    currentMenuTab = 0;
}


int S9xShowDialog(char *title, char *dialogText, int newDialogBackColor, SMenuItem *menuItems, int itemCount)
{
    SMenuTab *currentTab = &dialogTab;

    dialogBackColor = newDialogBackColor;

    currentTab->Title = title;
    currentTab->DialogText = dialogText;
    currentTab->MenuItems = menuItems;
    currentTab->ItemCount = itemCount;

    currentTab->FirstItemIndex = 0;
    currentTab->SelectedItemIndex = 0;

    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID > -1)
        {
            currentTab->SelectedItemIndex = i;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + DIALOG_HEIGHT)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - DIALOG_HEIGHT + 1;
            break;
        }
    }

    // fade the dialog fade in
    //
    aptMainLoop();
    S9xDrawEverythingAndSwapBuffers();
    gspWaitForVBlank();  

    isDialog = true;
    for (int f = 8; f >= 0; f--)
    {
        aptMainLoop();
        S9xDrawEverythingAndSwapBuffers(f);    
        gspWaitForVBlank();  
    }

    // Execute the dialog and return result.
    //
    if (itemCount > 0)
    {
        int result = S9xMenuSelectItem();

        return result;
    }
    return 0;
}


void S9xHideDialog()
{
    // fade the dialog out
    //
    for (int f = 0; f <= 8; f++)
    {
        aptMainLoop();
        S9xDrawEverythingAndSwapBuffers(f);        
        gspWaitForVBlank();    
    }
    isDialog = false;
    
}


void S9xShowWaitingMessage(char *title, char *messageLine1, char *messageLine2)
{
    S9xShowTitleAndMessage(
        0xffffff, 0x2196F3,
        0x333333, 0xffffff,
        title, messageLine1, messageLine2, "", "");
}


void S9xAlertSuccess(char *title, char *messageLine1, char *messageLine2)
{
    S9xShowTitleAndMessage(
        0xffffff, 0x43A047,
        0x333333, 0xffffff,
        title, messageLine1, messageLine2, "", "A - OK");

    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;

    while (aptMainLoop())
    {
        hidScanInput();
        thisKeysHeld = hidKeysHeld();
        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        if (keysDown & KEY_A)
        {
            return;
        }
        gspWaitForVBlank();

    }
}


void S9xAlertFailure(char *title, char *messageLine1, char *messageLine2)
{
    S9xShowTitleAndMessage(
        0xffffff, 0xC62828,
        0x333333, 0xffffff,
        title, messageLine1, messageLine2, "", "A - OK");

    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;

    while (aptMainLoop())
    {
        hidScanInput();
        thisKeysHeld = hidKeysHeld();
        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        if (keysDown & KEY_A)
        {
            return;
        }
        gspWaitForVBlank();

    }
}


bool S9xConfirm(char *title, char *messageLine1, char *messageLine2)
{
    S9xShowTitleAndMessage(
        0xffffff, 0x00897B,
        0x333333, 0xffffff,
        title, messageLine1, messageLine2, "", "START - Yes      B - No");


    u32 lastKeysHeld = 0xffffff;
    u32 thisKeysHeld = 0;


    while (aptMainLoop())
    {
        hidScanInput();
        thisKeysHeld = hidKeysHeld();
        u32 keysDown = (~lastKeysHeld) & thisKeysHeld;
        lastKeysHeld = thisKeysHeld;

        if (keysDown & KEY_START)
        {
            return true;
        }
        if (keysDown & KEY_B)
        {
            return false;
        }
        gspWaitForVBlank();

    }
    return false;

}


void S9xUncheckGroup(SMenuItem *menuItems, int itemCount, int group)
{
    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID / 1000 == group / 1000)
        {
            menuItems[i].Checked = 0;
        }

    }
}


void S9xCheckItemByID(SMenuItem *menuItems, int itemCount, int id)
{
    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID == id)
        {
            menuItems[i].Checked = 1;
            break;
        }

    }
}

void S9xSetCheckItemByID(SMenuItem *menuItems, int itemCount, int id, int value)
{
    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID == id)
        {
            menuItems[i].Checked = value;
            break;
        }
    }
}

void S9xSetGaugeValueItemByID(SMenuItem *menuItems, int itemCount, int id, int value, char *text)
{
    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID == id)
        {
            if (value < menuItems[i].GaugeMinValue)
                value = menuItems[i].GaugeMinValue;
            if (value > menuItems[i].GaugeMaxValue)
                value = menuItems[i].GaugeMaxValue;
            menuItems[i].GaugeValue = value;

            if (text != NULL)
                menuItems[i].Text = text;
            break;
        }

    }
}

int S9xGetGaugeValueItemByID(SMenuItem *menuItems, int itemCount, int id)
{
    for (int i = 0; i < itemCount; i++)
    {
        if (menuItems[i].ID == id)
        {
            return menuItems[i].GaugeValue;
            break;
        }

    }
    return 0;
}


bool S9xTakeScreenshot(char* path)
{
    int x, y;

    FILE *pFile = fopen(path, "wb");
    if (pFile == NULL) return false;

    // Modified this to take only the top screen
    //
    u32 bitmapsize = 400*240*2;
    u8* tempbuf = (u8*)linearAlloc(0x8A + 400*240*2);
    memset(tempbuf, 0, 0x8A + bitmapsize);

    *(u16*)&tempbuf[0x0] = 0x4D42;
    *(u32*)&tempbuf[0x2] = 0x8A + bitmapsize;
    *(u32*)&tempbuf[0xA] = 0x8A;
    *(u32*)&tempbuf[0xE] = 0x28;
    *(u32*)&tempbuf[0x12] = 400;
    *(u32*)&tempbuf[0x16] = 240;
    *(u32*)&tempbuf[0x1A] = 0x1;
    *(u32*)&tempbuf[0x1C] = 0x10;
    *(u32*)&tempbuf[0x1E] = 0x3;
    *(u32*)&tempbuf[0x22] = bitmapsize;
    *(u32*)&tempbuf[0x36] = 0x0000F800;
    *(u32*)&tempbuf[0x3A] = 0x000007E0;
    *(u32*)&tempbuf[0x3E] = 0x0000001F;
    *(u32*)&tempbuf[0x42] = 0x00000000;

    u8* framebuf = (u8*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    for (y = 0; y < 240; y++)
    {
        for (x = 0; x < 400; x++)
        {
            int si = 1 + (((239 - y) + (x * 240)) * 4);
            int di = 0x8A + (x + ((239 - y) * 400)) * 2;

            u16 word = RGB8_to_565(framebuf[si++], framebuf[si++], framebuf[si++]);
            tempbuf[di++] = word & 0xFF;
            tempbuf[di++] = word >> 8;
        }
    }

    /*
    framebuf = (u8*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    for (y = 0; y < 240; y++)
    {
        for (x = 0; x < 320; x++)
        {
            int si = ((239 - y) + (x * 240)) * 2;
            int di = 0x8A + ((x+40) + ((239 - y) * 400)) * 2;

            tempbuf[di++] = framebuf[si++];
            tempbuf[di++] = framebuf[si++];
        }
    }
    */

    fwrite(tempbuf, sizeof(char), 0x8A + bitmapsize, pFile);
    fclose(pFile);

    linearFree(tempbuf);
    return true;
}

