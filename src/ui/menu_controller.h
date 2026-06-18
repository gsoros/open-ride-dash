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
    static constexpr const char* tag = "MenuController";
    static constexpr uint8_t MENU_ITEM_COUNT = 16;
    static constexpr uint8_t EXIT_MENU_ITEM = MENU_ITEM_COUNT - 1;

    inline static constexpr const char* MENU_ITEMS[MENU_ITEM_COUNT] = {
        "Dummy 1",
        "Dummy 2",
        "Another menu item",
        "Item with a long name",
        "Short",
        "Dummy 6",
        "Dummy 7",
        "Dummy 8",
        "Dummy 9",
        "Dummy 10",
        "Dummy 11",
        "Dummy 12",
        "Dummy 13",
        "Dummy 14",
        "Dummy 15",
        "Exit",
    };

    bool _active = false;
    bool _dirty = false;
    uint8_t _selectedItem = 0;
};

#endif  // MENU_CONTROLLER_H
