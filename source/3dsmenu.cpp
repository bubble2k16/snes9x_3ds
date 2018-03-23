#include <array>
#include <cmath>
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

#define SNES9X_VERSION "v1.30"
#define ANIMATE_TAB_STEPS 3


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
    
    // Display the subtitle
    if (!currentTab->SubTitle.empty())
    {
        maxItems--;
        ui3dsDrawStringWithNoWrapping(20, menuStartY, 300, menuStartY + fontHeight, 
            subtitleTextColor, HALIGN_LEFT, currentTab->SubTitle.c_str());
        menuStartY += fontHeight;
    }

    int line = 0;
    int color = 0xffffff;

    // Draw all the individual items
    //
    for (int i = currentTab->FirstItemIndex;
        i < currentTab->MenuItems.size() && i < currentTab->FirstItemIndex + maxItems; i++)
    {
        int y = line * fontHeight + menuStartY;

        // Draw the selected background 
        //
        if (currentTab->SelectedItemIndex == i)
        {
            ui3dsDrawRect(0, y, 320, y + 14, selectedItemBackColor);
        }
        
        if (currentTab->MenuItems[i].Type == MenuItemType::Header1)
        {
            color = headerItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, currentTab->MenuItems[i].Text.c_str());
            ui3dsDrawRect(horizontalPadding, y + fontHeight - 1, 320 - horizontalPadding, y + fontHeight, color);
        }
        else if (currentTab->MenuItems[i].Type == MenuItemType::Header2)
        {
            color = headerItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, currentTab->MenuItems[i].Text.c_str());
        }
        else if (currentTab->MenuItems[i].Type == MenuItemType::Disabled)
        {
            color = disabledItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, currentTab->MenuItems[i].Text.c_str());
        }
        else if (currentTab->MenuItems[i].Type == MenuItemType::Action)
        {
            color = normalItemTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemTextColor;
            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, currentTab->MenuItems[i].Text.c_str());

            color = normalItemDescriptionTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemDescriptionTextColor;
            if (!currentTab->MenuItems[i].Description.empty())
            {
                ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, currentTab->MenuItems[i].Description.c_str());
            }
        }
        else if (currentTab->MenuItems[i].Type == MenuItemType::Checkbox)
        {
            if (currentTab->MenuItems[i].Value == 0)
            {
                color = disabledItemTextColor;
                if (currentTab->SelectedItemIndex == i)
                    color = selectedItemTextColor;
                ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, currentTab->MenuItems[i].Text.c_str());

                ui3dsDrawStringWithNoWrapping(280, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, "\xfe");
            }
            else
            {
                color = normalItemTextColor;
                if (currentTab->SelectedItemIndex == i)
                    color = selectedItemTextColor;
                ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, currentTab->MenuItems[i].Text.c_str());

                ui3dsDrawStringWithNoWrapping(280, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, "\xfd");
            }
        }
        else if (currentTab->MenuItems[i].Type == MenuItemType::Gauge)
        {
            color = normalItemTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemTextColor;

            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_LEFT, currentTab->MenuItems[i].Text.c_str());

            const int max = 40;
            int diff = currentTab->MenuItems[i].GaugeMaxValue - currentTab->MenuItems[i].GaugeMinValue;
            int pos = (currentTab->MenuItems[i].Value - currentTab->MenuItems[i].GaugeMinValue) * (max - 1) / diff;

            char gauge[max+1];
            for (int j = 0; j < max; j++)
                gauge[j] = (j == pos) ? '\xfa' : '\xfb';
            gauge[max] = 0;
            ui3dsDrawStringWithNoWrapping(245, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, gauge);
        }
        else if (currentTab->MenuItems[i].Type == MenuItemType::Picker)
        {
            color = normalItemTextColor;
            if (currentTab->SelectedItemIndex == i)
                color = selectedItemTextColor;

            ui3dsDrawStringWithNoWrapping(horizontalPadding, y, 160, y + fontHeight, color, HALIGN_LEFT, currentTab->MenuItems[i].Text.c_str());

            if (!currentTab->MenuItems[i].PickerItems.empty() && currentTab->MenuItems[i].GaugeMinValue)
            {
                int selectedIndex = -1;
                for (int j = 0; j < currentTab->MenuItems[i].PickerItems.size(); j++)
                {
                    std::vector<SMenuItem>& pickerItems = currentTab->MenuItems[i].PickerItems;
                    if (pickerItems[j].Value == currentTab->MenuItems[i].Value)
                    {
                        selectedIndex = j;
                    }
                }
                if (selectedIndex > -1)
                {
                    ui3dsDrawStringWithNoWrapping(160, y, 320 - horizontalPadding, y + fontHeight, color, HALIGN_RIGHT, currentTab->MenuItems[i].PickerItems[selectedIndex].Text.c_str());
                }
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
    if (currentTab->FirstItemIndex + maxItems < currentTab->MenuItems.size())
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
        int color = i == currentMenuTab ? 0xFFFFFF : 0x90CAF9;

        int offsetLeft = 10;
        int offsetRight = 10;

        int availableSpace = 320 - ( offsetLeft + offsetRight );
        int pixelPerOption =      availableSpace / static_cast<int>(menuTab.size());
        int extraPixelOnOptions = availableSpace % static_cast<int>(menuTab.size());

        // each tab gains an equal amount of horizontal space
        // if space is not cleanly divisible by tab count, the earlier tabs gain one extra pixel each until we reach the requested space
        int xLeft =  (     i     * pixelPerOption ) + offsetLeft + std::min( i,     extraPixelOnOptions );
        int xRight = ( ( i + 1 ) * pixelPerOption ) + offsetLeft + std::min( i + 1, extraPixelOnOptions );
        int yTextTop = 6;
        int yCurrentTabBoxTop = 21;
        int yCurrentTabBoxBottom = 24;

        ui3dsDrawStringWithNoWrapping(xLeft, yTextTop, xRight, yCurrentTabBoxTop, color, HALIGN_CENTER, menuTab[i].Title.c_str());

        if (i == currentMenuTab) {
            ui3dsDrawRect(xLeft, yCurrentTabBoxTop, xRight, yCurrentTabBoxBottom, 0xFFFFFF);
        }
    }

    // Shadows
    //ui3dsDrawRect(0, 23, 320, 24, 0xaaaaaa);
    //ui3dsDrawRect(0, 24, 320, 25, 0xcccccc);
    //ui3dsDrawRect(0, 25, 320, 27, 0xeeeeee);

    ui3dsDrawStringWithNoWrapping(10, 223, 285, 240, 0xFFFFFF, HALIGN_LEFT,
        "A:Select  B:Cancel");
    ui3dsDrawStringWithNoWrapping(10, 223, 285, 240, 0xFFFFFF, HALIGN_RIGHT,
        "SNES9x for 3DS " SNES9X_VERSION);

    //battery display
    const int maxBatteryLevel = 5;
    const int battLevelWidth = 3;
    const int battFullLevelWidth = (maxBatteryLevel) * battLevelWidth + 1;
    const int battBorderWidth = 1;
    const int battY1 = 226;
    const int battY2 = 233;
    const int battX2 = 311;
    const int battYHeight = battY2 - battY1;
    const int battHeadWidth = 2;
    const int battHeadSpacing = 1;

    // battery positive end
    ui3dsDrawRect(
        battX2 - battFullLevelWidth - battBorderWidth - battHeadWidth, 
        battY1 + battHeadSpacing, 
        battX2 - battFullLevelWidth - battBorderWidth, 
        battY2 - battHeadSpacing, 
        0xFFFFFF, 1.0f);
    // battery body
    ui3dsDrawRect(
        battX2 - battFullLevelWidth - battBorderWidth, 
        battY1 - battBorderWidth, 
        battX2 + battBorderWidth, 
        battY2 + battBorderWidth, 
        0xFFFFFF, 1.0f);
    // battery's empty insides
    ui3dsDrawRect(
        battX2 - battFullLevelWidth, 
        battY1, 
        battX2, 
        battY2, 
        0x1976D2, 1.0f);
    
    ptmuInit();
    
    u8 batteryChargeState = 0;
    u8 batteryLevel = 0;
    if(R_SUCCEEDED(PTMU_GetBatteryChargeState(&batteryChargeState)) && batteryChargeState) {
        ui3dsDrawRect(
            battX2-battFullLevelWidth + 1, battY1 + 1, 
            battX2 - 1, battY2 - 1, 0xFF9900, 1.0f);
    } else if(R_SUCCEEDED(PTMU_GetBatteryLevel(&batteryLevel))) {
        if (batteryLevel > 5)
            batteryLevel = 5;
        for (int i = 0; i < batteryLevel; i++)
        {
            ui3dsDrawRect(
                battX2-battLevelWidth*(i+1), battY1 + 1, 
                battX2-battLevelWidth*(i) - 1, battY2 - 1, 0xFFFFFF, 1.0f);
        }
    } else {
        //ui3dsDrawRect(battX2, battY1, battX2, battY2, 0xFFFFFF, 1.0f);
    }
 
    ptmuExit();

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

void menu3dsDrawDialog(SMenuTab& dialogTab)
{
    // Dialog's Background
    int dialogBackColor2 = ui3dsApplyAlphaToColor(dialogBackColor, 0.9f);
    ui3dsDrawRect(0, 0, 320, 75, dialogBackColor2);
    ui3dsDrawRect(0, 75, 320, 160, dialogBackColor);

    // Draw the dialog's title and descriptive text
    int dialogTitleTextColor = 
        ui3dsApplyAlphaToColor(dialogBackColor, 0.5f) + 
        ui3dsApplyAlphaToColor(dialogTextColor, 0.5f);
    ui3dsDrawStringWithNoWrapping(30, 10, 290, 25, dialogTitleTextColor, HALIGN_LEFT, dialogTab.Title.c_str());
    ui3dsDrawStringWithWrapping(30, 30, 290, 70, dialogTextColor, HALIGN_LEFT, dialogTab.DialogText.c_str());

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


void menu3dsDrawEverything(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, int menuFrame = 0, int menuItemsFrame = 0, int dialogFrame = 0)
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
        menu3dsDrawDialog(dialogTab);
        ui3dsSetTranslate(0, 0);
    }
    swapBuffer = true;
}


