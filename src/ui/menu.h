#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <array>

class Display;

class Menu {
   public:
    static constexpr size_t MAX_LABEL_LEN = 32;

    struct Label {
        char buffer[MAX_LABEL_LEN]{};

        Label& operator=(const char* str) {
            if (str == nullptr) {
                buffer[0] = '\0';
                return *this;
            }
            strncpy(buffer, str, MAX_LABEL_LEN - 1);
            buffer[MAX_LABEL_LEN - 1] = '\0';
            return *this;
        }

        operator const char*() const { return buffer; }
    };

    using Action = void (Menu::*)();

    struct Item {
        Label label;
        Action action = nullptr;
    };

    enum class SelectResult : uint8_t {
        Ignored,
        Handled,
        Exit,
    };

    enum class Key : uint8_t {
        Wifi = 0,
        SaveBrightness,
        Dummy2,
        AnotherMenuItem3,
        ItemWithLongName4,
        Short,
        Dummy6,
        Dummy7,
        ItemWithLongName8,
        Dummy9,
        Dummy10,
        Dummy11,
        Dummy12,
        Dummy13,
        Restart,
        Exit,
        Count,
    };

    struct Snapshot {
        bool active = false;
        bool dirty = false;
        uint8_t selectedItem = 0;
        uint8_t itemCount = 0;
        const char* items[static_cast<size_t>(Key::Count)] = {};
    };

    Menu();

    bool active() const { return _active; }
    bool dirty() const { return _dirty; }
    void setDirty(bool b = true) { _dirty = b; }

    Item& operator[](Key key) { return _items[idx(key)]; }
    const Item& operator[](Key key) const { return _items[idx(key)]; }
    void execute(Key key);

    void enter();
    bool previousItem();
    bool nextItem();
    SelectResult selectItem();
    void exit();
    Snapshot snapshot() const;
    void markRendered();
    void onBrightnessChange(bool saved);

    auto begin() { return _items.begin(); }
    auto end() { return _items.end(); }
    auto begin() const { return _items.begin(); }
    auto end() const { return _items.end(); }

   protected:
    static constexpr const char* tag = "Menu";
    static constexpr uint8_t ITEM_COUNT = static_cast<uint8_t>(Key::Count);
    static constexpr uint8_t EXIT_ITEM = static_cast<uint8_t>(Key::Exit);

    static constexpr size_t idx(Key k) { return static_cast<size_t>(k); }

    void onWifiSelect();
    void onBrightnessSelect();
    void onRestartSelect();
    void onNoopSelect();
    void onExitSelect();

    mutable std::array<Item, ITEM_COUNT> _items{};
    mutable bool _dirty = false;
    bool _active = false;
    uint8_t _selectedItem = 0;
};

#endif  // MENU_H
