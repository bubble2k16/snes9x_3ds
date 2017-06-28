#include <array>
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

#define SNES9X_VERSION "v1.20"
#define ANIMATE_TAB_STEPS 3


SMenuTab            dialogTab;
int menuTabCount;

bool                transferGameScreen = false;
int                 transferGameScreenCount = 0;

bool                swapBuffer = true;


//-------------------------------------------------------
// Sets a flag to tell the menu selector
// to transfer the emulator's rendered frame buffer
// to the actual screen's frame buffer.
//
// Usually you will set this to true during emulation,
// and set this to false when this program first runs.
//-------------------------------------------------------
void menu3dsSetTransferGameScreen(bool transfer)
{
    transferGameScreen = transfer;
    if (transfer)
        transferGameScreenCount = 2;
    else
        transferGameScreenCount = 0;

}



// Draw a black screen.
//
void menu3dsDrawBlackScreen(float opacity)
{
    ui3dsDrawRect(0, 0, 320, 240, 0x000000, opacity);    
}



void menu3dsSwapBuffersAndWaitForVBlank()
{
    if (transferGameScreenCount)
    {
        gpu3dsTransferToScreenBuffer();
        transferGameScreenCount --;
    }
    if (swapBuffer)
    {
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    else
    {
        gspWaitForVBlank();
    }
    
    swapBuffer = false;
}


void menu3dsDrawItems(
    SMenuTab *currentTab, int horizontalPadding, int menuStartY, int maxItems,
    int selectedItemBackColor,
    int selectedItemTextColor, 
    int selectedItemDescriptionTextColor, 
    int checkedItemTextColor, 
    int normalItemTextColor,
    int normalItemDescriptionTextColor,
    int disabledItemTextColor, 
    int headerItemTextColor, 
    int subtitleTextColor)
{
    int fontHeight = 13;
    char gauge[52];
    char menuTextBuffer[512];
    char selectedTextBuffer[512];
    
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

    // Draw all the individual items
    //
    for (int i = currentTab->FirstItemIndex;
        i < currentTab->ItemCount && i < currentTab->FirstItemIndex + maxItems; i++)
    {
        int y = line * fontHeight + menuStartY;

        if (currentTab->MenuItems[i].Text == NULL)
            snprintf (menuTextBuffer, 511, "");
        else
            snprintf (menuTextBuffer, 511, "%s", currentTab->MenuItems[i].Text);

        // Draw the selected background 
        //
        if (currentTab->SelectedItemIndex == i)
        {
            ui3dsDrawRect(0, y, 320, y + 14, selectedItemBackColor);
        }
        
        if (currentTab->MenuItems[i].Type == MENUITEM_HEADER1)
        {
            color = headerItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);
            ui3dsDrawRect(horizontalPadding, y + fontHeight - 1, 320 - horizontalPadding, y + fontHeight, color);
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_HEADER2)
        {
            color = headerItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_DISABLED)
        {
            color = disabledItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_ACTION)
        {
            color = normalItemTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);

            color = normalItemDescriptionTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemDescriptionTextColor;
            if (currentTab->MenuItems[i].Description != NULL)
            {
                snprintf(menuTextBuffer, 511, "%s", currentTab->MenuItems[i].Description);
                ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, menuTextBuffer);
            }
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_CHECKBOX)
        {
            if (currentTab->MenuItems[i].Value == 0)
            {
                color = disabledItemTextColor;
                if (currentTab->SelectedItemIndex == i)
                    color = selectedItemTextColor;
                ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);

                snprintf(menuTextBuffer, 511, "\xfe");
                ui3dsDrawStringWithNoWrapping(280, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, menuTextBuffer);
            }
            else
            {
                color = normalItemTextColor;
                if (currentTab->SelectedItemIndex == i)
                    color = selectedItemTextColor;
                ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);

                snprintf(menuTextBuffer, 511, "\xfd");
                ui3dsDrawStringWithNoWrapping(280, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, menuTextBuffer);
            }
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_GAUGE)
        {
            color = normalItemTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemTextColor;

            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);

            int max = 40;
            int diff = currentTab->MenuItems[i].GaugeMaxValue - currentTab->MenuItems[i].GaugeMinValue;
            int pos = (currentTab->MenuItems[i].Value - currentTab->MenuItems[i].GaugeMinValue) * (max - 1) / diff;

            for (int j = 0; j < max; j++)
                gauge[j] = (j == pos) ? '\xfa' : '\xfb';
            gauge[max] = 0;
            ui3dsDrawStringWithNoWrapping(245, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, gauge);
        }
        else if (currentTab->MenuItems[i].Type == MENUITEM_PICKER)
        {
            color = normalItemTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemTextColor;

            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 160, y + fontHeight, color, HALIGN_LEFT, menuTextBuffer);

            snprintf(selectedTextBuffer, 511, "");
            if (currentTab->MenuItems[i].PickerItems != NULL)
            {
                for (int j = 0; j < currentTab->MenuItems[i].PickerItemCount; j++)
                {
                    SMenuItem *pickerItems = (SMenuItem *)currentTab->MenuItems[i].PickerItems;
                    if (pickerItems[j].ID == currentTab->MenuItems[i].Value)
                    {
                        snprintf(selectedTextBuffer, 511, "%s", pickerItems[j].Text);
                    }
                }
                ui3dsDrawStringWithNoWrapping(160, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, selectedTextBuffer);
            }
        }

        line ++;
    }


    // Draw the "up arrow" to indicate more options available at top
    //
    if (currentTab->FirstItemIndex != 0)
    {
        ui3dsDrawStringWithNoWrapping(320 - horizontalPadding, menuStartY, 320, menuStartY + fontHeight, disabledItemTextColor, HALIGN_CENTER, "\xf8");
    }

    // Draw the "down arrow" to indicate more options available at bottom
    //
    if (currentTab->FirstItemIndex + maxItems < currentTab->ItemCount)
    {
        ui3dsDrawStringWithNoWrapping(320 - horizontalPadding, menuStartY + (maxItems - 1) * fontHeight, 320, menuStartY + maxItems * fontHeight, disabledItemTextColor, HALIGN_CENTER, "\xf9");
    }
    
}