SMenuTab *menu3dsAnimateTab(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, int direction)
{
    SMenuTab *currentTab = &menuTab[currentMenuTab];

    if (direction < 0)
    {
        for (int i = 1; i <= ANIMATE_TAB_STEPS; i++)
        {
            aptMainLoop();
            menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab, 0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }

        currentMenuTab--;
        if (currentMenuTab < 0)
            currentMenuTab = static_cast<int>(menuTab.size() - 1);
        currentTab = &menuTab[currentMenuTab];
        
        for (int i = -ANIMATE_TAB_STEPS; i <= 0; i++)
        {
            aptMainLoop();
            menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab, 0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }
    }
    else if (direction > 0)
    {
        for (int i = -1; i >= -ANIMATE_TAB_STEPS; i--)
        {
            aptMainLoop();
            menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab, 0, i, 0);
            menu3dsSwapBuffersAndWaitForVBlank();
        }

        currentMenuTab++;
        if (currentMenuTab >= static_cast<int>(menuTab.size()))
            currentMenuTab = 0;
        currentTab = &menuTab[currentMenuTab];
        
        for (int i = ANIMATE_TAB_STEPS; i >= 0; i--)
        {
            aptMainLoop();
            menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab, 0, i, 0);
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
int menu3dsMenuSelectItem(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab)
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
        menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab);
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

        if (!currentTab->SubTitle.empty())
        {
            maxItems--;
        }

        if ((thisKeysHeld & KEY_UP) || (thisKeysHeld & KEY_DOWN) || (thisKeysHeld & KEY_LEFT) || (thisKeysHeld & KEY_RIGHT))
            framesDKeyHeld ++;
        else
            framesDKeyHeld = 0;
        if (keysDown & KEY_B)
        {
            returnResult = -1;
            break;
        }
        if ((keysDown & KEY_RIGHT) || (keysDown & KEY_R) || ((thisKeysHeld & KEY_RIGHT) && (framesDKeyHeld > 15) && (framesDKeyHeld % 2 == 0)))
        {
            if (!isDialog)
            {
                if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Gauge)
                {
                    if (keysDown & KEY_RIGHT || ((thisKeysHeld & KEY_RIGHT) && (framesDKeyHeld > 15) && (framesDKeyHeld % 2 == 0)))
                    {
                        if (currentTab->MenuItems[currentTab->SelectedItemIndex].Value <
                            currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMaxValue)
                        {
                            currentTab->MenuItems[currentTab->SelectedItemIndex].SetValue(currentTab->MenuItems[currentTab->SelectedItemIndex].Value + 1);
                        }
                        menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab);
                    }
                }
                else
                {
                    currentTab = menu3dsAnimateTab(dialogTab, isDialog, currentMenuTab, menuTab, +1);
                }
            }
        }
        if ((keysDown & KEY_LEFT) || (keysDown & KEY_L)|| ((thisKeysHeld & KEY_LEFT) && (framesDKeyHeld > 15) && (framesDKeyHeld % 2 == 0)))
        {
            if (!isDialog)
            {
                if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Gauge)
                {
                    if (keysDown & KEY_LEFT || ((thisKeysHeld & KEY_LEFT) && (framesDKeyHeld > 15) && (framesDKeyHeld % 2 == 0)))
                    {
                        // Gauge adjustment
                        if (currentTab->MenuItems[currentTab->SelectedItemIndex].Value >
                            currentTab->MenuItems[currentTab->SelectedItemIndex].GaugeMinValue)
                        {
                            currentTab->MenuItems[currentTab->SelectedItemIndex].SetValue(currentTab->MenuItems[currentTab->SelectedItemIndex].Value - 1);
                        }
                        menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab);
                    }
                }
                else
                {
                    currentTab = menu3dsAnimateTab(dialogTab, isDialog, currentMenuTab, menuTab, -1);
                }
            }
        }
        if (keysDown & KEY_START || keysDown & KEY_A)
        {
            if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Action)
            {
                returnResult = currentTab->MenuItems[currentTab->SelectedItemIndex].Value;
                currentTab->MenuItems[currentTab->SelectedItemIndex].SetValue(1);
                break;
            }
            if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Checkbox)
            {
                if (currentTab->MenuItems[currentTab->SelectedItemIndex].Value == 0)
                    currentTab->MenuItems[currentTab->SelectedItemIndex].SetValue(1);
                else
                    currentTab->MenuItems[currentTab->SelectedItemIndex].SetValue(0);
                menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab);
            }
            if (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Picker)
            {
                snprintf(menuTextBuffer, 511, "%s", currentTab->MenuItems[currentTab->SelectedItemIndex].Text.c_str());
                int resultValue = menu3dsShowDialog(dialogTab, isDialog, currentMenuTab, menuTab, menuTextBuffer,
                    currentTab->MenuItems[currentTab->SelectedItemIndex].PickerDescription,
                    currentTab->MenuItems[currentTab->SelectedItemIndex].PickerBackColor,
                    currentTab->MenuItems[currentTab->SelectedItemIndex].PickerItems,
                    currentTab->MenuItems[currentTab->SelectedItemIndex].Value
                    );
                if (resultValue != -1)
                {
                    currentTab->MenuItems[currentTab->SelectedItemIndex].SetValue(resultValue);
                }
                menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab);
                menu3dsHideDialog(dialogTab, isDialog, currentMenuTab, menuTab);


            }
        }
        if (keysDown & KEY_UP || ((thisKeysHeld & KEY_UP) && (framesDKeyHeld > 15) && (framesDKeyHeld % 2 == 0)))
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
                        currentTab->SelectedItemIndex = currentTab->MenuItems.size() - 1;
                    }
                }
                moveCursorTimes++;
            }
            while (
                (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Disabled ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Header1 ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Header2
                ) &&
                moveCursorTimes < currentTab->MenuItems.size());

            currentTab->MakeSureSelectionIsOnScreen(maxItems, isDialog ? 1 : 2);
            menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab);

        }
        if (keysDown & KEY_DOWN || ((thisKeysHeld & KEY_DOWN) && (framesDKeyHeld > 15) && (framesDKeyHeld % 2 == 0)))
        {
            int moveCursorTimes = 0;
            do
            {
                if (thisKeysHeld & KEY_X)
                {
                    currentTab->SelectedItemIndex += 15;
                    if (currentTab->SelectedItemIndex >= currentTab->MenuItems.size())
                        currentTab->SelectedItemIndex = currentTab->MenuItems.size() - 1;
                }
                else
                {
                    currentTab->SelectedItemIndex++;
                    if (currentTab->SelectedItemIndex >= currentTab->MenuItems.size())
                    {
                        currentTab->SelectedItemIndex = 0;
                        currentTab->FirstItemIndex = 0;
                    }
                }
                moveCursorTimes++;
            }
            while (
                (currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Disabled ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Header1 ||
                currentTab->MenuItems[currentTab->SelectedItemIndex].Type == MenuItemType::Header2
                ) &&
                moveCursorTimes < currentTab->MenuItems.size());

            currentTab->MakeSureSelectionIsOnScreen(maxItems, isDialog ? 1 : 2);
            menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab);
        }

        menu3dsSwapBuffersAndWaitForVBlank();
    }

    return returnResult;
    
}



