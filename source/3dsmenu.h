
#ifndef _3DSMENU_H_
#define _3DSMENU_H_

typedef struct
{
    int     ID;
    char    *Text;
    int     Checked;            // -1, not a checkbox
                                // 0, unchecked
                                // 1, checked

    int     GaugeMinValue;      
    int     GaugeMaxValue;      // Set MinValue < MaxValue to make the gauge visible. 
    int     GaugeValue;         // 
} SMenuItem;


void S9xSetTransferGameScreen(bool transfer);
int S9xMenuSelectItem();

void S9xSetTabSubTitle(int tabIndex, char *subtitle);
void S9xAddTab(char *title, SMenuItem *menuItems, int itemCount);
void S9xClearMenuTabs();
void S9xSetCurrentMenuTab(int tabIndex);
void S9xSetSelectedItemIndexByID(int tabIndex, int ID);

void S9xShowTitleAndMessage(
    int titleForeColor, int titleBackColor,
    int mainForeColor, int mainBackColor,
    char *title, char *messageLine1, char *messageLine2, char *messageLine3, char *messageLine4);
void S9xShowWaitingMessage(char *title, char *messageLine1, char *messageLine2);
void S9xAlertSuccess(char *title, char *messageLine1, char *messageLine2);
void S9xAlertFailure(char *title, char *messageLine1, char *messageLine2);
bool S9xConfirm(char *title, char *messageLine1, char *messageLine2);

void S9xUncheckGroup(SMenuItem *menuItems, int itemCount, int group);
void S9xCheckItemByID(SMenuItem *menuItems, int itemCount, int id);
void S9xSetCheckItemByID(SMenuItem *menuItems, int itemCount, int id, int value);
void S9xSetGaugeValueItemByID(SMenuItem *menuItems, int itemCount, int id, int value, char *text);
int S9xGetGaugeValueItemByID(SMenuItem *menuItems, int itemCount, int id);

bool S9xTakeScreenshot(char *path);

#endif