// Display the list of choices for selection
//
void menu3dsDrawMenu(std::vector<SMenuTab>& menuTab, int& currentMenuTab, int menuItemFrame, int translateY)
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
    for (int i = 0; i < static_cast<int>(menuTab.size()); i++)
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

    ui3dsDrawStringWithNoWrapping(10, 223, 310, 240, 0xFFFFFF, HALIGN_LEFT,
        "A:Select  B:Cancel");
    ui3dsDrawStringWithNoWrapping(10, 223, 310, 240, 0xFFFFFF, HALIGN_RIGHT,
        "SNES9x for 3DS " SNES9X_VERSION);

    int line = 0;
    int maxItems = MENU_HEIGHT;
    int menuStartY = 29;

    ui3dsSetTranslate(menuItemFrame * 3, translateY);

    if (menuItemFrame == 0)
    {
        menu3dsDrawItems(
            currentTab, 20, menuStartY, maxItems,
            0x333333,       // selectedItemBackColor
            0xffffff,       // selectedItemTextColor
            0x777777,       // selectedItemDescriptionTextColor

            0x000000,       // checkedItemTextColor
            0x333333,       // normalItemTextColor      
            0x777777,       // normalItemDescriptionTextColor      
            0x888888,       // disabledItemTextColor
            0x1E88E5,       // headerItemTextColor
            0x1E88E5);      // subtitleTextColor
    }
    else
    {
        if (menuItemFrame < 0)
            menuItemFrame = -menuItemFrame;
        float alpha = (float)(ANIMATE_TAB_STEPS - menuItemFrame + 1) / (ANIMATE_TAB_STEPS + 1);

        int white = ui3dsApplyAlphaToColor(0xFFFFFF, 1.0f - alpha);
        
         menu3dsDrawItems(
            currentTab, 20, menuStartY, maxItems,
            ui3dsApplyAlphaToColor(0x333333, alpha) + white,
            ui3dsApplyAlphaToColor(0xffffff, alpha) + white,       // selectedItemTextColor
            ui3dsApplyAlphaToColor(0x777777, alpha) + white,       // selectedItemDescriptionTextColor

            ui3dsApplyAlphaToColor(0x000000, alpha) + white,       // checkedItemTextColor
            ui3dsApplyAlphaToColor(0x333333, alpha) + white,       // normalItemTextColor      
            ui3dsApplyAlphaToColor(0x777777, alpha) + white,       // normalItemDescriptionTextColor      
            ui3dsApplyAlphaToColor(0x888888, alpha) + white,       // disabledItemTextColor
            ui3dsApplyAlphaToColor(0x1E88E5, alpha) + white,       // headerItemTextColor
            ui3dsApplyAlphaToColor(0x1E88E5, alpha) + white);      // subtitleTextColor       
        //svcSleepThread((long)(1000000.0f * 1000.0f));
    }

      
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

