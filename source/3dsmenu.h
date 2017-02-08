
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




void S9xSetTransferGameScreen(bool transfer);
int S9xMenuSelectItem(void (*itemChangedCallback)(int ID, int value));

void S9xSetTabSubTitle(int tabIndex, char *subtitle);
void S9xAddTab(char *title, SMenuItem *menuItems, int itemCount);
void S9xClearMenuTabs();
void S9xSetCurrentMenuTab(int tabIndex);
void S9xSetSelectedItemIndexByID(int tabIndex, int ID);
void S9xSetValueByID(int tabIndex, int ID, int value);
int S9xGetValueByID(int tabIndex, int ID);

void S9xDrawBlackScreen(float opacity = 1.0f);
int S9xShowDialog(char *title, char *dialogText, int dialogBackColor, SMenuItem *menuItems, int itemCount, int selectedID = -1);
void S9xHideDialog();

bool S9xTakeScreenshot(const char *path);


#define DIALOGCOLOR_RED     0xEC407A
#define DIALOGCOLOR_GREEN   0x4CAF50
#define DIALOGCOLOR_CYAN    0x0097A7

#endif
