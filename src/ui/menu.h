#ifndef MENU_H
#define MENU_H

/*
    TODO: Refactor this class to use the improved structure below, allowing more maintainable and flexible menu items with labels and actions. The current implementation is functional but lacks the ability to easily add new items or change their behavior without modifying the class itself.

// Usage examples:
// menu[Menu::Key::Wifi].label = "WiFi Connecting";
// menu.execute(Menu::Key::Wifi);
// for (const auto& item : menu) display.drawMenuLine(item.label);

class Menu {
   public:
    static constexpr size_t MAX_LABEL_LEN = 32;

    struct Label {
        char buffer[MAX_LABEL_LEN]{};

        Label& operator=(const char* str) {
            strncpy(buffer, str, MAX_LABEL_LEN - 1);
            buffer[MAX_LABEL_LEN - 1] = '\0';
            return *this;
        }

        operator const char*() const { return buffer; }
    };

    struct Item {
        Label label;
        void (*action)() = nullptr;
    };

    enum class Key : uint8_t {
        Wifi = 0,
        Brightness,
        Exit,
        Count  // Total number of keys
    };

    Menu() {
        items[idx(Key::Wifi)].label = "WiFi";
        items[idx(Key::Wifi)].action = &onWifiSelect;

        items[idx(Key::Brightness)].label = "Brightness";
        items[idx(Key::Brightness)].action = &onBrightnessSelect;

        items[idx(Key::Exit)].label = "Exit";
        items[idx(Key::Exit)].action = &onExitSelect;
    }

    Item& operator[](Key key) {
        return items[idx(key)];
    }

    const Item& operator[](Key key) const {
        return items[idx(key)];
    }

    void execute(Key key) {
        auto& item = (*this)[key];
        if (item.action) {
            item.action();
        }
    }

    auto begin() { return items.begin(); }
    auto end() { return items.end(); }
    auto begin() const { return items.begin(); }
    auto end() const { return items.end(); }

   private:
    static constexpr size_t idx(Key k) { return static_cast<size_t>(k); }

    static void onWifiSelect() {
      api.queueCommand("wifi");
    }

    static void onBrightnessSelect() {
      display.saveBrightness();
    }

    static void onExitSelect() {
      exit();
    }

    static void onBrightnessChange(bool isSaved) {
      items[idx(Key::Brightness)].label = isSaved ? "Brightness Saved" : "Save Brightness";
    }

    std::array<Item, static_cast<size_t>(Key::Count)> items{};
};

*/

#include <Arduino.h>

class Menu {
   public:
    enum class SelectResult : uint8_t {
        Ignored,
        Handled,
        Exit,
    };

    struct Snapshot {
        bool active = false;
        bool dirty = false;
        uint8_t selectedItem = 0;
        uint8_t itemCount = 0;
        const char* const* items = nullptr;
    };

    bool active() const { return _active; }
    bool dirty() const { return _dirty; }

    void enter();
    bool previousItem();
    bool nextItem();
    SelectResult selectItem();
    void exit();
    Snapshot snapshot() const;
    void markRendered();

   protected:
    void refreshItems() const;
    static constexpr const char* tag = "Menu";
    static constexpr uint8_t ITEM_COUNT = 16;
    static constexpr uint8_t EXIT_ITEM = ITEM_COUNT - 1;  // Last item is "Exit"

    inline static constexpr const char* STATIC_ITEMS[ITEM_COUNT] = {
        "WiFi",                     // 0 dynamic label provided at runtime
        "Save Brightness",          // 1
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

    mutable const char* _items[ITEM_COUNT] = {};
    mutable char _item0Label[32] = {};
    mutable bool _dirty = false;
    bool _active = false;
    uint8_t _selectedItem = 0;
};

#endif  // MENU_H
