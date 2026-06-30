#ifndef MENU_CONTROLLER_H
#define MENU_CONTROLLER_H

#include <Arduino.h>

struct MenuSnapshot {
    bool active = false;
    bool dirty = false;
    uint8_t selectedItem = 0;
    uint8_t itemCount = 0;
    const char* const* items = nullptr;
};

class MenuController {
   public:
    enum class SelectResult : uint8_t {
        Ignored,
        Handled,
        ExitMenu,
    };

    bool active() const { return _active; }
    bool dirty() const { return _dirty; }

    void enterMenu();
    bool previousMenuItem();
    bool nextMenuItem();
    SelectResult selectMenuItem();
    void exitMenu();
    MenuSnapshot snapshot() const;
    void markRendered();

   protected:
    void refreshItems() const;
    static constexpr const char* tag = "MenuController";
    static constexpr uint8_t MENU_ITEM_COUNT = 16;
    static constexpr uint8_t EXIT_MENU_ITEM = MENU_ITEM_COUNT - 1;  // Last item is "Exit"

    inline static constexpr const char* STATIC_MENU_ITEMS[MENU_ITEM_COUNT] = {
        "WiFi",                     // 0 dynamic label provided at runtime
        "Dummy 1",                  // 1
        "Dummy 2",                  // 2
        "Another menu item 3",      // 3
        "Item with a long name 4",  // 4
        "Short",                    // 5
        "Dummy 6",                  // 6
        "Dummy 7",                  // 7
        "Item with a long name 8",  // 8
        "Dummy 9",                  // 9
        "Dummy 10",                 // 10
        "Dummy 11",                 // 11
        "Dummy 12",                 // 12
        "Dummy 13",                 // 13
        "Restart",                  // 14
        "Exit"                      // 15
    };

    mutable const char* _items[MENU_ITEM_COUNT] = {};
    mutable char _item0Label[32] = {};
    mutable bool _dirty = false;
    bool _active = false;
    uint8_t _selectedItem = 0;
};

#endif  // MENU_CONTROLLER_H