void menu3dsAddTab(std::vector<SMenuTab>& menuTab, char *title, const std::vector<SMenuItem>& menuItems)
{
    menuTab.emplace_back();
    SMenuTab *currentTab = &menuTab.back();

    currentTab->SetTitle(title);
    currentTab->MenuItems = menuItems;

    currentTab->FirstItemIndex = 0;
    currentTab->SelectedItemIndex = 0;
    for (int i = 0; i < currentTab->MenuItems.size(); i++)
    {
        if (menuItems[i].IsHighlightable())
        {
            currentTab->SelectedItemIndex = i;
            currentTab->MakeSureSelectionIsOnScreen(MENU_HEIGHT, 2);
            break;
        }
    }
}

void menu3dsSetSelectedItemByIndex(SMenuTab& tab, int index)
{
    if (index >= 0 && index < tab.MenuItems.size()) {
        tab.SelectedItemIndex = index;

        int maxItems = MENU_HEIGHT;
        if (!tab.SubTitle.empty()) {
            maxItems--;
        }
        tab.MakeSureSelectionIsOnScreen(maxItems, 2);
    }
}

int menu3dsShowMenu(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, bool animateMenu)
{
    isDialog = false;

    if (animateMenu)
    {
        for (int f = 8; f >= 0; f--)
        {
            aptMainLoop();
            menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab, f, 0, 0);
            menu3dsSwapBuffersAndWaitForVBlank();  
        }
    }

    return menu3dsMenuSelectItem(dialogTab, isDialog, currentMenuTab, menuTab);

}