void menu3dsDrawDialog()
{
    // Dialog's Background
    int dialogBackColor2 = ui3dsApplyAlphaToColor(dialogBackColor, 0.9f);
    ui3dsDrawRect(0, 0, 320, 75, dialogBackColor2);
    ui3dsDrawRect(0, 75, 320, 160, dialogBackColor);

    // Left trim the dialog title
    int len = strlen(dialogTab.Title);
    int startChar = 0;
    for (int i = 0; i < len; i++)
        if (dialogTab.Title[i] != ' ')
        {
            startChar = i;
            break;
        }

    // Draw the dialog's title and descriptive text
    int dialogTitleTextColor = 
        ui3dsApplyAlphaToColor(dialogBackColor, 0.5f) + 
        ui3dsApplyAlphaToColor(dialogTextColor, 0.5f);
    ui3dsDrawStringWithNoWrapping(30, 10, 290, 25, dialogTitleTextColor, HALIGN_LEFT, &dialogTab.Title[startChar]);
    ui3dsDrawStringWithWrapping(30, 30, 290, 70, dialogTextColor, HALIGN_LEFT, dialogTab.DialogText);

    // Draw the selectable items.
    int dialogItemDescriptionTextColor = dialogTitleTextColor;
    menu3dsDrawItems(
        &dialogTab, 30, 80, DIALOG_HEIGHT,
        dialogSelectedItemBackColor,        // selectedItemBackColor
        dialogSelectedItemTextColor,        // selectedItemTextColor
        dialogItemDescriptionTextColor,     // selectedItemDescriptionColor

        dialogItemTextColor,                // checkedItemTextColor
        dialogItemTextColor,                // normalItemTextColor
        dialogItemDescriptionTextColor,     // normalItemDescriptionTextColor
        dialogItemDescriptionTextColor,     // disabledItemTextColor
        dialogItemTextColor,                // headerItemTextColor
        dialogItemTextColor                 // subtitleTextColor
        );
}


void menu3dsDrawEverything(bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, int menuFrame = 0, int menuItemsFrame = 0, int dialogFrame = 0)
{
    if (!isDialog)
    {
        int y = 0 + menuFrame * menuFrame * 120 / 32;

        ui3dsSetViewport(0, 0, 320, 240);
        ui3dsSetTranslate(0, 0);
        ui3dsDrawRect(0, 0, 400, y, 0x000000);
        ui3dsSetTranslate(0, y);
        menu3dsDrawMenu(menuTab, currentMenuTab, menuItemsFrame, y);
    }
    else
    {
        int y = 80 + dialogFrame * dialogFrame * 80 / 32;

        ui3dsSetViewport(0, 0, 320, y);
        //ui3dsBlitToFrameBuffer(savedBuffer, 1.0f - (float)(8 - dialogFrame) / 10);
        ui3dsSetTranslate(0, 0);
        menu3dsDrawMenu(menuTab, currentMenuTab, 0, 0);
        ui3dsDrawRect(0, 0, 320, y, 0x000000, (float)(8 - dialogFrame) / 10);

        ui3dsSetViewport(0, 0, 320, 240);
        ui3dsSetTranslate(0, y);
        menu3dsDrawDialog();
        ui3dsSetTranslate(0, 0);
    }
    swapBuffer = true;
}


