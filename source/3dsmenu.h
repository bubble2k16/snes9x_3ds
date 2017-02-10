
#ifndef _3DSMENU_H_
#define _3DSMENU_H_

#define MENUITEM_DISABLED           -1
#define MENUITEM_HEADER1            0
#define MENUITEM_HEADER2            1
#define MENUITEM_ACTION             2
#define MENUITEM_CHECKBOX           3
#define MENUITEM_GAUGE              4
#define MENUITEM_PICKER             5

typedef struct
{
    int     Type;               // -1 - Disabled
                                // 0 - Header
                                // 1 - Action 
                                // 2 - Checkbox
                                // 3 - Gauge
                                // 4 - Picker

    int     ID;                 
    
    char    *Text;

    char    *Description;

    int     Value;              
                                // Type = Gauge:
                                //   Value = Gauge Value
                                // Type = Checkbox:
                                //   0, unchecked
                                //   1, checked
                                // Type = Picker:
                                //   Selected ID of Picker

    int     GaugeMinValue;
    int     GaugeMaxValue;      // Set MinValue < MaxValue to make the gauge visible.

    // All these fields are used if this is a picker.
    // (ID = 100000)
    //
    char    *PickerDescription;
    int     PickerItemCount;
    void    *PickerItems;
    int     PickerBackColor;
} SMenuItem;




void menu3dsSetTransferGameScreen(bool transfer);

void menu3dsSetTabSubTitle(int tabIndex, char *subtitle);
void menu3dsAddTab(char *title, SMenuItem *menuItems, int itemCount);
void menu3dsClearMenuTabs();
void menu3dsSetCurrentMenuTab(int tabIndex);
void menu3dsSetSelectedItemIndexByID(int tabIndex, int ID);
void menu3dsSetValueByID(int tabIndex, int ID, int value);
int menu3dsGetValueByID(int tabIndex, int ID);

void menu3dsDrawBlackScreen(float opacity = 1.0f);

int menu3dsShowMenu(void (*itemChangedCallback)(int ID, int value), bool animateMenu);
void menu3dsHideMenu();

int menu3dsShowDialog(char *title, char *dialogText, int dialogBackColor, SMenuItem *menuItems, int itemCount, int selectedID = -1);
void menu3dsHideDialog();

bool menu3dsTakeScreenshot(const char *path);


#define DIALOGCOLOR_RED     0xEC407A
#define DIALOGCOLOR_GREEN   0x4CAF50
#define DIALOGCOLOR_CYAN    0x0097A7

#endif
