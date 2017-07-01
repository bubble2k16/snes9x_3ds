#ifndef _3DSMENU_H_
#define _3DSMENU_H_

#include <string>
#include <vector>

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

class SMenuTab {
public:
    SMenuItem   *MenuItems;
    std::string SubTitle;
    std::string Title;
    std::string DialogText;
    int         ItemCount;
    int         FirstItemIndex;
    int         SelectedItemIndex;

    void SetTitle(const std::string& title) {
        // Left trim the dialog title
        size_t offs = title.find_first_not_of(' ');
        Title.assign(offs != title.npos ? title.substr(offs) : title);
    }
};


void menu3dsSetTransferGameScreen(bool transfer);

void menu3dsAddTab(std::vector<SMenuTab>& menuTab, char *title, SMenuItem *menuItems, int itemCount);
void menu3dsSetSelectedItemIndexByID(int& currentMenuTab, std::vector<SMenuTab>& menuTab, int tabIndex, int ID);
void menu3dsSetValueByID(std::vector<SMenuTab>& menuTab, int tabIndex, int ID, int value);
int menu3dsGetValueByID(std::vector<SMenuTab>& menuTab, int tabIndex, int ID);

void menu3dsDrawBlackScreen(float opacity = 1.0f);

int menu3dsShowMenu(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, void (*itemChangedCallback)(int ID, int value), bool animateMenu);
void menu3dsHideMenu(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab);

int menu3dsShowDialog(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, char *title, char *dialogText, int dialogBackColor, SMenuItem *menuItems, int itemCount, int selectedID = -1);
void menu3dsHideDialog(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab);

bool menu3dsTakeScreenshot(const char *path);


#define DIALOGCOLOR_RED     0xEC407A
#define DIALOGCOLOR_GREEN   0x4CAF50
#define DIALOGCOLOR_CYAN    0x0097A7

#endif