SMenuTab *menu3dsAnimateTab(bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, int direction)
{
    SMenuTab *currentTab = &menuTab[currentMenuTab];

    if (direction < 0)
    {
        for (int i = 1; i <= ANIMATE_TAB_STEPS; i++)
        {
            aptMainLoop();
            menu3dsDrawEverything(isDialog, currentMenuTab, menuTab, 0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }

        currentMenuTab--;
        if (currentMenuTab < 0)
            currentMenuTab = static_cast<int>(menuTab.size() - 1);
        currentTab = &menuTab[currentMenuTab];
        
        for (int i = -ANIMATE_TAB_STEPS; i <= 0; i++)
        {
            aptMainLoop();
            menu3dsDrawEverything(isDialog, currentMenuTab, menuTab, 0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }
    }
    else if (direction > 0)
    {
        for (int i = -1; i >= -ANIMATE_TAB_STEPS; i--)
        {
            aptMainLoop();
            menu3dsDrawEverything(isDialog, currentMenuTab, menuTab, 0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }

        currentMenuTab++;
        if (currentMenuTab >= static_cast<int>(menuTab.size()))
            currentMenuTab = 0;
        currentTab = &menuTab[currentMenuTab];
        
        for (int i = ANIMATE_TAB_STEPS; i >= 0; i--)
        {
            aptMainLoop();
            menu3dsDrawEverything(isDialog, currentMenuTab, menuTab, 0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }
    }
    return currentTab;
}


static u32 lastKeysHeld = 0xffffff;
static u32 thisKeysHeld = 0;


// Displays the menu and allows the user to select from
// a list of choices.
//
int menu3dsMenuSelectItem(bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, void (*itemChangedCallback)(int ID, int value))
{
    int framesDKeyHeld = 0;
    int returnResult = -1;

    char menuTextBuffer[512];

    SMenuTab *currentTab = &menuTab[currentMenuTab];

    if (isDialog)
        currentTab = &dialogTab;

    for (int i = 0; i < 2; i ++)
    {
        aptMainLoop();
        menu3dsDrawEverything(isDialog, currentMenuTab, menuTab);
        menu3dsSwapBuffersAndWaitForVBlank();

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

        gpu3dsCheckSlider();
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
                if (keysDown & KEY_RIGHT &&
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_GAUGE)
                {
                    if (currentTab->MenuItems[currentTab->SelectedItemIndex].Value <
                        currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMaxValue)
                    {
                        currentTab->MenuItems[currentTab->SelectedItemIndex].Value ++ ;
                    }
                    menu3dsDrawEverything(isDialog, currentMenuTab, menuTab);
                }
                else
                {
                    currentTab = menu3dsAnimateTab(isDialog, currentMenuTab, menuTab, +1);
                }
            }
        }
        if ((keysDown & KEY_LEFT) || (keysDown & KEY_L))
        {
            if (!isDialog)
            {
                if (keysDown & KEY_LEFT &&
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_GAUGE)
                {
                    // Gauge adjustment
                    if (currentTab->MenuItems[currentTab->SelectedItemIndex].Value >
                        currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMinValue)
                    {
                        currentTab->MenuItems[currentTab->SelectedItemIndex].Value -- ;
                    }
                    menu3dsDrawEverything(isDialog, currentMenuTab, menuTab);
                }
                else
                {
                    currentTab = menu3dsAnimateTab(isDialog, currentMenuTab, menuTab, -1);
                }
            }
        }
        if (keysDown & KEY_START || keysDown & KEY_A)
        {
            if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_ACTION)
            {
                returnResult = currentTab->MenuItems[currentTab->SelectedItemIndex].ID;
                break;
            }
            if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_CHECKBOX)
            {
                if (currentTab->MenuItems[currentTab->SelectedItemIndex].Value == 0)
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Value = 1;
                else
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Value = 0;
                menu3dsDrawEverything(isDialog, currentMenuTab, menuTab);
            }
            if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_PICKER)
            {
                snprintf(menuTextBuffer, 511, "%s", currentTab->MenuItems[currentTab->SelectedItemIndex].Text);
                int resultValue = menu3dsShowDialog(isDialog, currentMenuTab, menuTab, menuTextBuffer,
                    currentTab->MenuItems[currentTab->SelectedItemIndex].PickerDescription,
                    currentTab->MenuItems[currentTab->SelectedItemIndex].PickerBackColor,
                    (SMenuItem *)currentTab->MenuItems[currentTab->SelectedItemIndex].PickerItems,
                    currentTab->MenuItems[currentTab->SelectedItemIndex].PickerItemCount,
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Value
                    );
                if (resultValue != -1)
                {
                    if (itemChangedCallback)
                        itemChangedCallback(currentTab->MenuItems[currentTab->SelectedItemIndex].ID, resultValue);
                    
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Value = resultValue;
                }
                menu3dsDrawEverything(isDialog, currentMenuTab, menuTab);
                menu3dsHideDialog(isDialog, currentMenuTab, menuTab);


            }
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
            while (
                (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_DISABLED ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_HEADER1 ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_HEADER2
                ) &&
                moveCursorTimes < currentTab->ItemCount);

            if (currentTab->SelectedItemIndex < currentTab->FirstItemIndex)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + maxItems)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - maxItems + 1;

            menu3dsDrawEverything(isDialog, currentMenuTab, menuTab);

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
            while (
                (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_DISABLED ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_HEADER1 ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MENUITEM_HEADER2
                ) &&
                moveCursorTimes < currentTab->ItemCount);

            if (currentTab->SelectedItemIndex < currentTab->FirstItemIndex)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex;
            if (currentTab->SelectedItemIndex >= currentTab->FirstItemIndex + maxItems)
                currentTab->FirstItemIndex = currentTab->SelectedItemIndex - maxItems + 1;

            menu3dsDrawEverything(isDialog, currentMenuTab, menuTab);
        }

        menu3dsSwapBuffersAndWaitForVBlank();
    }

    return returnResult;
    
}



