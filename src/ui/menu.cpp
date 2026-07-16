#include "menu.h"

#include "tasks/api.h"
#include "tasks/display.h"
#include "tasks/wifi.h"
#include "tasks/ble.h"

extern Api api;
extern Display display;
extern Wifi wifi;
extern Ble ble;

Menu::Menu() {
    _items[idx(Key::Wifi)].label = "WiFi";
    _items[idx(Key::Wifi)].action = &Menu::onWifiSelect;

    _items[idx(Key::AP)].label = "WiFi AP";
    _items[idx(Key::AP)].action = &Menu::onApSelect;

    _items[idx(Key::BLE)].label = "BLE Disabled";
    _items[idx(Key::BLE)].action = &Menu::onBleSelect;

    _items[idx(Key::Brightness)].label = "Brightness";
    _items[idx(Key::Brightness)].action = &Menu::onBrightnessSelect;

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
    _items[idx(Key::Brightness)].label = saved ? "Brightness Saved" : "Save Brightness";
    setDirty();
}

void Menu::onWifiChange() {
    char label[32] = "WiFi Disabled";
    if (wifi.isStaEnabled())
        snprintf(label, sizeof(label), "WiFi %s", wifi.isStaConnected() ? "Connected" : "Disconnected");
    _items[idx(Key::Wifi)].label = label;
    snprintf(label, sizeof(label), "AP Disabled");
    if (wifi.isApEnabled())
        snprintf(label, sizeof(label), "AP Clients: %d", wifi.apClientCount());
    _items[idx(Key::AP)].label = label;
    setDirty();
}

void Menu::onBleChange() {
    char label[32] = "BLE Disabled";
    if (ble.isEnabled())
        snprintf(label, sizeof(label), "BLE %s", ble.isConnected() ? "Connected" : "Ready");
    _items[idx(Key::BLE)].label = label;
    setDirty();
}

void Menu::onWifiSelect() {
    if (!api.queueCommand("wifi toggle")) {
        ESP_LOGE(tag, "Failed to queue WiFi toggle command");
    }
}

void Menu::onApSelect() {
    if (!api.queueCommand("wifi ap toggle")) {
        ESP_LOGE(tag, "Failed to queue WiFi AP toggle command");
    }
}

void Menu::onBleSelect() {
    if (!api.queueCommand("ble toggle")) {
        ESP_LOGE(tag, "Failed to queue BLE toggle command");
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