void menu3dsHideMenu(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab)
{
    for (int f = 0; f <= 8; f++)
    {
        aptMainLoop();
        menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab, f, 0, 0);
        menu3dsSwapBuffersAndWaitForVBlank();  
    }    
    ui3dsSetTranslate(0, 0);
}

int menu3dsShowDialog(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, const std::string& title, const std::string& dialogText, int newDialogBackColor, const std::vector<SMenuItem>& menuItems, int selectedID)
{
    SMenuTab *currentTab = &dialogTab;

    dialogBackColor = newDialogBackColor;

    currentTab->SetTitle(title);
    currentTab->DialogText.assign(dialogText);
    currentTab->MenuItems = menuItems;

    currentTab->FirstItemIndex = 0;
    currentTab->SelectedItemIndex = 0;

    for (int i = 0; i < currentTab->MenuItems.size(); i++)
    {
        if ((selectedID == -1 && menuItems[i].IsHighlightable()) || 
            menuItems[i].Value == selectedID)
        {
            currentTab->SelectedItemIndex = i;
            currentTab->MakeSureSelectionIsOnScreen(DIALOG_HEIGHT, 1);
            break;
        }
    }

    // fade the dialog fade in
    //
    aptMainLoop();
    menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab);
    menu3dsSwapBuffersAndWaitForVBlank();  
    //ui3dsCopyFromFrameBuffer(savedBuffer);

    isDialog = true;
    for (int f = 8; f >= 0; f--)
    {
        aptMainLoop();
        menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab, 0, 0, f);
        menu3dsSwapBuffersAndWaitForVBlank();  
    }

    // Execute the dialog and return result.
    //
    if (currentTab->MenuItems.size() > 0)
    {
        int result = menu3dsMenuSelectItem(dialogTab, isDialog, currentMenuTab, menuTab);

        return result;
    }
    return 0;
}


void menu3dsHideDialog(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab)
{
    // fade the dialog out
    //
    for (int f = 0; f <= 8; f++)
    {
        aptMainLoop();
        menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab, 0, 0, f);
        menu3dsSwapBuffersAndWaitForVBlank();    
    }

    isDialog = false;
    
    // draw the updated menu
    //
    aptMainLoop();
    menu3dsDrawEverything(dialogTab, isDialog, currentMenuTab, menuTab);
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

