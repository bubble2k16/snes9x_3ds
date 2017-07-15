#ifndef _3DSMENU_H_
#define _3DSMENU_H_

#include <functional>
#include <string>
#include <vector>

enum class MenuItemType {
    Disabled,
    Header1,
    Header2,
    Action,
    Checkbox,
    Gauge,
    Picker,
};

class SMenuItem {
public:
    MenuItemType Type;

    int     ID;                 
    
    std::string Text;

    std::string Description;

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
    std::string PickerDescription;
    std::vector<SMenuItem> PickerItems;
    int     PickerBackColor;

protected:
    std::function<void(int)> ValueChangedCallback;

public:
    SMenuItem(
        std::function<void(int)> callback,
        MenuItemType type, int id, const std::string& text, const std::string& description, int value = 0,
        int min = 0, int max = 0,
        const std::string& pickerDesc = std::string(), const std::vector<SMenuItem>& pickerItems = std::vector<SMenuItem>(), int pickerColor = 0
    ) : ValueChangedCallback(callback), Type(type), ID(id), Text(text), Description(description), Value(value),
        GaugeMinValue(min), GaugeMaxValue(max),
        PickerDescription(pickerDesc), PickerItems(pickerItems), PickerBackColor(pickerColor) {}

    void SetValue(int value) {
        this->Value = value;
        if (this->ValueChangedCallback) {
            this->ValueChangedCallback(value);
        }
    }
};

class SMenuTab {
public:
    std::vector<SMenuItem> MenuItems;
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

void menu3dsAddTab(std::vector<SMenuTab>& menuTab, char *title, const std::vector<SMenuItem>& menuItems);
void menu3dsSetSelectedItemByIndex(SMenuTab& tab, int index);

void menu3dsDrawBlackScreen(float opacity = 1.0f);

int menu3dsShowMenu(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, bool animateMenu);
void menu3dsHideMenu(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab);

int menu3dsShowDialog(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab, const std::string& title, const std::string& dialogText, int dialogBackColor, const std::vector<SMenuItem>& menuItems, int selectedID = -1);
void menu3dsHideDialog(SMenuTab& dialogTab, bool& isDialog, int& currentMenuTab, std::vector<SMenuTab>& menuTab);

bool menu3dsTakeScreenshot(const char *path);


#define DIALOGCOLOR_RED     0xEC407A
#define DIALOGCOLOR_GREEN   0x4CAF50
#define DIALOGCOLOR_CYAN    0x0097A7

#endif
