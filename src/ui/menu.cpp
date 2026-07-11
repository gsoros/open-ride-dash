#include "menu.h"

#include "tasks/api.h"
#include "tasks/display.h"
#include "tasks/wifi.h"

extern Api api;
extern Display display;
extern Wifi wifi;

Menu::Menu() {
    _items[idx(Key::Wifi)].label = "WiFi";
    _items[idx(Key::Wifi)].action = &Menu::onWifiSelect;

    _items[idx(Key::SaveBrightness)].label = "Save Brightness";
    _items[idx(Key::SaveBrightness)].action = &Menu::onBrightnessSelect;

    _items[idx(Key::Dummy2)].label = "Dummy 2";
    _items[idx(Key::Dummy2)].action = &Menu::onNoopSelect;

    _items[idx(Key::AnotherMenuItem3)].label = "Another menu item 3";
    _items[idx(Key::AnotherMenuItem3)].action = &Menu::onNoopSelect;

    _items[idx(Key::ItemWithLongName4)].label = "Item with a long name 4";
    _items[idx(Key::ItemWithLongName4)].action = &Menu::onNoopSelect;

    _items[idx(Key::Short)].label = "Short";
    _items[idx(Key::Short)].action = &Menu::onNoopSelect;

    _items[idx(Key::Dummy6)].label = "Dummy 6";
    _items[idx(Key::Dummy6)].action = &Menu::onNoopSelect;

    _items[idx(Key::Dummy7)].label = "Dummy 7";
    _items[idx(Key::Dummy7)].action = &Menu::onNoopSelect;

    _items[idx(Key::ItemWithLongName8)].label = "Item with a long name 8";
    _items[idx(Key::ItemWithLongName8)].action = &Menu::onNoopSelect;

    _items[idx(Key::Dummy9)].label = "Dummy 9";
    _items[idx(Key::Dummy9)].action = &Menu::onNoopSelect;

    _items[idx(Key::Dummy10)].label = "Dummy 10";
    _items[idx(Key::Dummy10)].action = &Menu::onNoopSelect;

    _items[idx(Key::Dummy11)].label = "Dummy 11";
    _items[idx(Key::Dummy11)].action = &Menu::onNoopSelect;

    _items[idx(Key::Dummy12)].label = "Dummy 12";
    _items[idx(Key::Dummy12)].action = &Menu::onNoopSelect;

    _items[idx(Key::Dummy13)].label = "Dummy 13";
    _items[idx(Key::Dummy13)].action = &Menu::onNoopSelect;

    _items[idx(Key::Restart)].label = "Restart";
    _items[idx(Key::Restart)].action = &Menu::onRestartSelect;

    _items[idx(Key::Exit)].label = "Exit";
    _items[idx(Key::Exit)].action = &Menu::onExitSelect;
}

void Menu::execute(Key key) {
    auto& item = (*this)[key];
    if (item.action != nullptr) {
        (this->*item.action)();
    }
}

void Menu::enter() {
    if (_active) return;
    _active = true;
    _selectedItem = 0;
    setDirty();
    ESP_LOGI(tag, "Entering menu");
}

bool Menu::previousItem() {
    if (!_active) return false;
    _selectedItem = _selectedItem == 0 ? ITEM_COUNT - 1 : _selectedItem - 1;
    setDirty();
    return true;
}

bool Menu::nextItem() {
    if (!_active) return false;
    _selectedItem = (_selectedItem + 1) % ITEM_COUNT;
    setDirty();
    return true;
}

Menu::SelectResult Menu::selectItem() {
    if (!_active) return SelectResult::Ignored;
    ESP_LOGD(tag, "Selected item %d: %s",
             _selectedItem, _items[_selectedItem].label);
    if (_selectedItem == EXIT_ITEM) {
        exit();
        return SelectResult::Exit;
    }
    auto& item = _items[_selectedItem];
    if (item.action != nullptr) {
        (this->*item.action)();
    }
    return SelectResult::Handled;
}

void Menu::exit() {
    if (!_active) return;
    _active = false;
    setDirty();
    ESP_LOGD(tag, "Exiting menu");
}

Menu::Snapshot Menu::snapshot() const {
    Menu::Snapshot s = {};
    s.active = _active;
    s.dirty = _dirty;
    s.selectedItem = _selectedItem;
    s.itemCount = ITEM_COUNT;
    for (size_t i = 0; i < ITEM_COUNT; ++i) {
        s.items[i] = _items[i].label;
    }
    return s;
}

void Menu::markRendered() {
    setDirty(false);
}

void Menu::onBrightnessChange(bool saved) {
    _items[idx(Key::SaveBrightness)].label = saved ? "Brightness Saved" : "Save Brightness";
    setDirty();
}

void Menu::onWifiSelect() {
    if (!api.queueCommand("wifi")) {
        ESP_LOGE(tag, "Failed to queue WiFi toggle command");
    }
}

void Menu::onBrightnessSelect() {
    display.saveBrightness();
}

void Menu::onRestartSelect() {
    delay(200);
    if (!api.queueCommand("restart")) {
        ESP_LOGE(tag, "Failed to queue restart command");
    }
}

void Menu::onNoopSelect() {
}

void Menu::onExitSelect() {
    exit();
}