void menu3dsAddTab(std::vector<SMenuTab>& menuTab, char *title, SMenuItem *menuItems, int itemCount)
{
    menuTab.emplace_back();
    SMenuTab *currentTab = &menuTab.back();

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
}



void menu3dsSetTabSubTitle(std::vector<SMenuTab>& menuTab, int tabIndex, char *subtitle)
{
    SMenuTab *currentTab = &menuTab[tabIndex];

    currentTab->SubTitle[0] = 0;
    if (subtitle != NULL)
        strncpy(currentTab->SubTitle, subtitle, 255);
}

void menu3dsSetSelectedItemIndexByID(int& currentMenuTab, std::vector<SMenuTab>& menuTab, int tabIndex, int ID)
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


void menu3dsSetValueByID(std::vector<SMenuTab>& menuTab, int tabIndex, int ID, int value)
{
    SMenuTab *currentTab = &menuTab[tabIndex];

    for (int i = 0; i < currentTab->ItemCount; i++)
    {
        if (currentTab->MenuItems[i].ID == ID)
        {
            currentTab->MenuItems[i].Value = value;
            break;
        }
    }
}


int menu3dsGetValueByID(std::vector<SMenuTab>& menuTab, int tabIndex, int ID)
{
    SMenuTab *currentTab = &menuTab[tabIndex];

    for (int i = 0; i < currentTab->ItemCount; i++)
    {
        if (currentTab->MenuItems[i].ID == ID)
        {
            return currentTab->MenuItems[i].Value;
        }
    }
    return -1;
}

int menu3dsShowMenu(bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, void (*itemChangedCallback)(int ID, int value), bool animateMenu)
{
    isDialog = false;

    if (animateMenu)
    {
        for (int f = 8; f >= 0; f--)
        {
            aptMainLoop();
            menu3dsDrawEverything(isDialog, currentMenuTab, menuTab, f, 0, 0);
            menu3dsSwapBuffersAndWaitForVBlank();  
        }
    }

    return menu3dsMenuSelectItem(isDialog, currentMenuTab, menuTab, itemChangedCallback);

}

void menu3dsHideMenu(bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab)
{
    for (int f = 0; f <= 8; f++)
    {
        aptMainLoop();
        menu3dsDrawEverything(isDialog, currentMenuTab, menuTab, f, 0, 0);
        menu3dsSwapBuffersAndWaitForVBlank();  
    }    
    ui3dsSetTranslate(0, 0);
}

int menu3dsShowDialog(bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, char *title, char *dialogText, int newDialogBackColor, SMenuItem *menuItems, int itemCount, int selectedID)
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
        if ((selectedID == -1 && menuItems[i].ID > -1) || 
            menuItems[i].ID == selectedID)
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
    menu3dsDrawEverything(isDialog, currentMenuTab, menuTab);
    menu3dsSwapBuffersAndWaitForVBlank();  
    //ui3dsCopyFromFrameBuffer(savedBuffer);

    isDialog = true;
    for (int f = 8; f >= 0; f--)
    {
        aptMainLoop();
        menu3dsDrawEverything(isDialog, currentMenuTab, menuTab, 0, 0, f);
        menu3dsSwapBuffersAndWaitForVBlank();  
    }

    // Execute the dialog and return result.
    //
    if (itemCount > 0)
    {
        int result = menu3dsMenuSelectItem(isDialog, currentMenuTab, menuTab, NULL);

        return result;
    }
    return 0;
}


void menu3dsHideDialog(bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab)
{
    // fade the dialog out
    //
    for (int f = 0; f <= 8; f++)
    {
        aptMainLoop();
        menu3dsDrawEverything(isDialog, currentMenuTab, menuTab, 0, 0, f);
        menu3dsSwapBuffersAndWaitForVBlank();    
    }

    isDialog = false;
    
    // draw the updated menu
    //
    aptMainLoop();
    menu3dsDrawEverything(isDialog, currentMenuTab, menuTab);
    menu3dsSwapBuffersAndWaitForVBlank();  
    
}



bool menu3dsTakeScreenshot(const char* path)
{
    int x, y;

    FILE *pFile = fopen(path, "wb");
    if (pFile == NULL) return false;

    // Modified this to take only the top screen
    //
    u32 bitmapsize = 400*240*2;
    u8* tempbuf = (u8*)linearAlloc(0x8A + 400*240*2);
    if (tempbuf == NULL)
    {
        fclose(pFile);
        return false;
    }
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

